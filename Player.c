/*
 * Mp3Player.c
 *
 *  Created on: Jul 18, 2020
 *      Author: pola
 */
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "inc/hw_types.h"
#include "driverlib/pin_map.h"
#include "inc/hw_memmap.h"
#include "FAT32.h"
#include "wave_decoder.h"
#include "string.h"

uint8_t buffer[512];

void Mp3Player_Init(void)
{
    /*
     *  PWM for testing LED
     */
    GPIOPinTypePWM(GPIO_PORTF_BASE,1 << 1);
    GPIOPinConfigure(GPIO_PF1_M1PWM5);

    PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN |
                    PWM_GEN_MODE_NO_SYNC |
                    PWM_GEN_MODE_DBG_RUN |
                    PWM_GEN_MODE_GEN_NO_SYNC |
                    PWM_GEN_MODE_DB_NO_SYNC |
                    PWM_GEN_MODE_FAULT_UNLATCHED |
                    PWM_GEN_MODE_FAULT_NO_MINPER |
                    PWM_GEN_MODE_FAULT_LEGACY);

    PWMClockSet(PWM1_BASE, PWM_SYSCLK_DIV_64);

    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2, 60000);
//  PWMPulseWidthSet(PWM1_BASE, PWM_OUT_2, 10);
    HWREG(0x400290DC) = 50000;

    PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT, 1);

    PWMGenEnable(PWM1_BASE, PWM_GEN_2);
}

void Mp3Player_Update(void)
{
    char *files_name_arr[10] = {0};
    uint8_t count = 0;
    uint8_t itr;
    tWave_FrameInfo info;
    uint32_t samples_no;
    tWave_PcmFormat8Bit *pcm_buffer;

    FAT32_ListFiles(files_name_arr,&count);

    for(itr = 0 ; itr < 10 && (files_name_arr[itr] != 0) ; itr++)
    {
        if(strcmp(files_name_arr[itr],"taunt.wav") == 0)
        {
            break;
        }
    }

    while(FAT32_ReadFileAsBlocks(files_name_arr[itr],&buffer) == 0)
    {
        Wave_Decode(buffer,512, &info, (void**)&pcm_buffer, &samples_no);

        /*play the 8 bit samples*/
    }

    PWMGenDisable(PWM1_BASE, PWM_GEN_2);
}
