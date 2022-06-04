/*
 * rtos.h
 *
 *  Created on: May 6, 2022
 *      Author: gns2l
 */

#ifndef SRC_BSP_RTOS_H_
#define SRC_BSP_RTOS_H_

#include "def.h"


#ifdef __cplusplus
extern "C" {
#endif


#include "cmsis_os.h"
#include "cpu_utils.h"


void rtosInit(void);



#ifdef __cplusplus
}
#endif


#endif /* SRC_BSP_RTOS_H_ */
