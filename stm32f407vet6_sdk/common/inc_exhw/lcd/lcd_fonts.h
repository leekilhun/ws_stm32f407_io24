/*
 * lcd_fonts.h
 *
 *  Created on: Feb 27, 2021
 *      Author: gns2l
 */

#ifndef SRC_HW_DRIVER_LCD_LCD_FONTS_H_
#define SRC_HW_DRIVER_LCD_LCD_FONTS_H_

#include "hw_def.h"


typedef struct
{
  uint8_t   width;
  uint8_t   height;
  const uint16_t *data;
} lcd_font_t;


extern lcd_font_t font_07x10;
extern lcd_font_t font_11x18;
extern lcd_font_t font_16x26;
extern lcd_font_t font_hangul;



#endif /* SRC_HW_DRIVER_LCD_LCD_FONTS_H_ */
