/*
 * tickTimer.c
 *
 *  Created on: 2021. 4. 25.
 *      Author: gns2l
 */

#include "tickTimer.h"

#define DEF_TICK_TIMER_USE_TIMER_MAX 				16

typedef struct
{
	tickTimerType 					type;
	func_state						state;
	uint32_t						pass_ms;
	void (*func_callback)(void);
	bool							is_start;

}tickTimer_tbl_t;

static tickTimer_tbl_t timer_tbl[TICKTIMER_TYPE_max] =
		{
				{ TICKTIMER_TYPE_1ms, 	_DISABLE, 		0, 	NULL,	false},
				{ TICKTIMER_TYPE_20ms, 	_DISABLE,		0,	NULL,	false},
				{ TICKTIMER_TYPE_100ms, _DISABLE,		0,	NULL, 	false},
		};


typedef struct
{
	uint32_t							pass_ms;
	bool								is_start;
} timer_tab_t;


static timer_tab_t timer[DEF_TICK_TIMER_USE_TIMER_MAX] = {0} ;

static uint8_t tickTimerRetrun_id(void);

uint8_t tickTimerRetrun_id(void)
{
	for (int i = 0; i < DEF_TICK_TIMER_USE_TIMER_MAX; i++)
	{
		if (timer[i].is_start == false)
			return i;
	}

	return DEF_TICK_TIMER_USE_TIMER_MAX;
}

uint8_t tickTimer_Start(void)
{
	uint8_t id = tickTimerRetrun_id();
	if (id < DEF_TICK_TIMER_USE_TIMER_MAX)
	{
		timer[id].is_start = true;
		timer[id].pass_ms  = 0;
	}
	return id;
}

void tickTimer_Stop(uint8_t id)
{
	timer[id].is_start = false;
	timer[id].pass_ms  = 0;
}

uint32_t tickTimer_GetElaspTime(uint8_t id)
{
	return timer[id].pass_ms;
}

/**
 * return true , false
 */
bool	tickTimer_LessThan(uint8_t id, uint32_t msec)
{
	if (tickTimer_GetElaspTime(id) < msec)
	{
		tickTimer_Stop(id);
		return true;
	}
	else
		return false;
}

/**
 * return true , false
 */
bool	tickTimer_MoreThan(uint8_t id, uint32_t msec)
{
	if (tickTimer_GetElaspTime(id) > msec)
	{
		tickTimer_Stop(id);
		return true;
	}
	else
		return false;
}

bool  tickTimer_IsStarted(uint8_t id)
{
	return timer[id].is_start;
}


uint32_t tickTimerStart(tickTimerType type)
{
	timer_tbl[type].is_start = true;
	timer_tbl[type].pass_ms = 0;
	timer_tbl[type].state = _ENABLE;

	return _DEF_ERR_SUCCESS;
}

uint32_t tickTimerStop(tickTimerType type)
{
	timer_tbl[type].is_start = false;
	timer_tbl[type].pass_ms = 0;
	timer_tbl[type].state = _DISABLE;

	return _DEF_ERR_SUCCESS;
}

uint32_t tickTimerGetElaspTime(tickTimerType type)
{
	return timer_tbl[type].pass_ms;
}


bool	tickTimerLessThan(tickTimerType type, uint32_t msec)
{
	return (tickTimerGetElaspTime(type) < msec ? true : false);
}

bool	tickTimerMoreThan(tickTimerType type, uint32_t msec)
{
	return (tickTimerGetElaspTime(type) > msec ? true : false);
}

bool	tickTimerIsStarted(tickTimerType type)
{
	return timer_tbl[type].is_start;
}


void tickTimerAttachCallBackFunc(tickTimerType type, void (*func)())
{
	timer_tbl[type].func_callback = func;
}

void tickTimerISR(void)
{

	for (int i = 0; i < TICKTIMER_TYPE_max; i++)
	{
		if (timer_tbl[i].state == _ENABLE)
		{
			timer_tbl[i].pass_ms += 1 ;

			if (timer_tbl[i].func_callback != NULL )
			{
				switch (i)
				{
					case TICKTIMER_TYPE_1ms:
						 (*timer_tbl[i].func_callback)();
						break;
					case TICKTIMER_TYPE_20ms:
						if (timer_tbl[i].pass_ms >20)
						{
							tickTimerStart(i);
							(*timer_tbl[i].func_callback)();
						}
						break;
					case TICKTIMER_TYPE_100ms:
						if (timer_tbl[i].pass_ms >100)
						{
							tickTimerStart(i);
							(*timer_tbl[i].func_callback)();
						}
						break;
				}
			}
		}
	}


	for (int i = 0; i < DEF_TICK_TIMER_USE_TIMER_MAX; i++)
	{
		if (timer[i].is_start == true)	timer[i].pass_ms += 1 ;

	}



}
