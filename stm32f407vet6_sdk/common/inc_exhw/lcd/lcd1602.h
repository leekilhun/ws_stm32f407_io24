/*
 * LCD1602.h
 *
 *  Created on: Feb 28, 2021
 *      Author: gns2l
 */

#ifndef INC_LCD1602_H_
#define INC_LCD1602_H_



#include "hw_def.h"
#include "lcd.h"


#define LCD_DELAY_MS 5


#if _USE_HW_LCD1602_I2C_8574


bool lcd1602Init(void);
bool lcd1602InitDriver(lcd_driver_t *p_driver);



#endif  /*_USE_HW_LCD1602_I2C_8574*/




#if 0


#include "main.h"
#include "stm32f1xx_hal.h"

#define LCD_ADDR  0x27

#define PIN_RS    (1 << 0)
#define PIN_EN    (1 << 2)
#define BACKLIGHT (1 << 3)

#define LCD_DELAY_MS 5

void I2C_Scan(void);
HAL_StatusTypeDef LCD_SendInternal(uint8_t lcd_addr, uint8_t data, uint8_t flags);
void LCD_SendCommand(uint8_t lcd_addr, uint8_t cmd);
void LCD_SendData(uint8_t lcd_addr, uint8_t data);
void LCD_Init(uint8_t lcd_addr);
void LCD_SendString(uint8_t lcd_addr, char *str);





#endif

#endif /* INC_LCD1602_H_ */
