/*
 * buzzer.c
 *
 *  Created on: Dec 28, 2021
 *      Author: gns2.lee
 */


#include "tickTimer.h"
#include "buzzer.h"

#ifdef _USE_HW_BUZZER


#define DEF_BUZZER_PORT     GPIOC
#define DEF_BUZZER_PIN      GPIO_PIN_2
static bool is_buzzerOn;


bool buzzerInit(void)
{
  GPIO_InitTypeDef   GPIO_InitStructure = {0,};
  __HAL_RCC_GPIOC_CLK_ENABLE();

  GPIO_InitStructure.Pin = DEF_BUZZER_PIN;
  GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Pull  = GPIO_NOPULL;
  GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(DEF_BUZZER_PORT, &GPIO_InitStructure);

  is_buzzerOn = false;

  buzzerOff();

  return true;
}

void buzzerOn(void)
{
  HAL_GPIO_WritePin(DEF_BUZZER_PORT, DEF_BUZZER_PIN, GPIO_PIN_SET);
  is_buzzerOn = true;
}


void buzzerOff(void)
{
  HAL_GPIO_WritePin(DEF_BUZZER_PORT, DEF_BUZZER_PIN, GPIO_PIN_RESET);
  is_buzzerOn = false;
}

void buzzerToggle(void)
{
  HAL_GPIO_TogglePin(DEF_BUZZER_PORT, DEF_BUZZER_PIN);
}

void buzzerBeep(uint8_t times, uint32_t delay_ms)
{
  if (is_buzzerOn)
    buzzerOff();

  while(times>0)
  {
    buzzerToggle();
    delay(delay_ms);
    buzzerToggle();
    delay(delay_ms);
    times--;
  }
  buzzerOff();
}




#endif /*_USE_HW_BUZZ*/
