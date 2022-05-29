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

}

/*######################################################
   Main task
  ######################################################*/
void apMain(void)
{

  uint32_t pre_main_ms = millis();
  while (1)
  {

    if (millis() - pre_main_ms >= 1000)
    {
      ledToggle(_DEF_LED1);
      pre_main_ms = millis();
      //uartPrintf(_DEF_UART1, "test:%d \n\r",pre_main_ms);
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


