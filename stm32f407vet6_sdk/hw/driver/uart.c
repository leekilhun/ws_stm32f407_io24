/*
 * uart.c
 *
 *  Created on: Jul 10, 2021
 *      Author: gns2l
 */



#include "uart.h"

#ifdef _USE_HW_UART

#include "qbuffer.h"
#include "cdc.h"


#define UART_BUF_LENGTH      128

#ifdef _USE_HW_UART_DMA_1
UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;
static uint8_t  rx_data[UART_BUF_LENGTH];  //for interrupt receive data
static uint8_t  tx_data[UART_BUF_LENGTH];
#endif

typedef struct
{
  bool     is_open;
  uint32_t baud;
} uart_tbl_t;



static qbuffer_t qbuffer[UART_MAX_CH];
static uart_tbl_t uart_tbl[UART_MAX_CH];

bool uartInit(void)
{
  for (int i=0; i<UART_MAX_CH; i++)
  {
    uart_tbl[i].is_open = false;
    uart_tbl[i].baud = 115200;
  }

  return true;
}

bool uartOpen(uint8_t ch, uint32_t baud)
{
  bool ret = false;

  switch(ch)
  {
    case _DEF_UART1:
      uart_tbl[ch].is_open = true;
      ret = true;
      break;

    case _DEF_UART2:
    {
      huart1.Instance                 = USART1;
      huart1.Init.BaudRate            = 115200;
      huart1.Init.WordLength          = UART_WORDLENGTH_8B;
      huart1.Init.StopBits            = UART_STOPBITS_1;
      huart1.Init.Parity              = UART_PARITY_NONE;
      huart1.Init.Mode                = UART_MODE_TX_RX;
      huart1.Init.HwFlowCtl           = UART_HWCONTROL_NONE;
      huart1.Init.OverSampling        = UART_OVERSAMPLING_16;

      HAL_UART_DeInit(&huart1);

      qbufferCreate(&qbuffer[ch], &rx_data[0], UART_BUF_LENGTH);

#ifdef _USE_HW_UART_DMA_1
      __HAL_RCC_DMA2_CLK_ENABLE();

      /* DMA interrupt init */
      /* DMA2_Stream2_IRQn interrupt configuration */
      HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 5, 0);
      HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
      /* DMA2_Stream7_IRQn interrupt configuration */
      HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 5, 0);
      HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
#endif

      if (HAL_UART_Init(&huart1) != HAL_OK)
      {
        ret = false;
      }
      else
      {
        ret = true;
        uart_tbl[ch].is_open = true;
#ifdef _USE_HW_UART_DMA_1
        if(HAL_UART_Receive_DMA(&huart1, (uint8_t *)&rx_data[0], UART_BUF_LENGTH) != HAL_OK)
        {
          ret = false;
        }

        qbuffer[ch].in  = qbuffer[ch].len - hdma_usart1_rx.Instance->NDTR;
        qbuffer[ch].out = qbuffer[ch].in;
#else
        if (HAL_UART_Receive_IT(&huart1, (uint8_t *)&rx_data[ch], 1) != HAL_OK)
        {
          ret = false;
        }
#endif
      }
    }
    break;

    case _DEF_UART3:
      break;

    case _DEF_UART4:
      break;
    }

    return ret;
  }

  bool uartClose(uint8_t ch)
  {
    uart_tbl[ch].is_open = false;
    return true;
  }

  uint32_t uartAvailable(uint8_t ch)
  {
    uint32_t ret = 0;

    switch(ch)
    {
      case _DEF_UART1:
        ret = cdcAvailable();
        break;

      case _DEF_UART2:
#ifdef _USE_HW_UART_DMA_1
      qbuffer[ch].in = (qbuffer[ch].len - hdma_usart1_rx.Instance->NDTR);
      ret = qbufferAvailable(&qbuffer[ch]);
      break;
#endif
      case _DEF_UART3:
      case _DEF_UART4:
        ret = qbufferAvailable(&qbuffer[ch]);
        break;
      default:
        break;
    }
    return ret;
  }


  bool uartFlush(uint8_t ch)
  {
    uint32_t pre_time;

    pre_time = millis();
    while(uartAvailable(ch))
    {
      if (millis()-pre_time >= 10)
      {
        break;
      }
      uartRead(ch);
    }

    return true;
  }

  uint8_t uartRead(uint8_t ch)
  {
    uint8_t ret = 0;

    switch(ch)
    {
      case _DEF_UART1:
        ret = cdcRead();
        break;

      case _DEF_UART2:
      case _DEF_UART3:
      case _DEF_UART4:
        qbufferRead(&qbuffer[ch], &ret, 1);
        break;

      default:
        break;
    }

    return ret;
  }

  uint32_t uartWrite(uint8_t ch, uint8_t *p_data, uint32_t length)
  {
    uint32_t ret = 0;
    HAL_StatusTypeDef status;
    switch(ch)
    {
      case _DEF_UART1:
        ret = cdcWrite(p_data, length);
        break;

      case _DEF_UART2:
      {
#ifdef _USE_HW_UART_DMA_1
        uint32_t pre_time;
        pre_time = millis();
        while(huart1.gState != HAL_UART_STATE_READY)
        {
          if (millis()-pre_time >= 100)
          {
            break;
          }
        }
        if (huart1.gState == HAL_UART_STATE_READY)
        {
          memset(&tx_data[0],0,UART_BUF_LENGTH);
          memcpy(&tx_data[0],p_data,length);
          status = HAL_UART_Transmit_DMA(&huart1, &tx_data[0], length);
        }
#else
        status = HAL_UART_Transmit(&huart1, p_data, length, 100);
#endif
        if (status == HAL_OK)
          ret = length;
      }
      break;
      case _DEF_UART3:
        break;
      case _DEF_UART4:
        break;
      default:
        break;
    }

    return ret;
  }

  uint32_t uartPrintf(uint8_t ch, const char *fmt, ...)
  {
    char buf[256];
    va_list args;
    int len;
    uint32_t ret;

    va_start(args, fmt);
    len = vsnprintf(buf, 256, fmt, args);
    ret = uartWrite(ch, (uint8_t *)buf, len);
    va_end(args);

    return ret;
  }

  uint32_t uartGetBaud(uint8_t ch)
  {
    uint32_t ret = 0;

    switch(ch)
    {
      case _DEF_UART1:
        ret = cdcGetBaud();
        break;

      case _DEF_UART2:
        break;
      case _DEF_UART3:
        break;
      case _DEF_UART4:
        break;
      default:
        break;
    }

    return ret;
  }

  void USART1_IRQHandler(void)
  {
    HAL_UART_IRQHandler(&huart1);
  }

#ifdef _USE_HW_UART_DMA_1
  void DMA2_Stream2_IRQHandler(void)
  {
    HAL_DMA_IRQHandler(&hdma_usart1_rx);
  }

  void DMA2_Stream7_IRQHandler(void)
  {
    HAL_DMA_IRQHandler(&hdma_usart1_tx);
  }
#endif

  void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
  {
    if (huart->Instance == USART1)
    {
    }
  }


  void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
  {

    if (huart->Instance == USART1)
    {
#ifdef _USE_HW_UART_DMA_1
#else
      qbufferWrite(&qbuffer[_DEF_UART2], &rx_data[_DEF_UART2], 1);
      HAL_UART_Receive_IT(&huart1, (uint8_t *)&rx_data[_DEF_UART2], 1);
#endif
    }
  }


  void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
  {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
      if(uartHandle->Instance==USART1)
      {
        /* USART1 clock enable */
        __HAL_RCC_USART1_CLK_ENABLE();

        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**USART1 GPIO Configuration
        PA9     ------> USART1_TX
        PA10     ------> USART1_RX
        */
        GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* USART1 DMA Init */
        /* USART1_RX Init */
        hdma_usart1_rx.Instance = DMA2_Stream2;
        hdma_usart1_rx.Init.Channel = DMA_CHANNEL_4;
        hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart1_rx.Init.Mode = DMA_CIRCULAR;
        hdma_usart1_rx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_usart1_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK)
        {
          Error_Handler();
        }

        __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart1_rx);

        /* USART1_TX Init */
        hdma_usart1_tx.Instance = DMA2_Stream7;
        hdma_usart1_tx.Init.Channel = DMA_CHANNEL_4;
        hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart1_tx.Init.Mode = DMA_NORMAL;
        hdma_usart1_tx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_usart1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
        {
          Error_Handler();
        }

        __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart1_tx);

        /* USART1 interrupt Init */
        HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
      }

  }

  void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
  {
    if(uartHandle->Instance==USART1)
      {
        /* Peripheral clock disable */
        __HAL_RCC_USART1_CLK_DISABLE();

        /**USART1 GPIO Configuration
        PA9     ------> USART1_TX
        PA10     ------> USART1_RX
        */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

        /* USART1 DMA DeInit */
        HAL_DMA_DeInit(uartHandle->hdmarx);
        HAL_DMA_DeInit(uartHandle->hdmatx);

        /* USART1 interrupt Deinit */
        HAL_NVIC_DisableIRQ(USART1_IRQn);
      }

  }





#endif
