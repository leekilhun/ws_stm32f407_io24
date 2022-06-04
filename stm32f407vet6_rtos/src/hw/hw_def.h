/*
 * hw_def.h
 *
 *  Created on: May 6, 2022
 *      Author: gns2l
 */

#ifndef SRC_HW_HW_DEF_H_
#define SRC_HW_HW_DEF_H_


#include "bsp.h"

#define _USE_HW_LED
#define      HW_LED_MAX_CH          1

#define _USE_HW_UART
#define      HW_UART_MAX_CH         2

#define _USE_HW_CDC
#define _USE_HW_USB
#define      HW_USE_CDC             1
#define      HW_USE_MSC             0

#define _USE_HW_CLI
#define      HW_CLI_CMD_LIST_MAX    16
#define      HW_CLI_CMD_NAME_MAX    16
#define      HW_CLI_LINE_HIS_MAX    4
#define      HW_CLI_LINE_BUF_MAX    64


#define _HW_DEF_RTOS_MEM_SIZE(x)              ((x)/4)

#define _HW_DEF_RTOS_THREAD_PRI_MAIN          osPriorityNormal//osPriorityRealtime
#define _HW_DEF_RTOS_THREAD_PRI_FM_CMD        osPriorityNormal
#define _HW_DEF_RTOS_THREAD_PRI_LCD_CMD       osPriorityNormal
#define _HW_DEF_RTOS_THREAD_PRI_PC_CMD        osPriorityNormal
#define _HW_DEF_RTOS_THREAD_PRI_EVENT         osPriorityNormal//osPriorityHigh


#define _HW_DEF_RTOS_THREAD_MEM_MAIN          _HW_DEF_RTOS_MEM_SIZE( 1*1024)
#define _HW_DEF_RTOS_THREAD_MEM_LCD_CMD       _HW_DEF_RTOS_MEM_SIZE( 1*1024)
#define _HW_DEF_RTOS_THREAD_MEM_PC_CMD        _HW_DEF_RTOS_MEM_SIZE( 1*1024)
#define _HW_DEF_RTOS_THREAD_MEM_FM_CMD        _HW_DEF_RTOS_MEM_SIZE( 1*1024)
#define _HW_DEF_RTOS_THREAD_MEM_EVENT         _HW_DEF_RTOS_MEM_SIZE( 1*1024)

#define _USE_HW_FLASH

//#define _USE_HW_BUZZER

//#define _USE_HW_GPIO
#define      HW_GPIO_MAX_CH         1


/*simple logPrintf func*/
//#define _USE_HW_SLOG
//#define      HW_SLOG_CH              _DEF_UART2
#define _USE_HW_LOG
#define      HW_LOG_CH              _DEF_UART2
#define      HW_LOG_BOOT_BUF_MAX    1024
#define      HW_LOG_LIST_BUF_MAX    2048


//#define _USE_HW_I2C
#define      HW_I2C_MAX_CH          2

//#define _USE_HW_SPI
#define      HW_SPI_MAX_CH          1

//#define _USE_HW_CMD_ROBOTRO
//#define _USE_HW_CMD_FASTECH
//#define _USE_HW_CMD_NEXTION


#define _DEF_FIRMWATRE_VERSION    "V220509R1"
#define _DEF_BOARD_NAME           "STM32F407"




#endif /* SRC_HW_HW_DEF_H_ */
