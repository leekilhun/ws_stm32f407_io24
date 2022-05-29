/*
 * buzzer.h
 *
 *  Created on: Dec 28, 2021
 *      Author: gns2.lee
 */

#ifndef SRC_COMMON_INC_HW_BUZZER_H_
#define SRC_COMMON_INC_HW_BUZZER_H_


#include "hw_def.h"

#ifdef _USE_HW_BUZZER



#ifdef __cplusplus
extern "C" {
#endif

bool buzzerInit(void);

void buzzerOn(void);
void buzzerOff(void);
void buzzerToggle(void);

void buzzerBeep(uint8_t times, uint32_t delay_ms);

#ifdef __cplusplus
}
#endif

#endif /*_USE_HW_BUZZ*/

#endif /* SRC_COMMON_INC_HW_BUZZER_H_ */
