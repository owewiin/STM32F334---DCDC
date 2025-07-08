/*
 * CtlLoop.h
 *
 *  Created on: Jan 2, 2025
 *      Author: mshome
 */

#ifndef INC_CTLLOOP_H_
#define INC_CTLLOOP_H_

#include "function.h"
#include "stm32f3xx_hal.h"

void BUCK_Voltage_Increse_PID(int32_t Ref,int32_t Real);
void BuckModePWMReflash(void);
void LoopCtl(void);
void ResetLoopValue(void);

#endif /* INC_CTLLOOP_H_ */
