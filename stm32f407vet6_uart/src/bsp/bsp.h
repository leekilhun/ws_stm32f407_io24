/*
 * bsp.h
 *
 *  Created on: May 6, 2022
 *      Author: gns2l
 */

#ifndef SRC_BSP_BSP_H_
#define SRC_BSP_BSP_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "def.h"
#include "rtos.h"

#include "stm32f4xx_hal.h"

#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_system.h"
#include "stm32f4xx_ll_exti.h"
#include "stm32f4xx_ll_cortex.h"
#include "stm32f4xx_ll_utils.h"
#include "stm32f4xx_ll_pwr.h"
#include "stm32f4xx_ll_dma.h"
#include "stm32f4xx_ll_gpio.h"



bool bspInit(void);
void bspDeInit(void);

void delay(uint32_t ms);
uint32_t millis(void);
void Error_Handler(void);
void logPrintf(const char *fmt, ...);





#ifdef __cplusplus
}
#endif




#endif /* SRC_BSP_BSP_H_ */
