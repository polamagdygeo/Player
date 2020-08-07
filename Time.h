/**
 ******************************************************************************
 * @file           : Time
 * @brief          :
 ******************************************************************************
 */

#include "stdint.h"

void Time_Init(void);
void Time_Delay_MS(uint32_t delay_ms);
void Time_Timeout_Trigger(uint32_t time_ms);
uint8_t Time_Timeout_Poll(void);
