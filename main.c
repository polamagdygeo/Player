/**
 * main.c
 */


#include <Player.h>
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
#include "driverlib/interrupt.h"

int main(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);

    Time_Init();
    Sd_Init();
    FAT32_Init();
    Mp3Player_Init();

    Mp3Player_Update();


    while(1)
    {

    }

	return 0;
}
