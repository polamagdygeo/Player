/*
 * Player.c
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
#include "Time.h"

uint8_t file_block_buffer[SD_BLOCK_SIZE];

void MusicPlayer_Init(void)
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
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_2, 50000);
    // HWREG(0x400290DC) = 50000;

    PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT, 1);

    PWMGenEnable(PWM1_BASE, PWM_GEN_2);
}

void MusicPlayer_Update(void)
{
    char *files_name_arr[10] = {0};
    uint8_t count = 0;
    uint8_t itr;
    tWave_DecodingInfo info = {0};
    uint32_t samples_block_itr;
    uint8_t file_read_done;

    FAT32_ListFiles(files_name_arr,&count);

    for(itr = 0 ; itr < count && (files_name_arr[itr] != 0) ; itr++)
    {
        if(strcmp(files_name_arr[itr],"taunt.wav") == 0)
        {
            break;
        }
    }

    if(itr != count)
    {
        while((file_read_done = FAT32_ReadFileAsBlocks(files_name_arr[itr],&file_block_buffer)) != -1)
        {
            Wave_DecodeFrameSegment(file_block_buffer,SD_BLOCK_SIZE,&info);

            if(info.channels_no == 1)
            {
                if(info.bits_per_sample == 8)
                {
                    for(samples_block_itr = 0 ; samples_block_itr < info.last_read_samples_block_no ; samples_block_itr++)
                    {
                        ((tWave_RawPcmMono8BitSample*)info.pcm_buffer)[samples_block_itr];
                        Time_Delay_US(info.sample_period_us);
                    }
                }
                else
                {
                    for(samples_block_itr = 0 ; samples_block_itr < info.last_read_samples_block_no ; samples_block_itr++)
                    {
                        ((tWave_RawPcmMono16BitSample*)info.pcm_buffer)[samples_block_itr];
                        Time_Delay_US(info.sample_period_us);
                    }
                }
            }
            else
            {
                if(info.bits_per_sample == 8)
                {
                    for(samples_block_itr = 0 ; samples_block_itr < info.last_read_samples_block_no ; samples_block_itr++)
                    {
                        ((tWave_RawPcmStreo8BitSample*)info.pcm_buffer)[samples_block_itr].left_channel_val;
                        ((tWave_RawPcmStreo8BitSample*)info.pcm_buffer)[samples_block_itr].right_channel_val;
                        Time_Delay_US(info.sample_period_us);
                    }
                }
                else
                {
                    for(samples_block_itr = 0 ; samples_block_itr < info.last_read_samples_block_no ; samples_block_itr++)
                    {
                        ((tWave_RawPcmStreo16BitSample*)info.pcm_buffer)[samples_block_itr].left_channel_val;
                        ((tWave_RawPcmStreo16BitSample*)info.pcm_buffer)[samples_block_itr].right_channel_val;
                        Time_Delay_US(info.sample_period_us);
                    }
                }
            }

            if(file_read_done)
            {
                break;
            }
        }
    }

    PWMGenDisable(PWM1_BASE, PWM_GEN_2);
}
