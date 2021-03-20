/*
 * SD_Card.c
 *
 *  Created on: ??�/??�/????
 *      Author: Pola
 */

#include "SD_Card.h"
#include <string.h>
#include <stdint.h>
#include "stdlib.h"
#include "Time.h"
#include "port.h"
#include "main.h"
#include "driverlib/gpio.h"
#include "driverlib/ssi.h"
#include "driverlib/pin_map.h"
#include "inc/hw_ssi.h"
#include "inc/hw_types.h"

#define SD_PACKET_SIZE 							(512U + 3)
#define SD_CMD_TIMEOUT							1000UL
#define SD_BLOCK_READ_READINESS_TIMEOUT			1000UL
#define SD_INIT_TIMEOUT							10000UL
#define SD_DUMMY_BYTE							0xff
#define SD_READY_BYTE							0xfe
#define SD_PREINIT_DELAY						500UL

typedef enum{
	SD_CMD_0 = 0,				/* GO_IDLE_STATE */
	SD_CMD_1 = 1,				/* SEND_OP_COND */
	SD_CMD_8 = 8,				/* SEND_IF_COND */
	SD_CMD_12 = 12,				/* STOP_TRANSMISSION */
	SD_CMD_16 = 16,				/* SET_BLOCKLEN */
	SD_CMD_17 = 17,				/* READ_SINGLE_BLOCK */
	SD_CMD_18 = 18,				/* READ_MULTIPLE_BLOCK */
	SD_CMD_24 = 24,				/* WRITE_BLOCK */
	SD_CMD_25 = 25,				/* WRITE_MULTIPLE_BLOCK */
	SD_CMD_41 = 41,				/* SEND_OP_COND (ACMD) */
	SD_CMD_55 = 55,				/* APP_CMD */
	SD_CMD_58 = 58,				/* READ_OCR */
	SD_CMD_63 = 63
}tSD_Cmd;

typedef enum{
	SD_INIT_ENTER_SPI_MODE,
	SD_INIT_SEND_CMD0,
	SD_INIT_SEND_CMD8,
	SD_INIT_SEND_CMD55_SDV1,
	SD_INIT_SEND_CMD41_SDV1,
	SD_INIT_SEND_CMD55_SDV2,
	SD_INIT_SEND_CMD41_SDV2,
	SD_INIT_SEND_CMD58,
	SD_INIT_SEND_CMD16
}tSD_InitStates;

typedef enum{
    SD_CMD_STATE_SEND_WAIT_READINESS,
	SD_CMD_STATE_SEND,
	SD_CMD_STATE_SEND_RESPONSE
}tSD_CmdState;

static uint8_t Sd_TranceiveByte(uint8_t byte);
static int8_t Sd_SendCommand(uint8_t command,uint32_t arguement,uint8_t* r1_response,uint32_t* r7_response,uint8_t crc);
static uint8_t Sd_ReceiveDataBlock(uint8_t* buffer);

static uint8_t Sd_TranceiveByte(uint8_t byte)
{
	uint32_t ret = 0;

	/*Once byte is shifted ,get the received byte*/
    SSIDataPut(SSI2_BASE,byte);

    SSIDataGet(SSI2_BASE,&ret);

	return ret;
}

static int8_t Sd_SendCommand(uint8_t command,
		uint32_t arguement,
		uint8_t* r1_response,
		uint32_t* r7_response,
		uint8_t crc)
{
	tSD_CmdState state = SD_CMD_STATE_SEND_WAIT_READINESS;
	uint8_t idx;
	uint8_t Response_temp;
	int8_t ret_val = -1;
	uint8_t arguement_char;


	if(command <= SD_CMD_63)
	{
		ret_val = 0;

		/*Deselect*/
		GPIOPinWrite(SPI_PORT, 1 << SPI_CS_PIN, 1 << SPI_CS_PIN); /*CS Low for the entire program*/
	
		if(Sd_TranceiveByte(SD_DUMMY_BYTE) == SD_DUMMY_BYTE)
		{
			state = SD_CMD_STATE_SEND;
		}

        Time_Timeout_Trigger(SD_CMD_TIMEOUT);

		do
		{
			switch(state)
			{
            case SD_CMD_STATE_SEND_WAIT_READINESS:
				/*wait to be ready*/
				GPIOPinWrite(SPI_PORT, 1 << SPI_CS_PIN, 0 << SPI_CS_PIN); /*CS Low for the entire program*/

				Time_Delay_US(100);

                if(Sd_TranceiveByte(SD_DUMMY_BYTE) == SD_DUMMY_BYTE)
                {
                    state = SD_CMD_STATE_SEND;
                }
                break;
			case SD_CMD_STATE_SEND:
				Sd_TranceiveByte(command | 0x40);

				/*Command Args are 4 bytes*/
                for(idx = 0 ; idx < 4; idx++)   
                {
                    arguement_char = *(((uint8_t*)&arguement) + (3 - idx));
                    Sd_TranceiveByte(arguement_char);
                }

                Sd_TranceiveByte(crc);

                state = SD_CMD_STATE_SEND_RESPONSE;
				break;
			case SD_CMD_STATE_SEND_RESPONSE:
				/*Send dummy bytes until response is received*/
			    Response_temp = Sd_TranceiveByte(SD_DUMMY_BYTE);

				if((Response_temp & (1 << 7)) == 0)
				{
				    *r1_response = Response_temp;

	                if((command == CMD8) ||
	                        (command == CMD58))
	                {
	                    for(idx = 0 ; idx < 4; idx++)   /*response (1 / 1+4)*/
	                    {
	                        Response_temp = Sd_TranceiveByte(0xff);
	                        *R7 |= (uint32_t)Response_temp << ((3 - idx) * 8);
	                    }
	                }
	                else
	                {
	                    *R7 = 0;
	                }

	                ret_val = 1;
				}
				break;
			}
		}while((ret_val == 0) && (Time_Timeout_Poll() == 0));
	}

	return ret_val;
}

int8_t Sd_Init(void)
{
	static uint8_t state = SD_INIT_ENTER_SPI_MODE;
	uint8_t response_r1;
	uint32_t response_r7;
	uint8_t send_cmd_ret = 0;
	int8_t is_done = 0;
	uint8_t itr = 0;

	Time_Delay_MS(SD_PREINIT_DELAY);

	/**
	 *  There's a problem in the MOSI line that needs its disconnect and reconnect again at first sight it's HW issue
	 *
	 *  Both SD_INIT_ENTER_SPI_MODE implementation have same behavior
	 */

	/*Should be reentrant*/
	Time_Timeout_Trigger(SD_INIT_TIMEOUT);

	do
	{
		switch(state)
		{
        	case SD_INIT_ENTER_SPI_MODE:
				/*Requires generating 80 pulse with a specific frequency to enter this mode*/
				GPIOPinTypeGPIOOutput(SPI_PORT, 1 << SPI_CS_PIN);
				GPIOPinTypeGPIOOutput(SPI_PORT, 1 << SPI_MOSI_PIN);

				GPIOPinWrite(SPI_PORT, 1 << SPI_CS_PIN, 1 << SPI_CS_PIN); /*CS high*/
				GPIOPinWrite(SPI_PORT, 1 << SPI_MOSI_PIN, 1 << SPI_MOSI_PIN);

				GPIODirModeSet(SPI_PORT, 1 << SPI_MISO_PIN, GPIO_DIR_MODE_HW);
				GPIOPadConfigSet(SPI_PORT, 1 << SPI_MISO_PIN, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPU);
	           	GPIOPinTypeSSI(SPI_PORT,1 << SPI_MISO_PIN);
				GPIOPinConfigure(GPIO_PB6_SSI2RX);
	           	GPIOPinTypeSSI(SPI_PORT,1 << SPI_CLK_PIN);
				GPIODirModeSet(SPI_PORT, 1 << SPI_CLK_PIN, GPIO_DIR_MODE_HW);
				GPIOPadConfigSet(SPI_PORT, 1 << SPI_CLK_PIN, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPD);
				GPIOPinConfigure(GPIO_PB4_SSI2CLK);
				GPIOPinTypeSSI(SPI_PORT,1 << SPI_MOSI_PIN);
				GPIOPinConfigure(GPIO_PB7_SSI2TX);

				SSIClockSourceSet(SSI2_BASE, SSI_CLOCK_SYSTEM);
				SSIConfigSetExpClk(SSI2_BASE, F_OSC, SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 400000, 8);
				SSIEnable(SSI2_BASE);

				for(itr = 0; itr < 10 ; itr++)
				{
					Sd_TranceiveByte(SD_DUMMY_BYTE);
				}

				state = SD_INIT_SEND_CMD0;
				break;

			case SD_INIT_SEND_CMD0:
			    /*Reset and idle state*/
				send_cmd_ret = Sd_SendCommand(SD_CMD_0,0,&response_r1,&response_r7,0x95);	/* (valid CRC for CMD0(0)) */

				if(send_cmd_ret == 1)
				{
                    if(response_r1 == 1)
                    {
                        state = SD_INIT_SEND_CMD8;
                    }
				}
				break;

			case SD_INIT_SEND_CMD8:
			    /*Identify version*/
				send_cmd_ret = Sd_SendCommand(SD_CMD_8,0x1AA,&response_r1,&response_r7,0x87);	/* (valid CRC for CMD8(0x1AA)) */

				if(send_cmd_ret == 1)
				{
					if(response_r1 == 1)
					{
						if(response_r7 == 0x1AA)	 /* The card can work at vdd range of 2.7-3.6V */
						{
							/*SD Version 2*/
							state = SD_INIT_SEND_CMD55_SDV2;
						}
					}
					else
					{
						state = SD_INIT_SEND_CMD55_SDV1;
					}
				}
				break;

			case SD_INIT_SEND_CMD55_SDV1:
			    Time_Delay_MS(1);

				send_cmd_ret = Sd_SendCommand(SD_CMD_55,0,&response_r1,&response_r7,0xff);

				if(send_cmd_ret == 1)
				{
					if(response_r1 == 1)
					{
					    state = SD_INIT_SEND_CMD41_SDV1;
					}
					else
					{
					    /*Non compatible voltage*/
						is_done = -1;
					}
				}

				break;

			case SD_INIT_SEND_CMD55_SDV2:
			    Time_Delay_MS(1);

				send_cmd_ret = Sd_SendCommand(SD_CMD_55,0,&response_r1,&response_r7,0xff);

				if(send_cmd_ret == 1)
				{
					if(response_r1 == 1)
					{
					    state = SD_INIT_SEND_CMD41_SDV2;
					}
					else
					{
					    /*Non compatible voltage*/
						is_done = -1;
					}
				}

				break;

            case SD_INIT_SEND_CMD41_SDV1:
                send_cmd_ret = Sd_SendCommand(SD_CMD_41,0,&response_r1,&response_r7,0xff);

                if(send_cmd_ret == 1)
                {
                    if(response_r1 == 0)
                    {
                        state = SD_INIT_SEND_CMD16;	/* SDv1 */
                    }
                }

                break;

			case SD_INIT_SEND_CMD41_SDV2:
                /*Declare high capacity support with HCS high*/
                send_cmd_ret = Sd_SendCommand(SD_CMD_41,1UL << 30,&response_r1,&response_r7,0xff);

                if(send_cmd_ret == 1)
                {
                    if(response_r1 == 0)
                    {
                        state = SD_INIT_SEND_CMD58;
                    }
                    else
                    {
                        state = SD_INIT_SEND_CMD55_SDV2;
                    }
                }

                break;

			case SD_INIT_SEND_CMD58:
			    /*Read OCR to identify capacity*/
				send_cmd_ret = Sd_SendCommand(SD_CMD_58,0,&response_r1,&response_r7,0);

				if(send_cmd_ret == 1)
				{
					if(response_r1 == 0)
					{
						if((response_r7 & (1UL << 30)) == 1)/*Check CCS bit in OCR*/
						{
							/*High/extended capacity*/
							is_done = 1;
						}
						else
						{
						    /*Standard capacity*/
							state = SD_INIT_SEND_CMD16;
						}
					}
				}

				break;

			case SD_INIT_SEND_CMD16:
			    /*Set block length to 512 = 0x200*/
				send_cmd_ret = Sd_SendCommand(SD_CMD_16,512,&response_r1,&response_r7,0);

				if(send_cmd_ret == 1)
				{
					if(response_r1 == 0)
					{
						is_done = 1;
					}
				}

				break;
		}

	}while((is_done == 0) && (Time_Timeout_Poll() == 0));

	if(is_done)
	{
        /*Now u can maximize clock freq of spi*/
	}

	return is_done;
}

static uint8_t Sd_ReceiveDataBlock(uint8_t* buffer)
{
	uint32_t idx;
	uint8_t ret_val = 0;
	
    memset(buffer,0xff,SD_BLOCK_SIZE);

    Time_Timeout_Trigger(SD_BLOCK_READ_READINESS_TIMEOUT);

	/*Check readiness*/
    do
    {
        if(Sd_TranceiveByte(SD_DUMMY_BYTE) == SD_READY_BYTE)
        {
            ret_val = 1;
        }
    }while((ret_val == 0) && (Time_Timeout_Poll() == 0));

    if(ret_val == 1)
    {
        for(idx = 0 ; idx < SD_BLOCK_SIZE; idx++)
        {
            buffer[idx] = Sd_TranceiveByte(SD_DUMMY_BYTE);
        }

        /*Discard CRC*/
        Sd_TranceiveByte(SD_DUMMY_BYTE);
        Sd_TranceiveByte(SD_DUMMY_BYTE);
    }

	return ret_val;
}

uint8_t Sd_ReadBlock(uint32_t data_block_addr,uint8_t* buffer)
{
	uint8_t ret_val = 0;
	uint8_t response_r1;
	uint32_t response_r7;
	uint8_t is_response_received = 0;

	is_response_received = Sd_SendCommand(SD_CMD_17,data_block_addr,&response_r1,&response_r7,0);

	if(is_response_received == 1)
	{
		if(response_r1 == 0)
		{
			ret_val = Sd_ReceiveDataBlock(buffer);
		}
	}

	return ret_val;
}

uint8_t Sd_WriteBlock(uint32_t data_block_addr,uint8_t* buffer)
{
	uint8_t ret_val = 0;
	uint8_t response_r1;
	uint32_t response_r7;
	uint8_t is_response_received = 0;

	is_response_received = Sd_SendCommand(SD_CMD_24,data_block_addr,&response_r1,&response_r7,0);

	if(is_response_received == 1)
	{
		if(response_r1 == 0)
		{
			/*TODO :Add send data block*/
		}
	}

	return ret_val;
}

uint8_t Sd_ReadMultipleBlock(uint32_t data_block_addr,uint8_t blocks_number,uint8_t* buffer)
{
	uint8_t ret_val = 0;
	uint8_t idx;
	uint8_t response_r1;
	uint32_t response_r7;
	uint8_t is_response_received = 0;

	is_response_received = Sd_SendCommand(SD_CMD_18,data_block_addr,&response_r1,&response_r7,0);

	if(is_response_received == 1)
	{
		if(response_r1 == 0)
		{
			for(idx = 0 ; idx < blocks_number; idx++)
			{
				ret_val = Sd_ReceiveDataBlock(buffer + (SD_PACKET_SIZE * idx));
			}

			/*Check why*/
			is_response_received = Sd_SendCommand(SD_CMD_12,0,&response_r1,&response_r7,0);

	        if(response_r1 == 0)
	        {

	        }
		}
	}

	return ret_val;
}

uint8_t Sd_WriteMultipleBlock(uint32_t data_block_addr,
		uint8_t blocks_number,
		uint8_t* buffer)
{
	uint8_t ret_val = 0;
	uint8_t idx;
	uint8_t response_r1;
	uint32_t response_r7;
	uint8_t is_response_received = 0;

	is_response_received = Sd_SendCommand(SD_CMD_25,data_block_addr,&response_r1,&response_r7,0);

	if(is_response_received == 1)
	{
		if(response_r1 == 0)
		{
			for(idx = 0 ; idx < blocks_number; idx++)
			{
				/*TODO :Add send data block*/
			}
		}
	}

	return ret_val;
}
