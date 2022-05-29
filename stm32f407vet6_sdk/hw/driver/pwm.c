/*
 * pwm.c
 *
 *  Created on: Jan 2, 2022
 *      Author: gns2l
 */




#include "pwm.h"
#include "cli.h"

#ifdef _USE_HW_PWM


static bool is_init = false;


typedef struct
{
  TIM_HandleTypeDef  TimHandle;
  TIM_OC_InitTypeDef sConfig;
  uint32_t           channel;
} pwm_tbl_t;


pwm_tbl_t  pwm_tbl[HW_PWM_MAX_CH];


#ifdef _USE_HW_CLI
static void cliPwm(cli_args_t *args);
#endif

bool pwmInit(void)
{
  bool ret = true;
  pwm_tbl_t *p_pwm;
  uint32_t prescaler;
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  prescaler = (uint32_t)((SystemCoreClock /1) / (10000 * 255)) - 1;

  __HAL_RCC_TIM14_CLK_ENABLE();

  /****  MspPostInit  ****/
  __HAL_RCC_GPIOA_CLK_ENABLE();
  /**TIM14 GPIO Configuration
      PA7     ------> TIM14_CH1

   */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF4_TIM14;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


  p_pwm = &pwm_tbl[0];

  p_pwm->TimHandle.Instance               = TIM14;
  p_pwm->TimHandle.Init.Prescaler         = prescaler;
  p_pwm->TimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
  p_pwm->TimHandle.Init.Period            = 255-1;
  p_pwm->TimHandle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  p_pwm->TimHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&p_pwm->TimHandle) != HAL_OK)
  {
    ret = false;
  }
  sClockSourceConfig.ClockSource          = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&p_pwm->TimHandle, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_PWM_Init(&p_pwm->TimHandle) != HAL_OK)
  {
    ret = false;
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&p_pwm->TimHandle, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  p_pwm->sConfig.OCMode     = TIM_OCMODE_PWM1;
  p_pwm->sConfig.Pulse      = 128;
  p_pwm->sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
  p_pwm->sConfig.OCFastMode = TIM_OCFAST_ENABLE;

  if (HAL_TIM_PWM_ConfigChannel(&p_pwm->TimHandle, &p_pwm->sConfig, p_pwm->channel) != HAL_OK)
  {
    ret = false;
  }

  HAL_TIM_PWM_Start(&p_pwm->TimHandle, p_pwm->channel);


  is_init = ret;


#ifdef _USE_HW_CLI
cliAdd("pwm", cliPwm);
#endif
return ret;
}

bool pwmIsInit(void)
{
  return is_init;
}


void pwmWrite(uint8_t ch, uint16_t pwm_data)
{
  if (ch >= PWM_MAX_CH) return;

  switch(ch)
  {
    case _DEF_PWM1:
      pwm_tbl[ch].sConfig.Pulse = pwm_data;
      pwm_tbl[ch].TimHandle.Instance->CCR3 = pwm_data;
      break;
  }
}

uint16_t pwmRead(uint8_t ch)
{
  uint16_t ret = 0;

  if (ch >= HW_PWM_MAX_CH) return 0;


  switch(ch)
  {
    case _DEF_PWM1:
      ret = pwm_tbl[ch].sConfig.Pulse;
      break;
  }

  return ret;
}

uint16_t pwmGetMax(uint8_t ch)
{
  uint16_t ret = 255;

  if (ch >= HW_PWM_MAX_CH) return 255;


  switch(ch)
  {
    case _DEF_PWM1:
      ret = 255;
      break;
  }

  return ret;
}



#ifdef _USE_HW_CLI
void cliPwm(cli_args_t *args)
{
  bool ret = true;
  uint8_t  ch;
  uint32_t pwm;


  if (args->argc == 3)
  {
    ch  = (uint8_t)args->getData(1);
    pwm = (uint8_t)args->getData(2);

    ch = constrain(ch, 0, PWM_MAX_CH);

    if(args->isStr(0, "set"))
    {
      pwmWrite(ch, pwm);
      cliPrintf("pwm ch%d %d\n", ch, pwm);
    }
    else if(args->isStr(0, "test"))
    {
      uint32_t line = 0;
      uint8_t step = 0;
      uint8_t is_increase = 0;
      while(cliKeepLoop())
      {
        // increase
        if (is_increase)
        {
          if (step <255)
          {
            step++;
          }
          else
          {
            step--;
            is_increase = false;
          }
        }
        else
        {
          if (step >1)
          {
            step--;
          }
          else
          {
            step++;
            is_increase = true;
          }
        }

        pwmWrite(ch, step);
        cliPrintf("[%d] pwm set %03d ",line, step);
        cliPrintf("\r\n");
        if (line >3)
        {
          cliPrintf("\x1B[%dA", 5);
          line = 0;
        }
        else
        {
          line++;
        }
        delay(pwm);

      }
    }
    else
    {
      ret = false;
    }
  }
  else if (args->argc == 2)
  {
    ch = (uint8_t)args->getData(1);

    if(args->isStr(0, "get"))
    {
      cliPrintf("pwm ch%d %d\n", ch, pwmRead(ch));
    }
    else
    {
      ret = false;
    }
  }
  else
  {
    ret = false;
  }


  if (ret == false)
  {
    cliPrintf( "pwm test 0~%d dealy_ms \n", PWM_MAX_CH-1);
    cliPrintf( "pwm set 0~%d 0~255 \n", PWM_MAX_CH-1);
    cliPrintf( "pwm get 0~%d \n", PWM_MAX_CH-1);
  }

}
#endif

#endif
