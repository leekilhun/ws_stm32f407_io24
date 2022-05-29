/*
 * led.h
 *
 *  Created on: Jul 10, 2021
 *      Author: gns2l
 */

#ifndef SRC_COMMON_INC_HW_LED_H_
#define SRC_COMMON_INC_HW_LED_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "hw_def.h"


#ifdef _USE_HW_LED

#define LED_MAX_CH      HW_LED_MAX_CH


bool ledInit(void);
void ledOn(uint8_t ch);
void ledOff(uint8_t ch);
void ledToggle(uint8_t ch);

#endif


#ifdef __cplusplus
}
#endif


#endif /* SRC_COMMON_INC_HW_LED_H_ */
