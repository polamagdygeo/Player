

/**
 * main.c
 */


#include "main.h"
#include "driverlib/sysctl.h"
#include "SD_Card.h"
#include "FAT32.h"
#include "Time.h"

#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/ssi.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "inc/hw_ssi.h"
#include "inc/hw_types.h"

int main(void)
{
    tDirectoryEntry *directory_array = 0;
    uint8_t size = 0;
    uint8_t buffer[512];

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);


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

    Time_Init();
    Sd_Init();
    FAT32_Init();

    FAT32_SearchFilesWithExtension("TXT",&directory_array,&size);

    while(FAT32_ReadFileAsBlocks(&directory_array[0],&buffer) == 0)
    {
        buffer[0] = 0;
    }

    PWMGenDisable(PWM1_BASE, PWM_GEN_2);

    while(1)
    {

    }

	return 0;
}