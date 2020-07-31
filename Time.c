/**
 ******************************************************************************
 * @file           : Time
 * @brief          :
 ******************************************************************************
 */

#include "Time.h"
#include "main.h"
#include "driverlib/timer.h"
#include "inc/hw_memmap.h"
/**
    *@brief Time Init
    *@param void
    *@retval void
*/
void Time_Init(void)
{
    TimerConfigure(TIMER0_BASE,TIMER_CFG_ONE_SHOT_UP);
    TimerPrescaleSet(TIMER0_BASE,TIMER_BOTH, 16);
    TimerClockSourceSet(TIMER0_BASE,TIMER_CLOCK_SYSTEM);
}

void Time_Delay_MS(uint32_t delay_ms)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    TimerLoadSet(TIMER0_BASE,TIMER_BOTH,delay_ms*1000);
    TimerEnable(TIMER0_BASE, TIMER_BOTH);
    while((TimerIntStatus(TIMER0_BASE,0) & TIMER_TIMA_TIMEOUT) == 0);
    TimerDisable(TIMER0_BASE, TIMER_BOTH);
}

/**
    *@brief
    *@param void
    *@retval void
*/
void Time_Timeout_Trigger(uint32_t time_ms)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    TimerLoadSet(TIMER0_BASE,TIMER_BOTH,time_ms*1000);
    TimerEnable(TIMER0_BASE, TIMER_BOTH);
}

/**
    *@brief Time timeout poll
    *@param void
    *@retval void
*/
uint8_t Time_Timeout_Poll(void)
{
	uint8_t ret = 0;

	if((TimerIntStatus(TIMER0_BASE,0) & TIMER_TIMA_TIMEOUT) != 0)
	{
		ret = 1;
		TimerDisable(TIMER0_BASE, TIMER_BOTH);
	}

	return ret;
}
