/*
 * opPanel.h
 *
 *  Created on: Dec 29, 2021
 *      Author: gns2.lee
 */

#ifndef SRC_COMMON_INC_MODULE_OPPANEL_H_
#define SRC_COMMON_INC_MODULE_OPPANEL_H_

#include "exhw_def.h"


#ifdef _USE_MODULE_OPPANEL

#ifdef __cplusplus
extern "C" {
#endif

  typedef enum
  {
    OP_MODE_READY,
    OP_MODE_AUTORUN,
    OP_MODE_STOP,
    OP_MODE_DRY_RUN,
  } opMode;


  typedef enum
  {
    OP_INIT_STATUS ,
    OP_ERROR_STOP,
    OP_STEP_STOP,
    OP_START_RUN, // ready
    OP_RUN,  // auto run
  } opStatus;


  typedef enum
  {
    OP_SWITCH_NONE,
    OP_SWITCH_START = _GPIO_OP_SW_START,
    OP_SWITCH_STOP= _GPIO_OP_SW_STOP,
    OP_SWITCH_RESET= _GPIO_OP_SW_RESET,
    OP_SWITCH_ESTOP= _GPIO_OP_SW_ESTOP,
  } opSwitch;



  bool opPanel_Init(void);
  opStatus opPanel_GetStatus(void);
  int opPanel_SetStatus(opStatus state);

  opMode opPanel_GetMode(void);
  int opPanel_SetMode(opMode state);


  bool opPanel_GetPressed(opSwitch op_sw);

#ifdef __cplusplus
}
#endif

#endif

#endif /* SRC_COMMON_INC_MODULE_OPPANEL_H_ */
