/*
 * ap.cpp
 *
 *  Created on: May 6, 2022
 *      Author: gns2l
 */


#include "ap.h"

static void threadEvent(void const *argument);

void apInit(void)
{
  /* rtos initial*/
  {
    /**/
    osThreadDef(threadEvent, threadEvent, _HW_DEF_RTOS_THREAD_PRI_EVENT, 0, _HW_DEF_RTOS_THREAD_MEM_EVENT);
    if (osThreadCreate(osThread(threadEvent), NULL) != NULL)
    {
      logPrintf("threadEvent \t\t: OK\r\n");
    }
    else
    {
      logPrintf("threadEvent \t\t: Fail\r\n");
    }
  }

  cliOpen(_DEF_UART1, 115200);   // USB
  uartOpen(_DEF_UART2, 115200);

}

/*######################################################
   Main task
  ######################################################*/
void apMain(void)
{

  uint32_t pre_main_ms = millis();
  uint32_t pre_uart_ms = 0;
  uint32_t exe_uart_ms = 0;
  while (1)
  {

    if (millis() - pre_main_ms >= 1000)
    {
      ledToggle(_DEF_LED1);
      pre_main_ms = millis();
      //uartPrintf(_DEF_UART1, "test:%d \n\r",pre_main_ms);
    }
    if (uartAvailable(_DEF_UART2) > 0)
    {
      uint8_t rx_data;
      pre_uart_ms = millis();
      rx_data = uartRead(_DEF_UART2);
      uartPrintf(_DEF_UART2, "Uart1 Rx %c %X,  %d ms\n ", rx_data, rx_data,exe_uart_ms);
      exe_uart_ms = millis() - pre_uart_ms;
    }

    delay(1);
  }
}

/*######################################################
  Event Task  -> cnProcess step
  ######################################################*/
void threadEvent(void const *argument)
{
  UNUSED(argument);
  while (1)
  {
    cliMain();
    delay(1);
  }
}


