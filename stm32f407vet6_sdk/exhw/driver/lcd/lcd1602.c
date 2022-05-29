/*
 * LCD1602.c
 *
 *  Created on: Feb 28, 2021
 *      Author: gns2l
 */

#include "lcd/lcd1602.h"
#include "i2c.h"

#if _USE_HW_LCD1602_I2C_8574


#define LCD1602_WIDTH       		16
#define LCD1602_HEIGHT      		2

// commands
#define LCD_CLEARDISPLAY 				0x01
#define LCD_RETURNHOME 					0x02
#define LCD_ENTRYMODESET 				0x04
#define LCD_DISPLAYCONTROL 			0x08
#define LCD_CURSORSHIFT 				0x10
#define LCD_FUNCTIONSET 				0x20
#define LCD_SETCGRAMADDR 				0x40
#define LCD_SETDDRAMADDR 				0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 					0x00
#define LCD_ENTRYLEFT 					0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 					0x04
#define LCD_DISPLAYOFF 					0x00
#define LCD_CURSORON 						0x02
#define LCD_CURSOROFF 					0x00
#define LCD_BLINKON 						0x01
#define LCD_BLINKOFF 						0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 				0x08
#define LCD_CURSORMOVE 					0x00
#define LCD_MOVERIGHT 					0x04
#define LCD_MOVELEFT 						0x00

// flags for function set
#define LCD_8BITMODE 						0x10
#define LCD_4BITMODE 						0x00
#define LCD_2LINE 							0x08
#define LCD_1LINE 							0x00
#define LCD_5x10DOTS 						0x04
#define LCD_5x8DOTS 						0x00

// flags for backlight control
#define LCD_BACKLIGHT 					0x08
#define LCD_NOBACKLIGHT 				0x00

#define PIN_RS    							(1 << 0)
#define PIN_RW    							(1 << 1)
#define PIN_EN    							(1 << 2)
#define BACKLIGHT 							(1 << 3)

/*
#define En B00000100  // Enable bit
#define Rw B00000010  // Read/Write bit
#define Rs B00000001  // Register select bit
*/


static uint8_t i2c_ch  = _DEF_I2C1;
static uint8_t i2c_dev = (0x27<<1);//  0x3C
//static void (*frameCallBack)(void) = NULL;

//static uint8_t lcd1602_buffer[LCD1602_WIDTH*LCD1602_HEIGHT];


static bool lcd1602Reset(void);
static void lcd1602WriteCmd(uint8_t cmd_data);
static bool lcd1602SendBuffer(uint8_t *p_data, uint32_t length, uint32_t timeout_ms);

static bool lcd_sendInternal(uint8_t lcd_addr, uint8_t data, uint8_t flags);
static void lcd_sendData(uint8_t data);
static void lcd1602SendString(char *str, uint8_t len);




/*static void lcd1602SetWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1);
static uint16_t ssd1306GetWidth(void);
static uint16_t ssd1306GetHeight(void);
static bool ssd1306SendBuffer(uint8_t *p_data, uint32_t length, uint32_t timeout_ms);
static bool ssd1306SetCallBack(void (*p_func)(void));
static void ssd1306Fill(uint16_t color);
static bool ssd1306UpdateDraw(void);
static void ssd1306DrawPixel(uint8_t x, uint8_t y, uint16_t color);*/


//static uint8_t lcd1602_buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];



bool lcd1602Init(void)
{
  bool ret;

  ret = lcd1602Reset();

  return ret;
}





bool lcd1602InitDriver(lcd_driver_t *p_driver)
{
	p_driver->init        = lcd1602Init;
	p_driver->reset       = lcd1602Reset;
	p_driver->setString   = lcd1602SendString;
	p_driver->setWindow   = NULL;
	p_driver->getWidth    = NULL;
	p_driver->getHeight   = NULL;
	p_driver->setCallBack = NULL;
	p_driver->sendBuffer  = lcd1602SendBuffer;
	p_driver->setCommand  = lcd1602WriteCmd;
	return true;

}

bool lcd1602Reset()
{
	bool ret;
	ret = i2cBegin(i2c_ch, 100);

	// 4-bit mode, 2 lines, 5x7 format
	lcd1602WriteCmd(0x30);
	// display & cursor home (keep this!)
	lcd1602WriteCmd(0x02);
	// display on, right shift, underline off, blink off
	lcd1602WriteCmd(0x0C);
	// clear display (optional here)
	lcd1602WriteCmd(0x01);

	return ret;
}


bool lcd1602SendBuffer(uint8_t *p_data, uint32_t length, uint32_t timeout_ms)
{

	uint16_t *p_buf = (uint16_t *)p_data;
  uint32_t idx = 0;

	for (int y=0; y<LCD1602_HEIGHT; y++)
	{
		for (int x=0; x<LCD1602_WIDTH; x++)
		{
			//if (y == 0) 	lcd1602WriteCmd(0x80);
			//else   			 	lcd1602WriteCmd(0xC0);
			if (idx<length)
			{
				lcd_sendData(p_buf[y*LCD1602_WIDTH + x]);
				idx++;
			}
		}
	}

	return true;
}


void lcd1602SendString(char *str, uint8_t len)
{
	lcd1602WriteCmd(0x01);
	HAL_Delay(LCD_DELAY_MS);
	for(int i = 0; i < len ; i++)
	{
		lcd_sendData((uint8_t)str[i]);
		HAL_Delay(LCD_DELAY_MS);
	}
//	while(*str)
//	{
//		lcd_sendData((uint8_t)(*str));
//		HAL_Delay(20);
//		str++;
//	}
}


bool lcd_sendInternal(uint8_t lcd_addr, uint8_t data, uint8_t flags)
{
	for(;;) {
		//res = HAL_I2C_IsDeviceReady(&hi2c1, lcd_addr, 1, HAL_MAX_DELAY);
		//if(res == HAL_OK)
		//	break;
		if(i2cIsDeviceReady(i2c_ch, lcd_addr) == true)
			break;
	}

	uint8_t up = data & 0xF0;
	uint8_t lo = (data << 4) & 0xF0;

	uint8_t data_arr[4];
	data_arr[0] = up|flags|BACKLIGHT|PIN_EN;
	data_arr[1] = up|flags|BACKLIGHT;
	data_arr[2] = lo|flags|BACKLIGHT|PIN_EN;
	data_arr[3] = lo|flags|BACKLIGHT;

	//res = HAL_I2C_Master_Transmit(&hi2c1, lcd_addr, data_arr, sizeof(data_arr), HAL_MAX_DELAY);

	return i2cWriteData(i2c_ch, lcd_addr, &data_arr[0],sizeof(data_arr),HAL_MAX_DELAY);
}

void lcd1602WriteCmd(uint8_t cmd_data)
{
	lcd_sendInternal(i2c_dev>>1, cmd_data, 0);
}

void lcd_sendData(uint8_t data)
{
	lcd_sendInternal(i2c_dev>>1, data, PIN_RS);
}





#endif  /*_USE_HW_LCD1602_I2C_8574*/


#if 0


#include "i2c.h"
#include "lcd1602.h"
#include <string.h>

void I2C_Scan(void)
{
	char info[] = "Scanning I2C bus...\r\n";

	printf("%s", info);

	HAL_StatusTypeDef res;
	for(uint16_t i = 0; i < 128; i++)
	{
		res = HAL_I2C_IsDeviceReady(&hi2c1, i << 1, 1, 10);
		if(res == HAL_OK)
		{
			char msg[64];

			snprintf(msg, sizeof(msg), "0x%02X", i);
			printf("%s", msg);

		} else
		{
			printf("%s", ".");
		}
	}

	printf("%s", "\r\n");
}

HAL_StatusTypeDef LCD_SendInternal(uint8_t lcd_addr, uint8_t data, uint8_t flags)
{
	HAL_StatusTypeDef res;
	for(;;) {
		res = HAL_I2C_IsDeviceReady(&hi2c1, lcd_addr, 1, HAL_MAX_DELAY);
		if(res == HAL_OK)
			break;
	}

	uint8_t up = data & 0xF0;
	uint8_t lo = (data << 4) & 0xF0;

	uint8_t data_arr[4];
	data_arr[0] = up|flags|BACKLIGHT|PIN_EN;
	data_arr[1] = up|flags|BACKLIGHT;
	data_arr[2] = lo|flags|BACKLIGHT|PIN_EN;
	data_arr[3] = lo|flags|BACKLIGHT;

	res = HAL_I2C_Master_Transmit(&hi2c1, lcd_addr, data_arr, sizeof(data_arr), HAL_MAX_DELAY);
	HAL_Delay(LCD_DELAY_MS);
	return res;
}

void LCD_SendCommand(uint8_t lcd_addr, uint8_t cmd)
{
	LCD_SendInternal(lcd_addr, cmd, 0);
}

void LCD_SendData(uint8_t lcd_addr, uint8_t data)
{
	LCD_SendInternal(lcd_addr, data, PIN_RS);
}

void LCD_Init(uint8_t lcd_addr)
{
	// 4-bit mode, 2 lines, 5x7 format
	LCD_SendCommand(lcd_addr, 0x30);
	// display & cursor home (keep this!)
	LCD_SendCommand(lcd_addr, 0x02);
	// display on, right shift, underline off, blink off
	LCD_SendCommand(lcd_addr, 0x0C);
	// clear display (optional here)
	LCD_SendCommand(lcd_addr, 0x01);
}

void LCD_SendString(uint8_t lcd_addr, char *str)
{
	while(*str)
	{
		LCD_SendData(lcd_addr, (uint8_t)(*str));
		str++;
	}
}

#endif
