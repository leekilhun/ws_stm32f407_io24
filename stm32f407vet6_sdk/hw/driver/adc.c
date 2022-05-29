/*
 * adc.c
 *
 *  Created on: Dec 10, 2021
 *      Author: gns2l
 */


#include "adc.h"


#ifdef _USE_HW_ADC


ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;


typedef struct
{
  ADC_HandleTypeDef       *p_hadc;
  DMA_HandleTypeDef       *p_dma_adc;

  uint32_t                channel;
  uint32_t                SamplingTime;

  GPIO_TypeDef            *port;
  uint32_t                pin;
  bool                    is_init;
} adc_tbl_t;

static adc_tbl_t adc_tbl[ADC_MAX_CH] =
    {
        { &hadc1, &hdma_adc1, ADC_CHANNEL_8,ADC_SAMPLETIME_480CYCLES, GPIOB, GPIO_PIN_0, false},
    };

static uint16_t adc_data[ADC_MAX_CH];


static uint8_t adcRead(uint8_t ch);

/*
static uint32_t adcRead8(uint8_t ch);
static uint32_t adcRead10(uint8_t ch);
static uint32_t adcRead12(uint8_t ch);
static uint32_t adcRead16(uint8_t ch);
 */

static uint32_t adcConvVoltage(uint8_t ch, uint32_t adc_value);
static uint32_t adcConvCurrent(uint8_t ch, uint32_t adc_value);


#ifdef _USE_HW_CLI
#include "cli.h"
static void cliAdc(void);
#endif

static bool adcOpen(uint8_t ch);


bool adcInit(void)
{
  uint32_t i;


  for (i=0; i<ADC_MAX_CH; i++)
  {

    adcOpen(i);

  }

#if _USE_HW_CLI
  cliAdd("adc", cliAdc);
#endif

  return true;
}

bool adcOpen(uint8_t ch)
{
  ADC_ChannelConfTypeDef sConfig      = {0};
  ADC_HandleTypeDef *p_handle         = adc_tbl[ch].p_hadc;

  switch(ch)
  {
    case _DEF_ADC1:                       // BAT_ADC;

      /* DMA controller clock enable */
      __HAL_RCC_DMA2_CLK_ENABLE();
      /* DMA interrupt init */
      /* DMA2_Stream0_IRQn interrupt configuration */
      HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
      HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);


      p_handle->Instance                    = ADC1;
      p_handle->Init.ClockPrescaler         = ADC_CLOCK_SYNC_PCLK_DIV8;
      p_handle->Init.Resolution             = ADC_RESOLUTION_12B;
      p_handle->Init.ScanConvMode           = DISABLE;
      p_handle->Init.ContinuousConvMode     = ENABLE;
      p_handle->Init.DiscontinuousConvMode  = DISABLE;
      p_handle->Init.ExternalTrigConvEdge   = ADC_EXTERNALTRIGCONVEDGE_NONE;
      p_handle->Init.ExternalTrigConv       = ADC_SOFTWARE_START;
      p_handle->Init.DataAlign              = ADC_DATAALIGN_RIGHT;
      p_handle->Init.NbrOfConversion        = 1;
      p_handle->Init.DMAContinuousRequests  = ENABLE;
      p_handle->Init.EOCSelection           = ADC_EOC_SINGLE_CONV;

      if (HAL_ADC_Init(p_handle) != HAL_OK)
      {
        Error_Handler();
      }
      /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
       */
      sConfig.Channel                       = adc_tbl[ch].channel;
      sConfig.Rank                          = 1;
      sConfig.SamplingTime                  = adc_tbl[ch].SamplingTime;
      if (HAL_ADC_ConfigChannel(p_handle, &sConfig) != HAL_OK)
      {
        Error_Handler();
      }

      adc_tbl[ch].is_init = true;

      break;

    case _DEF_ADC2:
      break;
  }

  return true;
}


bool adcStartDma(uint8_t ch, uint32_t* pData, uint32_t len)
{
  if (HAL_ADC_Start_DMA(adc_tbl[ch].p_hadc, pData, len) == HAL_OK)
  {
    return true;
  }

  return false;
}

uint8_t adcRead(uint8_t ch)
{
  uint32_t adc_value;

  if (adc_tbl[ch].is_init != true)
  {
    return 0;
  }

  adc_value = adc_data[ch];

  return adc_value;
}

/*
uint32_t adcRead8(uint8_t ch)
{
  return adcRead(ch)>>4;
}
uint32_t adcRead10(uint8_t ch)
{
  return adcRead(ch)>>2;
}
uint32_t adcRead12(uint8_t ch)
{
  return adcRead(ch);
}
uint32_t adcRead16(uint8_t ch)
{
  return adcRead(ch)<<4;
}
 */

uint32_t adcReadVoltage(uint8_t ch)
{
  return adcConvVoltage(ch, adcRead(ch));
}

uint32_t adcReadCurrent(uint8_t ch)
{

  return adcConvCurrent(ch, adcRead(ch));
}

uint32_t adcConvVoltage(uint8_t ch, uint32_t adc_value)
{
  uint32_t ret = 0;

  switch(ch)
  {
    case 0:
    case 1:
      ret  = (uint32_t)((adc_value * 3300 * 10) / (4095*10));
      ret += 5;
      ret /= 10;
      break;

    case 2:
      ret  = (uint32_t)((adc_value * 3510 * 26) / (4095*10));
      ret += 5;
      ret /= 10;
      break;

  }

  return ret;
}

uint32_t adcConvCurrent(uint8_t ch, uint32_t adc_value)
{
  return 0;
}



void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if(adcHandle->Instance==ADC1)
  {
    /* ADC1 clock enable */
    __HAL_RCC_ADC1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**ADC1 GPIO Configuration
    PB0     ------> ADC1_IN8
     */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* ADC1 DMA Init */
    /* ADC1 Init */
    hdma_adc1.Instance = DMA2_Stream0;
    hdma_adc1.Init.Channel = DMA_CHANNEL_0;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_DISABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_CIRCULAR;
    hdma_adc1.Init.Priority = DMA_PRIORITY_LOW;
    hdma_adc1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_adc1) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(adcHandle,DMA_Handle,hdma_adc1);

  }
}




void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{

  if(adcHandle->Instance==ADC1)
  {
    /* Peripheral clock disable */
    __HAL_RCC_ADC1_CLK_DISABLE();

    /**ADC1 GPIO Configuration
    PB0     ------> ADC1_IN8
     */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0);

    /* ADC1 DMA DeInit */
    HAL_DMA_DeInit(adcHandle->DMA_Handle);
  }
}





#endif /*_USE_HW_ADC*/
