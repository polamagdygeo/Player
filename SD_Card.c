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
#include "driverlib/timer.h"
#include "driverlib/ssi.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "inc/hw_ssi.h"
#include "inc/hw_types.h"

#define SD_PACKET_SIZE 			(512U + 3)

typedef enum{
	CMD0 = 0|1<<6,
	CMD1 = 1|1<<6,
	CMD8 = 8|1<<6,
	CMD12 = 12|1<<6,
	CMD16 = 16|1<<6,
	CMD17 = 17|1<<6,
	CMD18 = 18|1<<6,
	CMD24 = 24|1<<6,
	CMD25 = 25|1<<6,
	CMD41 = 41|1<<6,
	CMD55 = 55|1<<6,
	CMD58 = 58|1<<6,
	CMD63 = 63|1<<6
}tSD_CMD;

typedef enum{
	SD_INIT_ENTER_SPI_MODE,
	SD_INIT_SEND_CMD0,
	SD_INIT_SEND_CMD8,
	SD_INIT_SEND_CMD55,
	SD_INIT_SEND_CMD41,
	SD_INIT_SEND_CMD58,
	SD_INIT_SEND_CMD16
}tSD_InitStates;

typedef enum{
    SEND_WAIT_READINESS,
	SEND_CMD,
	SEND_CMD_RESPONSE
}tSD_SendCmdState;

static uint8_t Sd_TranceiveByte(uint8_t byte);
static uint8_t Sd_SendCommand(uint8_t command,uint32_t arguement,uint8_t* R1,uint32_t* R7,uint8_t crc);
static uint8_t Sd_ReceiveDataBlock(uint8_t* buffer);

static uint8_t Sd_TranceiveByte(uint8_t byte)
{
	uint32_t ret = 0;

    SSIDataPut(SSI2_BASE,byte);

    SSIDataGet(SSI2_BASE,&ret);

	return ret;
}

static uint8_t Sd_SendCommand(uint8_t command,
		uint32_t arguement,
		uint8_t* R1,
		uint32_t* R7,
		uint8_t crc)
{
	tSD_SendCmdState state = SEND_WAIT_READINESS;
	uint8_t idx;
	uint8_t Response_temp;
	uint8_t ret_val = 0;
	uint8_t arguement_char;


	if(command <= CMD63)
	{
        Time_Timeout_Trigger(1000);

		do
		{
			switch(state)
			{
            case SEND_WAIT_READINESS:
                if(Sd_TranceiveByte(0xff) == 0xff)
                {
                    state = SEND_CMD;
                }
                break;
			case SEND_CMD:
				Sd_TranceiveByte(command);

                for(idx = 0 ; idx < 4; idx++)   /*Args (4 bytes)*/
                {
                    arguement_char = *(((uint8_t*)&arguement) + (3-idx));
                    Sd_TranceiveByte(arguement_char);
                }

                Sd_TranceiveByte(crc);

                state = SEND_CMD_RESPONSE;
				break;
			case SEND_CMD_RESPONSE:
				/*Send dummy bytes until response is received*/
			    Response_temp = Sd_TranceiveByte(0xff);

				if((Response_temp & (1 << 7)) == 0)
				{
				    *R1 = Response_temp;

	                if(command == CMD8 ||
	                        command == CMD58 )
	                {
	                    for(idx = 0 ; idx < 4; idx++)   /*response (1 / 1+4)*/
	                    {
	                        Response_temp = Sd_TranceiveByte(0xff);
	                        *R7 |= (uint32_t)Response_temp << ((3-idx)*8);
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
		}while(ret_val == 0 && Time_Timeout_Poll() == 0);
	}

	return ret_val;
}

void Sd_Init(void)
{
	static uint8_t state = SD_INIT_ENTER_SPI_MODE;
	uint8_t response_r1;
	uint32_t response_r7;
	uint8_t ret_val = 0;
	uint8_t isDone = 0;

	Time_Delay_MS(500);

	/**
	 *  There's a problem in the MOSI line that needs its disconnect and reconnect again at first sight it's HW issue
	 *
	 *  Both SD_INIT_ENTER_SPI_MODE implementation have same behavior
	 */

	do
	{
		switch(state)
		{
//			case SD_INIT_ENTER_SPI_MODE:
//                GPIOPinTypeSSI(SPI_PORT, 1 << SPI_CLK_PIN |
//                               1 << SPI_MISO_PIN |
//                               1 << SPI_MOSI_PIN);
//                GPIOPinConfigure(GPIO_PB4_SSI2CLK);
//                GPIOPinConfigure(GPIO_PB6_SSI2RX);
//                GPIOPinConfigure(GPIO_PB7_SSI2TX);
//                GPIOPinTypeGPIOOutput(SPI_PORT, 1 << SPI_CS_PIN);
//			    SSIClockSourceSet(SSI2_BASE, SSI_CLOCK_SYSTEM);
//                SSIConfigSetExpClk(SSI2_BASE, F_OSC, SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 400000, 8);
//                SSIEnable(SSI2_BASE);
//
//                GPIOPinWrite(SPI_PORT, 1 << SPI_CS_PIN, 1 << SPI_CS_PIN); /*CS high*/
//
//			    for(i = 0 ; i < 74 ; i++)
//			    {
//			        Sd_TranceiveByte(0xff);
//			    }
//
////                GPIOPinTypeSSI(SPI_PORT,1 << SPI_CS_PIN);
////                GPIOPinConfigure(GPIO_PB5_SSI2FSS);
//
//                GPIOPinWrite(SPI_PORT, 1 << SPI_CS_PIN, 0 << SPI_CS_PIN); /*CS Low for the entire program*/
//
//				state = SD_INIT_SEND_CMD0;
//				break;

        case SD_INIT_ENTER_SPI_MODE:
            GPIOPinTypePWM(SPI_PORT,1 << SPI_CLK_PIN);
            GPIOPinConfigure(GPIO_PB4_M0PWM2);

            PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN |
                            PWM_GEN_MODE_NO_SYNC |
                            PWM_GEN_MODE_DBG_RUN |
                            PWM_GEN_MODE_GEN_NO_SYNC |
                            PWM_GEN_MODE_DB_NO_SYNC |
                            PWM_GEN_MODE_FAULT_UNLATCHED |
                            PWM_GEN_MODE_FAULT_NO_MINPER |
                            PWM_GEN_MODE_FAULT_LEGACY);

            PWMClockSet(PWM0_BASE, PWM_SYSCLK_DIV_2);

            PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, 20);
//          PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 10);
            HWREG(0x40028098) = 10;

            GPIOPinTypeGPIOOutput(SPI_PORT, 1 << SPI_CS_PIN);
            GPIOPinTypeGPIOOutput(SPI_PORT, 1 << SPI_MOSI_PIN);

            GPIOPinWrite(SPI_PORT, 1 << SPI_CS_PIN, 1 << SPI_CS_PIN); /*CS high*/
            GPIOPinWrite(SPI_PORT, 1 << SPI_MOSI_PIN, 1 << SPI_MOSI_PIN);

            PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT, 1);

            PWMGenEnable(PWM0_BASE, PWM_GEN_1);

            Time_Delay_MS(2);

            PWMGenDisable(PWM0_BASE, PWM_GEN_1);

            GPIODirModeSet(SPI_PORT, 1 << SPI_MISO_PIN, GPIO_DIR_MODE_HW);
            GPIOPadConfigSet(SPI_PORT, 1 << SPI_MISO_PIN, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPU);
//            GPIOPinTypeSSI(SPI_PORT,1 << SPI_MISO_PIN);
            GPIOPinConfigure(GPIO_PB6_SSI2RX);
//            GPIOPinTypeSSI(SPI_PORT,1 << SPI_CLK_PIN);
            GPIODirModeSet(SPI_PORT, 1 << SPI_CLK_PIN, GPIO_DIR_MODE_HW);
            GPIOPadConfigSet(SPI_PORT, 1 << SPI_CLK_PIN, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPD);
            GPIOPinConfigure(GPIO_PB4_SSI2CLK);
            GPIOPinTypeSSI(SPI_PORT,1 << SPI_MOSI_PIN);
            GPIOPinConfigure(GPIO_PB7_SSI2TX);

            SSIClockSourceSet(SSI2_BASE, SSI_CLOCK_SYSTEM);
            SSIConfigSetExpClk(SSI2_BASE, F_OSC, SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 400000, 8);
            SSIEnable(SSI2_BASE);

            GPIOPinWrite(SPI_PORT, 1 << SPI_CS_PIN, 0 << SPI_CS_PIN); /*CS Low for the entire program*/
            state = SD_INIT_SEND_CMD0;
            break;

			case SD_INIT_SEND_CMD0:
			    /*Reset and idle state*/
				ret_val = Sd_SendCommand(CMD0,0,&response_r1,&response_r7,0x95);

				if(ret_val == 1)
				{
                    if(response_r1 == 1)
                    {
                        state=SD_INIT_SEND_CMD8;
                    }
                    else
                    {
                        /*Unknown card*/
                    }
				}
				break;

			case SD_INIT_SEND_CMD8:
			    /*Identify version*/
				ret_val = Sd_SendCommand(CMD8,0x000001AA,&response_r1,&response_r7,0x87);

				if(ret_val==1)
				{
					if(response_r1 == 1)
					{
						if(response_r7 == 0x1AA)
						{
							/*SD Version 2*/
							state = SD_INIT_SEND_CMD55;
						}
						else if(response_r7 == 0x0000005)
						{
							/*SD Version 1*/
						    while(1);
						}
					}
				}
				break;

			case SD_INIT_SEND_CMD55:
			    /*Voltage range*/
				ret_val = Sd_SendCommand(CMD55,0x00000000,&response_r1,&response_r7,0xff);

				if(ret_val==1)
				{
					if(response_r1 == 1)
					{
					    state = SD_INIT_SEND_CMD41;
					}
					else
					{
					    /*Non compatible voltage*/
					    while(1);
					}
				}

				break;

            case SD_INIT_SEND_CMD41:
                /*Declare high capacity support with HCS high*/
                ret_val = Sd_SendCommand(CMD41,0x40000000,&response_r1,&response_r7,0xff);

                if(ret_val == 1)
                {
                    if(response_r1 == 0)
                    {
                        state = SD_INIT_SEND_CMD58;
                    }
                    else
                    {
                        state = SD_INIT_SEND_CMD55;
                    }
                }

                break;

			case SD_INIT_SEND_CMD58:
			    /*Read OCR to identify capacity*/
				ret_val = Sd_SendCommand(CMD58,0,&response_r1,&response_r7,0);

				if(ret_val == 1)
				{
					if(response_r1 == 0)
					{
						if((response_r7 & (1UL << 30)) == 1)/*Check CCS bit in OCR*/
						{
							/*High/extended capacity*/
							isDone = 1;
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
				ret_val = Sd_SendCommand(CMD16,0x200,&response_r1,&response_r7,0);

				if(ret_val == 1)
				{
					if(response_r1 == 0)
					{
						isDone = 1;
					}
				}

				break;
		}

	}while(isDone == 0);

	if(isDone)
	{
        /*Now u can maximize clock freq of spi*/
	}
}

static uint8_t Sd_ReceiveDataBlock(uint8_t* buffer)
{
	uint32_t idx;
	uint8_t ret_val = 0;
	
    memset(buffer,0xff,512);

    Time_Timeout_Trigger(1000);

    do
    {
        if(Sd_TranceiveByte(0xff) == 0xFE)
        {
            ret_val = 1;
        }
    }while(ret_val == 0 && Time_Timeout_Poll() == 0);

    if(ret_val == 1)
    {
        for(idx = 0 ; idx < 512; idx++) /*Block = 512 byte*/
        {
            buffer[idx] = Sd_TranceiveByte(0xff);
        }

        /*Discard CRC*/
        Sd_TranceiveByte(0xff);
        Sd_TranceiveByte(0xff);
    }

	return ret_val;
}

uint8_t Sd_ReadBlock(uint32_t data_block_addr,uint8_t* buffer)
{
	uint8_t ret_val = 0;
	uint8_t response_r1;
	uint32_t response_r7;
	uint8_t response_received_flag = 0;

	response_received_flag = Sd_SendCommand(CMD17,data_block_addr,&response_r1,&response_r7,0);

	if(response_received_flag == 1)
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
	uint8_t response_received_flag = 0;

	response_received_flag = Sd_SendCommand(CMD24,data_block_addr,&response_r1,&response_r7,0);

	if(response_received_flag == 1)
	{
		if(response_r1 == 0)
		{
			/*Add send data block*/
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
	uint8_t response_received_flag = 0;

	response_received_flag = Sd_SendCommand(CMD18,data_block_addr,&response_r1,&response_r7,0);

	if(response_received_flag == 1)
	{
		if(response_r1 == 0)
		{
			for(idx = 0 ; idx < blocks_number; idx++)
			{
				ret_val = Sd_ReceiveDataBlock(buffer+(SD_PACKET_SIZE*idx));
			}

			/*Check why*/
			response_received_flag = Sd_SendCommand(CMD12,0,&response_r1,&response_r7,0);

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
	uint8_t response_received_flag = 0;

	response_received_flag = Sd_SendCommand(CMD25,data_block_addr,&response_r1,&response_r7,0);

	if(response_received_flag == 1)
	{
		if(response_r1 == 0)
		{
			for(idx = 0 ; idx < blocks_number; idx++)
			{
				/*Add send data block*/
			}
		}
	}

	return ret_val;
}
