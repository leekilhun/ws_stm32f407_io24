/*
 * planner.h
 *
 *  Created on: 2022. 1. 27.
 *      Author: gns2l
 */

#ifndef SRC_AP_PLANNER_H_
#define SRC_AP_PLANNER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SCHEDULER_MAX        8

#include "def.h"

  typedef enum
  {
    JL_ONE_TIME = 1,
    JL_LOOP_TIME = 2,
  }job_mode_t;

  bool scheduler_Init(void);
  void scheduler_Set  (uint8_t ch, uint32_t timer_ms, job_mode_t job_mode, void (*func)());
  void scheduler_Start(uint8_t ch);
  void scheduler_Stop (uint8_t ch);
  void scheduler_Reset(uint8_t ch);
  void scheduler_ISR(void);

  uint8_t  scheduler_GetHandle(void);
  uint32_t scheduler_GetCounter(void);


  //void scheduler_Start(uint8_t ch, job_period period);
  //void scheduler_Stop(uint8_t ch, job_period period);
  //void scheduler_AttachCallback(job_period period, void (*func)());
  //void scheduler_ISR(void);



#ifdef __cplusplus
}
#endif


#endif /* SRC_AP_PLANNER_H_ */
