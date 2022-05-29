/*
 * ssd1306.h
 *
 *  Created on: May 3, 2021
 *      Author: gns2l
 */

#ifndef SRC_COMMON_INC_MODULE_LCD_SSD1306_H_
#define SRC_COMMON_INC_MODULE_LCD_SSD1306_H_

#include "lcd.h"

#ifdef _USE_MODULE_LCD1306_I2C

#ifdef __cplusplus
extern "C" {
#endif


	bool ssd1306Init(void);
	bool ssd1306InitDriver(lcd_driver_t *p_driver);


#ifdef __cplusplus
}
#endif

#endif

#endif /* SRC_COMMON_INC_MODULE_LCD_SSD1306_H_ */
