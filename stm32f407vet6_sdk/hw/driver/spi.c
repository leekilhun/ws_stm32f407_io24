/*
 * spi.c
 *
 *  Created on: Nov 27, 2021
 *      Author: gns2l
 */




#include "spi.h"


#ifdef _USE_HW_SPI


#define SPI_RX_BUFF_SIZE   32

typedef struct
{
  bool is_open;
  bool is_tx_done;
  bool is_rx_done;
  bool is_error;

  void (*func_tx)(void);
  void (*func_rx)(void);

  SPI_HandleTypeDef *h_spi;
  DMA_HandleTypeDef *h_dma_tx;
  DMA_HandleTypeDef *h_dma_rx;
} spi_t;

/* Buffer used for reception */
uint8_t aRxBuffer[SPI_RX_BUFF_SIZE];

spi_t spi_tbl[SPI_MAX_CH];

SPI_HandleTypeDef hspi1; // w25q16
DMA_HandleTypeDef hdma_spi1_rx;
DMA_HandleTypeDef hdma_spi1_tx;

bool spiInit(void)
{
  bool ret = true;


  for (int i=0; i<SPI_MAX_CH; i++)
  {
    spi_tbl[i].is_open = false;
    spi_tbl[i].is_tx_done = true;
    spi_tbl[i].is_rx_done = true;
    spi_tbl[i].is_error = false;
    spi_tbl[i].func_tx = NULL;
    spi_tbl[i].func_rx = NULL;
    spi_tbl[i].h_dma_rx = NULL;
    spi_tbl[i].h_dma_tx = NULL;
  }

  return ret;
}


bool spiBegin(uint8_t ch)
{
  bool ret = false;
  spi_t *p_spi = &spi_tbl[ch];

  switch(ch)
  {
    case _DEF_SPI1:  //
      p_spi->h_spi = &hspi1;
      p_spi->h_dma_rx = &hdma_spi1_rx;
      p_spi->h_dma_tx = &hdma_spi1_tx;

      p_spi->h_spi->Instance              = SPI1;
      p_spi->h_spi->Init.Mode             = SPI_MODE_MASTER;
      p_spi->h_spi->Init.Direction        = SPI_DIRECTION_2LINES;
      p_spi->h_spi->Init.DataSize         = SPI_DATASIZE_8BIT;
      p_spi->h_spi->Init.CLKPolarity      = SPI_POLARITY_LOW;
      p_spi->h_spi->Init.CLKPhase         = SPI_PHASE_1EDGE;
      p_spi->h_spi->Init.NSS              = SPI_NSS_HARD_OUTPUT;//SPI_NSS_SOFT;
      p_spi->h_spi->Init.BaudRatePrescaler= SPI_BAUDRATEPRESCALER_8;
      p_spi->h_spi->Init.FirstBit         = SPI_FIRSTBIT_MSB;
      p_spi->h_spi->Init.TIMode           = SPI_TIMODE_DISABLE;
      p_spi->h_spi->Init.CRCCalculation   = SPI_CRCCALCULATION_DISABLE;
      p_spi->h_spi->Init.CRCPolynomial    = 7;
      p_spi->h_spi->Init.CRCLength        = SPI_CRC_LENGTH_DATASIZE;
      p_spi->h_spi->Init.NSSPMode         = SPI_NSS_PULSE_ENABLE;

      HAL_SPI_DeInit(p_spi->h_spi);

      if (HAL_SPI_Init(p_spi->h_spi) == HAL_OK)
      {
        ret = true;
      }

      break;

    case _DEF_SPI2:   //

      break;
    case _DEF_SPI3:   //

      break;
  }

  if (ret)
  {
    p_spi->is_open = true;
  }
  return ret;
}

bool spiReset(uint8_t ch)
{
  bool ret = false;
  spi_t *p_spi = &spi_tbl[ch];

  HAL_SPI_DeInit(p_spi->h_spi);
  if (HAL_SPI_Init(p_spi->h_spi) == HAL_OK)
  {
    ret = true;
    p_spi->is_open = ret;
  }

  return ret;
}

bool spiIsBegin(uint8_t ch)
{
  return spi_tbl[ch].is_open;
}


void spiSetDataMode(uint8_t ch, uint8_t dataMode)
{
  spi_t  *p_spi = &spi_tbl[ch];


  if (p_spi->is_open == false) return;


  switch( dataMode )
  {
    // CPOL=0, CPHA=0
    case SPI_MODE0:
      p_spi->h_spi->Init.CLKPolarity = SPI_POLARITY_LOW;
      p_spi->h_spi->Init.CLKPhase    = SPI_PHASE_1EDGE;
      HAL_SPI_Init(p_spi->h_spi);
      break;

      // CPOL=0, CPHA=1
    case SPI_MODE1:
      p_spi->h_spi->Init.CLKPolarity = SPI_POLARITY_LOW;
      p_spi->h_spi->Init.CLKPhase    = SPI_PHASE_2EDGE;
      HAL_SPI_Init(p_spi->h_spi);
      break;

      // CPOL=1, CPHA=0
    case SPI_MODE2:
      p_spi->h_spi->Init.CLKPolarity = SPI_POLARITY_HIGH;
      p_spi->h_spi->Init.CLKPhase    = SPI_PHASE_1EDGE;
      HAL_SPI_Init(p_spi->h_spi);
      break;

      // CPOL=1, CPHA=1
    case SPI_MODE3:
      p_spi->h_spi->Init.CLKPolarity = SPI_POLARITY_HIGH;
      p_spi->h_spi->Init.CLKPhase    = SPI_PHASE_2EDGE;
      HAL_SPI_Init(p_spi->h_spi);
      break;
  }
}

void spiSetBitWidth(uint8_t ch, uint8_t bit_width)
{
  spi_t  *p_spi = &spi_tbl[ch];

  if (p_spi->is_open == false) return;

  p_spi->h_spi->Init.DataSize = SPI_DATASIZE_8BIT;

  if (bit_width == 16)
  {
    p_spi->h_spi->Init.DataSize = SPI_DATASIZE_16BIT;
  }
  HAL_SPI_Init(p_spi->h_spi);
}


void spiSetClockDivider(uint8_t spi_ch, uint8_t clockDiv)
{
  spi_t  *p_spi = &spi_tbl[spi_ch];
  uint32_t div_value;

  if (p_spi->is_open == false) return;

  switch(clockDiv)
  {
    case SPI_CLOCK_DIV_1:
      div_value = SPI_BAUDRATEPRESCALER_4;
      break;

    case SPI_CLOCK_DIV_2:
      div_value = SPI_BAUDRATEPRESCALER_8;
      break;

    case SPI_CLOCK_DIV_4:
      div_value = SPI_BAUDRATEPRESCALER_16;
      break;

    case SPI_CLOCK_DIV_8:
      div_value = SPI_BAUDRATEPRESCALER_32;
      break;

    case SPI_CLOCK_DIV_16:
      div_value = SPI_BAUDRATEPRESCALER_64;
      break;

    default:
      div_value = SPI_BAUDRATEPRESCALER_4;
      break;
  }
  p_spi->h_spi->Init.BaudRatePrescaler = div_value;

  HAL_SPI_Init(p_spi->h_spi);
}




uint8_t spiTransfer8(uint8_t ch, uint8_t data)
{
  uint8_t ret;
  spi_t  *p_spi = &spi_tbl[ch];


  if (p_spi->is_open == false) return 0;

  HAL_SPI_TransmitReceive(p_spi->h_spi, &data, &ret, 1, 10);

  return ret;
}

uint16_t spiTransfer16(uint8_t ch, uint16_t data)
{
  uint8_t tBuf[2];
  uint8_t rBuf[2];
  uint16_t ret;
  spi_t  *p_spi = &spi_tbl[ch];


  if (p_spi->is_open == false) return 0;

  if (p_spi->h_spi->Init.DataSize == SPI_DATASIZE_8BIT)
  {
    tBuf[1] = (uint8_t)data;
    tBuf[0] = (uint8_t)(data>>8);
    HAL_SPI_TransmitReceive(p_spi->h_spi, (uint8_t *)&tBuf, (uint8_t *)&rBuf, 2, 10);

    ret = rBuf[0];
    ret <<= 8;
    ret += rBuf[1];
  }
  else
  {
    HAL_SPI_TransmitReceive(p_spi->h_spi, (uint8_t *)&data, (uint8_t *)&ret, 1, 10);
  }

  return ret;
}

bool spiTransfer(uint8_t ch, uint8_t *tx_buf, uint8_t *rx_buf, uint32_t length, uint32_t timeout)
{
  bool ret = true;
  HAL_StatusTypeDef status;
  spi_t  *p_spi = &spi_tbl[ch];

  if (p_spi->is_open == false) return false;

  if (rx_buf == NULL)
  {
    status =  HAL_SPI_Transmit(p_spi->h_spi, tx_buf, length, timeout);
  }
  else if (tx_buf == NULL)
  {
    status =  HAL_SPI_Receive(p_spi->h_spi, rx_buf, length, timeout);
  }
  else
  {
    status =  HAL_SPI_TransmitReceive(p_spi->h_spi, tx_buf, rx_buf, length, timeout);
  }

  if (status != HAL_OK)
  {
    return false;
  }

  return ret;
}

void spiDmaTxStart(uint8_t spi_ch, uint8_t *p_buf, uint32_t length)
{
  spi_t  *p_spi = &spi_tbl[spi_ch];

  if (p_spi->is_open == false) return;

  p_spi->is_tx_done = false;
  HAL_SPI_Transmit_DMA(p_spi->h_spi, p_buf, length);
}

bool spiDmaTxTransfer(uint8_t ch, void *buf, uint32_t length, uint32_t timeout)
{
  bool ret = true;
  uint32_t t_time;


  spiDmaTxStart(ch, (uint8_t *)buf, length);

  t_time = millis();

  if (timeout == 0) return true;

  while(1)
  {
    if(spiDmaTxIsDone(ch))
    {
      break;
    }
    if((millis()-t_time) > timeout)
    {
      ret = false;
      break;
    }
  }

  return ret;
}

bool spiDmaTxIsDone(uint8_t ch)
{
  spi_t  *p_spi = &spi_tbl[ch];

  if (p_spi->is_open == false)     return true;

  return p_spi->is_tx_done;
}

void spiAttachTxInterrupt(uint8_t ch, void (*func)())
{
  spi_t  *p_spi = &spi_tbl[ch];


  if (p_spi->is_open == false)     return;

  p_spi->func_tx = func;
}



void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == spi_tbl[_DEF_SPI1].h_spi->Instance)
  {
    spi_tbl[_DEF_SPI1].is_error = true;
  }
  else if (hspi->Instance == spi_tbl[_DEF_SPI2].h_spi->Instance)
  {
    spi_tbl[_DEF_SPI2].is_error = true;
  }
  else if (hspi->Instance == spi_tbl[_DEF_SPI3].h_spi->Instance)
  {
    spi_tbl[_DEF_SPI3].is_error = true;
  }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  spi_t  *p_spi;

  if (hspi->Instance == spi_tbl[_DEF_SPI1].h_spi->Instance)
  {
    p_spi = &spi_tbl[_DEF_SPI1];

    p_spi->is_tx_done = true;

    if (p_spi->func_tx != NULL)
    {
      (*p_spi->func_tx)();
    }
  }
  else if (hspi->Instance == spi_tbl[_DEF_SPI2].h_spi->Instance)
  {
    p_spi = &spi_tbl[_DEF_SPI2];

    p_spi->is_tx_done = true;

    if (p_spi->func_tx != NULL)
    {
      (*p_spi->func_tx)();
    }
  }
  else if (hspi->Instance == spi_tbl[_DEF_SPI3].h_spi->Instance)
  {
    p_spi = &spi_tbl[_DEF_SPI3];

    p_spi->is_tx_done = true;

    if (p_spi->func_tx != NULL)
    {
      (*p_spi->func_tx)();
    }
  }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == spi_tbl[_DEF_SPI2].h_spi->Instance)
  {
    HAL_SPI_Receive_IT(spi_tbl[_DEF_SPI2].h_spi , (uint8_t *)aRxBuffer, SPI_RX_BUFF_SIZE);
  }
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
  spi_t  *p_spi;


  if (hspi->Instance == spi_tbl[_DEF_SPI1].h_spi->Instance)
  {
    p_spi = &spi_tbl[_DEF_SPI1];

    p_spi->is_tx_done = true;

    if (p_spi->func_tx != NULL)
    {
      (*p_spi->func_tx)();
    }
  }
  else if (hspi->Instance == spi_tbl[_DEF_SPI2].h_spi->Instance)
  {
    p_spi = &spi_tbl[_DEF_SPI2];

    p_spi->is_tx_done = true;

    if (p_spi->func_tx != NULL)
    {
      (*p_spi->func_tx)();
    }
  }
  else if (hspi->Instance == spi_tbl[_DEF_SPI3].h_spi->Instance)
  {
    p_spi = &spi_tbl[_DEF_SPI3];

    p_spi->is_tx_done = true;

    if (p_spi->func_tx != NULL)
    {
      (*p_spi->func_tx)();
    }
  }
}


void SPI1_IRQHandler(void)
{
  HAL_SPI_IRQHandler(&hspi1);
}


void HAL_SPI_MspInit(SPI_HandleTypeDef* spiHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(spiHandle->Instance==SPI1)
  {
    __HAL_RCC_SPI1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**SPI1 GPIO Configuration
       PA5     ------> SPI1_SCK
       PA6     ------> SPI1_MISO
       PA7     ------> SPI1_MOSI
       PB0     ------> SPI1_NSS
     */
    GPIO_InitStruct.Pin       = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF0_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin       = GPIO_PIN_0;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF0_SPI1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* SPI1 DMA Init */
    /* SPI1_RX Init */
    hdma_spi1_rx.Instance                 = DMA1_Channel2;
    hdma_spi1_rx.Init.Request             = DMA_REQUEST_SPI1_RX;
    hdma_spi1_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_spi1_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_spi1_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_spi1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi1_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_spi1_rx.Init.Mode                = DMA_NORMAL;
    hdma_spi1_rx.Init.Priority            = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_spi1_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(spiHandle,hdmarx,hdma_spi1_rx);

    /* SPI1_TX Init */
    hdma_spi1_tx.Instance                 = DMA1_Channel3;
    hdma_spi1_tx.Init.Request             = DMA_REQUEST_SPI1_TX;
    hdma_spi1_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_spi1_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_spi1_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi1_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_spi1_tx.Init.Mode                = DMA_NORMAL;
    hdma_spi1_tx.Init.Priority            = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_spi1_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(spiHandle,hdmatx,hdma_spi1_tx);

    /* SPI1 interrupt Init */
    HAL_NVIC_SetPriority(SPI1_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(SPI1_IRQn);
  }

}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef* spiHandle)
{
  if(spiHandle->Instance==SPI1)
  {
    /* Peripheral clock disable */
    __HAL_RCC_SPI1_CLK_DISABLE();

    /**SPI1 GPIO Configuration
       PA5     ------> SPI1_SCK
       PA6     ------> SPI1_MISO
       PA7     ------> SPI1_MOSI
       PB0     ------> SPI1_NSS
     */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0);

    /* SPI1 DMA DeInit */
    HAL_DMA_DeInit(spiHandle->hdmarx);
    HAL_DMA_DeInit(spiHandle->hdmatx);

    /* SPI1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(SPI1_IRQn);
  }
}

#endif
