/*
 * hw.c
 *
 *  Created on: May 6, 2022
 *      Author: gns2l
 */



#include "hw.h"

bool hwInit(void)
{
  bool ret = true;

#ifdef _USE_HW_CLI
  ret &= cliInit();
#endif

#ifdef _USE_HW_LOG
  ret &= logInit();
#endif


  logPrintf("[ Firmware Begin... ]\r\n");
  logPrintf("Booting..Name \t\t: %s\r\n", _DEF_BOARD_NAME);
  logPrintf("Booting..Ver  \t\t: %s\r\n", _DEF_FIRMWATRE_VERSION);


#ifdef _USE_HW_UART
  ret &= uartInit();
#endif

#ifdef _USE_HW_LED
  ret &= ledInit();
#endif



#ifdef _USE_HW_BUZZER
  ret &= buzzerInit();
#endif

#ifdef _USE_HW_LED
  ret &= ledInit();
#endif

#ifdef _USE_HW_FLASH
  ret &= flashInit();
#endif

#ifdef _USE_HW_GPIO
  ret &= gpioInit();
#endif


#ifdef _USE_HW_SPI
  ret &= spiInit();
#endif

#ifdef _USE_HW_I2C
  ret &= i2cInit();
#endif


#ifdef _USE_HW_CAN
  ret &= canInit();
#endif

  logBoot(false);


  return ret;
}

