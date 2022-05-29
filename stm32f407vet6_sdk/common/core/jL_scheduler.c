/*
 * jL_scheduler.c
 *
 *  Created on: 2022. 1. 27.
 *      Author: gns2l
 */


#include "jL_scheduler.h"

typedef struct
{
  bool     scheduler_en;        // 타이머 인에이블 신호
  job_mode_t scheduler_mode;    // 타이머 모드
  uint32_t scheduler_ctn;       // 현재의 타이머 값
  uint32_t scheduler_init;      // 타이머 초기화될때의 카운트 값
  void (*func)(void );          // 만료될때 실행될 함수
}scheduler_t;

static volatile uint32_t scheduler_counter      = 0;
static volatile uint8_t  scheduler_handle_index = 0;
static scheduler_t  scheduler_tbl[SCHEDULER_MAX];           //  배열 선언


bool scheduler_Init(void)
{
  uint8_t i;
  static uint8_t excute = 0;

  if (excute == 1)
    return false;  // 이미 한번 실행했다면 정지.

  // 구조체 초기화
  for(i=0; i<SCHEDULER_MAX; i++)
  {
    scheduler_tbl[i].scheduler_en   = false;
    scheduler_tbl[i].scheduler_ctn  = 0;
    scheduler_tbl[i].scheduler_init = 0;
    scheduler_tbl[i].func       = NULL;
  }

  excute = 1;

  return true;
}
void scheduler_Set  (uint8_t ch, uint32_t timer_ms, job_mode_t job_mode, void (*func)())
{
  scheduler_tbl[ch].scheduler_mode = job_mode;    // 모드설정
  scheduler_tbl[ch].func           = func;        // 실행할 함수
  scheduler_tbl[ch].scheduler_ctn  = timer_ms;
  scheduler_tbl[ch].scheduler_init = timer_ms;
}

void scheduler_Start(uint8_t ch)
{
  if(ch < SCHEDULER_MAX)
  {
    scheduler_tbl[ch].scheduler_ctn = scheduler_tbl[ch].scheduler_init;
    scheduler_tbl[ch].scheduler_en  = true;
  }
}
void scheduler_Stop (uint8_t ch)
{
  if(ch < SCHEDULER_MAX)
  {
    scheduler_tbl[ch].scheduler_en = false;
  }
}

void scheduler_Reset(uint8_t ch)
{
  scheduler_tbl[ch].scheduler_en   = false;
  scheduler_tbl[ch].scheduler_ctn  = scheduler_tbl[ch].scheduler_init;
}


uint8_t scheduler_GetHandle(void)
{
  uint8_t ret = scheduler_handle_index;

  if (ret < SCHEDULER_MAX)
  {
    scheduler_handle_index++;
  }
  else
  {
    ret = 0xff;
  }

  return ret;
}

uint32_t scheduler_GetCounter(void)
{
  return scheduler_counter;
}


void scheduler_ISR(void)
{
  uint8_t i;

  scheduler_counter++;

  for (i=0; i<SCHEDULER_MAX && i<scheduler_handle_index; i++) // 타이머 갯수만큼
  {
    if ( scheduler_tbl[i].scheduler_en == true)                      // 타이머가 활성화 됬니?
    {
      scheduler_tbl[i].scheduler_ctn--;                              // 타이머값 감소

      if (scheduler_tbl[i].scheduler_ctn == 0)                       // 타이머 오버플로어
      {
        if(scheduler_tbl[i].scheduler_mode == JL_ONE_TIME)              // 한번만 실행하는거면
        {
          scheduler_tbl[i].scheduler_en = false;                     // 타이머 OFF 한다.
        }

        scheduler_tbl[i].scheduler_ctn = scheduler_tbl[i].scheduler_init; // 타이머 초기화

        (*scheduler_tbl[i].func)();     // 함수 실행
      }
    }
  }
}


#if 0
typedef struct
{
  job_period           jobPeriod;
  func_state            state;
  uint32_t            pass_ms;
  void (*func_callback)(void);
}scheduler_tbl_t;

static scheduler_tbl_t job_tbl[JOB_PERIOD_max] =
    {
      { JOB_PERIOD_1ms,     _DISABLE,   0,  NULL},
      { JOB_PERIOD_5ms,     _DISABLE,   0,  NULL},
      { JOB_PERIOD_10ms,    _DISABLE,   0,  NULL},
      { JOB_PERIOD_50ms,    _DISABLE,   0,  NULL},
      { JOB_PERIOD_100ms,   _DISABLE,   0,  NULL},
      { JOB_PERIOD_500ms,   _DISABLE,   0,  NULL},
      { JOB_PERIOD_1000ms,  _DISABLE,   0,  NULL},
    };

void scheduler_Start(job_period period)
{
  job_tbl[period].pass_ms = 0;
  job_tbl[period].state = _ENABLE;
}

void scheduler_Stop(job_period period)
{
  job_tbl[period].pass_ms = 0;
  job_tbl[period].state = _DISABLE;
}


void scheduler_AttachCallback(job_period period, void (*func)())
{
  job_tbl[period].func_callback = func;
}

void scheduler_ISR(void)
{
  for (int i = 0; i < JOB_PERIOD_max; i++)
  {
    if (job_tbl[i].state == _ENABLE)
    {
      job_tbl[i].pass_ms += 1 ;
    }
    if (job_tbl[i].func_callback != NULL)
    {
      switch(i)
      {
        case JOB_PERIOD_1ms:
          (*job_tbl[i].func_callback)();
          break;
        case JOB_PERIOD_5ms:
          if (job_tbl[i].pass_ms <6)
            break;
          (*job_tbl[i].func_callback)();
          job_tbl[i].pass_ms = 0;
          break;
        case JOB_PERIOD_10ms:
          if (job_tbl[i].pass_ms <11)
            break;
          (*job_tbl[i].func_callback)();
          job_tbl[i].pass_ms = 0;
          break;
        case JOB_PERIOD_50ms:
          if (job_tbl[i].pass_ms <51)
            break;
          (*job_tbl[i].func_callback)();
          job_tbl[i].pass_ms = 0;
          break;
        case JOB_PERIOD_100ms:
          if (job_tbl[i].pass_ms <101)
            break;
          (*job_tbl[i].func_callback)();
          job_tbl[i].pass_ms = 0;
          break;
        case JOB_PERIOD_500ms:
          if (job_tbl[i].pass_ms <501)
            break;
          (*job_tbl[i].func_callback)();
          job_tbl[i].pass_ms = 0;
          break;
        case JOB_PERIOD_1000ms:
          if (job_tbl[i].pass_ms <1001)
            break;
          (*job_tbl[i].func_callback)();
          job_tbl[i].pass_ms = 0;
          break;
      }
    }
  }
}

#endif
