/*
 * tickTimer.h
 *
 *  Created on: 2021. 4. 25.
 *      Author: gns2l
 */

#ifndef SRC_COMMON_INC_MODULE_TICKTIMER_H_
#define SRC_COMMON_INC_MODULE_TICKTIMER_H_

#include "def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	TICKTIMER_TYPE_1ms,
	TICKTIMER_TYPE_20ms,
	TICKTIMER_TYPE_100ms,

	TICKTIMER_TYPE_max
}tickTimerType;


uint8_t tickTimer_Start(void);
void tickTimer_Stop(uint8_t id);
uint32_t tickTimer_GetElaspTime(uint8_t id);
bool	tickTimer_LessThan(uint8_t id, uint32_t msec);
bool	tickTimer_MoreThan(uint8_t id, uint32_t msec);
bool  tickTimer_IsStarted(uint8_t id);

uint32_t tickTimerStart(tickTimerType type);
uint32_t tickTimerStop(tickTimerType type);
uint32_t tickTimerGetElaspTime(tickTimerType type);
bool	tickTimerLessThan(tickTimerType type, uint32_t msec);
bool	tickTimerMoreThan(tickTimerType type, uint32_t msec);
bool	tickTimerIsStarted(tickTimerType type);

void tickTimerAttachCallBackFunc(tickTimerType type, void (*func)());
void tickTimerISR(void);


#ifdef __cplusplus
}
#endif

#endif /* SRC_COMMON_INC_MODULE_TICKTIMER_H_ */
