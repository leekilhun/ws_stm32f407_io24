/*
 * can.c
 *
 *  Created on: Jul 10, 2021
 *      Author: gns2l
 */


#include "can.h"
#include "qbuffer.h"
#include "cli.h"

#ifdef _USE_HW_CAN


typedef struct
{
  uint32_t prescaler;
  uint32_t sjw;
  uint32_t tseg1;
  uint32_t tseg2;
} can_baud_cfg_t;

const can_baud_cfg_t can_baud_cfg_80m_normal[] =
    {
        {20, 4, 13, 4}, // 100K, 77.7%
        {16, 4, 13, 4}, // 125K, 77.7%
        {8,  4, 13, 4}, // 250K, 77.7%
        {4,  4, 13, 4}, // 500K, 77.7.5%
        {2,  4, 13, 4}, // 1M,   77.7%
    };

const can_baud_cfg_t can_baud_cfg_80m_data[] =
    {
        {20, 4, 11, 8}, // 100K, 61.1%
        {16, 4, 11, 8}, // 125K, 61.1%
        {8,  4, 11, 8}, // 250K, 61.1%
        {4,  4, 11, 8}, // 500K, 61.1%
        {2,  4, 10, 7}, // 1M,   61.1%
    };


const can_baud_cfg_t *p_baud_normal = can_baud_cfg_80m_normal;
const can_baud_cfg_t *p_baud_data   = can_baud_cfg_80m_data;


//CAN_HandleTypeDef hcan;

const uint32_t dlc_len_tbl[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};



typedef struct
{
  bool is_init;
  bool is_open;

  uint32_t err_code;
  uint8_t  state;
  uint32_t recovery_cnt;

  uint32_t q_rx_full_cnt;
  uint32_t q_tx_full_cnt;
  uint32_t fifo_full_cnt;
  uint32_t fifo_lost_cnt;

  uint32_t fifo_idx;
  uint32_t enable_int;
  uint32_t interrrupt_line;

  can_mode_t  mode;
  can_frame_t frame;
  can_baud_t  baud;
  can_baud_t  baud_data;

  uint32_t rx_cnt;
  uint32_t tx_cnt;

  CAN_HandleTypeDef hcan;
  bool (*handler)(can_msg_t *arg);

  qbuffer_t q_msg;
  can_msg_t can_msg[CAN_MSG_RX_BUF_MAX];
} can_tbl_t;

static can_tbl_t can_tbl[CAN_MAX_CH];

static volatile uint32_t err_int_cnt = 0;


#ifdef _USE_HW_CLI
static void cliCan(cli_args_t *args);
#endif

static void canErrUpdate(uint8_t ch);
static void canRxFifoUpdate(uint8_t ch);




bool canInit(void)
{
  bool ret = true;

  uint8_t i;


  for(i = 0; i < CAN_MAX_CH; i++)
  {
    can_tbl[i].is_init       = true;
    can_tbl[i].is_open       = false;
    can_tbl[i].err_code      = CAN_ERR_NONE;
    can_tbl[i].state         = 0;
    can_tbl[i].recovery_cnt  = 0;

    can_tbl[i].q_rx_full_cnt = 0;
    can_tbl[i].q_tx_full_cnt = 0;
    can_tbl[i].fifo_full_cnt = 0;
    can_tbl[i].fifo_lost_cnt = 0;

    can_tbl[i].rx_cnt        = 0;
    can_tbl[i].tx_cnt        = 0;
    qbufferCreateBySize(&can_tbl[i].q_msg, (uint8_t *)&can_tbl[i].can_msg[0], sizeof(can_msg_t), CAN_MSG_RX_BUF_MAX);

  }


#ifdef _USE_HW_CLI
  cliAdd("can", cliCan);
#endif
  return ret;
}

bool canOpen(uint8_t ch, can_mode_t mode, can_frame_t frame, can_baud_t baud, can_baud_t baud_data)
{
  bool ret = true;

  if (ch >= CAN_MAX_CH) return false;
  CAN_HandleTypeDef  *p_can;
  //uint32_t tdc_offset;

  p_can = &can_tbl[ch].hcan;
  switch(ch)
  {
    case _DEF_CAN1:
      /* CAN_Init 0 */
      p_can->Instance                   = CAN1;
      p_can->Init.Prescaler             = 4;
      p_can->Init.Mode                  = CAN_MODE_LOOPBACK;
      p_can->Init.SyncJumpWidth         = CAN_SJW_4TQ;
      p_can->Init.TimeSeg1              = CAN_BS1_13TQ;
      p_can->Init.TimeSeg2              = CAN_BS2_4TQ;
      p_can->Init.TimeTriggeredMode     = DISABLE;
      p_can->Init.AutoBusOff            = DISABLE;
      p_can->Init.AutoWakeUp            = DISABLE;
      p_can->Init.AutoRetransmission    = ENABLE;
      p_can->Init.ReceiveFifoLocked     = DISABLE;
      p_can->Init.TransmitFifoPriority  = DISABLE;

      can_tbl[ch].mode                  = mode;
      can_tbl[ch].frame                 = frame;
      can_tbl[ch].baud                  = baud;
      can_tbl[ch].baud_data             = baud_data;
      can_tbl[ch].fifo_idx              = CAN_RX_FIFO0;

      if (HAL_CAN_Init(p_can) != HAL_OK)
      {
        Error_Handler();
      }
      ret = true;
      break;
  }

  if (ret != true)
  {
    return false;
  }


  canConfigFilter(ch, 0, CAN_STD, 0x0000, 0x0000);
  canConfigFilter(ch, 0, CAN_EXT, 0x0000, 0x0000);



  if (HAL_CAN_Start(p_can) != HAL_OK)
  {
    return false;
  }




  can_tbl[ch].is_open = true;

  return ret;
}

void canClose(uint8_t ch)
{

}

bool canConfigFilter(uint8_t ch, uint8_t index, can_id_type_t id_type, uint32_t id, uint32_t id_mask)
{
  bool ret = false;
  CAN_FilterTypeDef sFilterConfig;

  if (ch >= CAN_MAX_CH) return false;

  if (id_type == CAN_STD)
  {
    sFilterConfig.FilterScale = CAN_FILTERSCALE_16BIT;

  }
  else
  {
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
  }

  sFilterConfig.FilterMaskIdHigh     = id_mask;
  sFilterConfig.FilterIdHigh         = id;
  sFilterConfig.FilterMaskIdLow      = id_mask;
  sFilterConfig.FilterIdLow          = id;
  sFilterConfig.FilterMode           = CAN_FILTERMODE_IDMASK;
  sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
  sFilterConfig.FilterBank           = index;
  sFilterConfig.FilterActivation     = ENABLE;

  if (HAL_CAN_ConfigFilter(&can_tbl[ch].hcan, &sFilterConfig) == HAL_OK)
  {
    HAL_CAN_ActivateNotification(&can_tbl[ch].hcan, CAN_IT_RX_FIFO1_MSG_PENDING);
    ret = true;
  }


  return ret;
}

uint32_t canMsgAvailable(uint8_t ch)
{
  if(ch > CAN_MAX_CH) return 0;

  return qbufferAvailable(&can_tbl[ch].q_msg);
}

bool canMsgInit(can_msg_t *p_msg, can_frame_t frame, can_id_type_t  id_type, can_dlc_t dlc)
{
  p_msg->frame   = frame;
  p_msg->id_type = id_type;
  p_msg->dlc     = dlc;
  p_msg->length  = dlc_len_tbl[dlc];
  return true;
}

bool canMsgWrite(uint8_t ch, can_msg_t *p_msg, uint32_t timeout)
{
  CAN_HandleTypeDef  *p_can;
  CAN_TxHeaderTypeDef tx_header;
  //uint32_t pre_time;
  bool ret = true;


  if(ch > CAN_MAX_CH) return false;

  if (can_tbl[ch].err_code & CAN_ERR_BUS_OFF) return false;


  p_can = &can_tbl[ch].hcan;

  if(ch > CAN_MAX_CH) return false;

  if (can_tbl[ch].err_code & CAN_ERR_BUS_OFF) return false;


  switch(p_msg->id_type)
  {
    case CAN_STD :
      tx_header.IDE = CAN_ID_STD;
      break;

    case CAN_EXT :
      tx_header.IDE = CAN_ID_EXT;
      break;
  }

  switch(p_msg->frame)
  {
    case CAN_CLASSIC:
      break;

    case CAN_FD_NO_BRS:
      break;

    case CAN_FD_BRS:
      break;
  }

  tx_header.StdId = p_msg->id;
  tx_header.RTR   = CAN_RTR_DATA;
  tx_header.DLC   = dlc_len_tbl[p_msg->dlc];



  uint32_t TxMailBox = HAL_CAN_GetTxMailboxesFreeLevel(p_can);
  HAL_CAN_AddTxMessage(p_can, &tx_header, p_msg->data, &TxMailBox);


//  tx_header.id  = p_msg->id;
//  tx_header.ext = true;
//  tx_header.dlc = dlc_len_tbl[p_msg->dlc];
//
//  memcpy(msg.data, p_msg->data, dlc_len_tbl[p_msg->dlc]);
//
//  if (mcp2515SendMsg(can_tbl[ch].hfdcan, &msg) == true)
//  {
//    ret = true;
//  }
//  else
//  {
//    ret = false;
//  }

  return ret;
}

bool canMsgRead(uint8_t ch, can_msg_t *p_msg)
{
  bool ret = true;


  if(ch > CAN_MAX_CH) return 0;

  ret = qbufferRead(&can_tbl[ch].q_msg, (uint8_t *)p_msg->data, 1);

  return ret;
}

uint16_t canGetRxErrCount(uint8_t ch)
{
  uint16_t ret = 0;
  if(ch > CAN_MAX_CH) return 0;

  ret = can_tbl[ch].hcan.Instance->ESR>>24; //mcp2515GetRxErrCount(ch);
  return ret;
}

uint16_t canGetTxErrCount(uint8_t ch)
{
  uint16_t ret = 0;
  if(ch > CAN_MAX_CH) return 0;

  ret = can_tbl[ch].hcan.Instance->ESR>>16 & 0x00ff;  // = mcp2515GetTxErrCount(ch);
  return ret;
}

uint32_t canGetError(uint8_t ch)
{
  if(ch > CAN_MAX_CH) return 0;
  //mcp2515ReadErrorFlags(can_tbl[ch].hfdcan);
  return (uint16_t)HAL_CAN_GetError(&can_tbl[ch].hcan);
}

/*
HAL_CAN_STATE_RESET             = 0x00U,  !< CAN not yet initialized or disabled /
HAL_CAN_STATE_READY             = 0x01U,  !< CAN initialized and ready for use   /
HAL_CAN_STATE_LISTENING         = 0x02U,  !< CAN receive process is ongoing      /
HAL_CAN_STATE_SLEEP_PENDING     = 0x03U,  !< CAN sleep request is pending        /
HAL_CAN_STATE_SLEEP_ACTIVE      = 0x04U,  !< CAN sleep mode is active            /
HAL_CAN_STATE_ERROR             = 0x05U   !< CAN error state
*/
uint32_t canGetState(uint8_t ch)
{
  if(ch > CAN_MAX_CH) return 0;
  HAL_CAN_StateTypeDef state= HAL_CAN_GetState(&can_tbl[ch].hcan);

  return (uint32_t)state;
}

void canAttachRxInterrupt(uint8_t ch, bool (*handler)(can_msg_t *arg))
{
  if(ch > CAN_MAX_CH) return;

  can_tbl[ch].handler = handler;
}

void canDetachRxInterrupt(uint8_t ch)
{
  if(ch > CAN_MAX_CH) return;

  can_tbl[ch].handler = NULL;
}

void canRecovery(uint8_t ch)
{
  if(ch > CAN_MAX_CH) return;

  HAL_CAN_Stop(&can_tbl[ch].hcan);
  HAL_CAN_Start(&can_tbl[ch].hcan);

  can_tbl[ch].recovery_cnt++;
}

bool canUpdate(void)
{
  bool ret = false;
  can_tbl_t *p_can;


  for (int i=0; i<CAN_MAX_CH; i++)
  {
    p_can = &can_tbl[i];

    canErrUpdate(i);
    canRxFifoUpdate(i);

    switch(p_can->state)
    {
      case 0:
        if (p_can->err_code & CAN_ERR_BUS_OFF)
        {
          canRecovery(i);
          p_can->state = 1;
          ret = true;
        }
        break;

      case 1:
        if ((p_can->err_code & CAN_ERR_BUS_OFF) == 0)
        {
          p_can->state = 0;
        }
        break;
    }
  }

  return ret;
}

void canRxFifoUpdate(uint8_t ch)
{
  can_msg_t *rx_buf;
  CAN_RxHeaderTypeDef rx_header;


  rx_buf  = (can_msg_t *)qbufferPeekWrite(&can_tbl[ch].q_msg);

  if (HAL_CAN_GetRxMessage(&can_tbl[ch].hcan, can_tbl[ch].fifo_idx, &rx_header, rx_buf->data) == HAL_OK)
  {
    if(rx_header.IDE == CAN_STD)
    {
      rx_buf->id      = rx_header.StdId;
      rx_buf->id_type = CAN_STD;
    }
    else
    {
      rx_buf->id      = rx_header.ExtId;
      rx_buf->id_type = CAN_EXT;
    }
    rx_buf->length = dlc_len_tbl[(rx_header.DLC >> 16) & 0x0F];
    rx_buf->frame = CAN_CLASSIC;

    can_tbl[ch].rx_cnt++;

    if (qbufferWrite(&can_tbl[ch].q_msg, NULL, 1) != true)
    {
      can_tbl[ch].q_rx_full_cnt++;
    }

    if( can_tbl[ch].handler != NULL )
    {
      if ((*can_tbl[ch].handler)((void *)rx_buf) == true)
      {
        qbufferRead(&can_tbl[ch].q_msg, NULL, 1);
      }
    }
  }

}

void canErrClear(uint8_t ch)
{
  if(ch > CAN_MAX_CH) return;

  can_tbl[ch].err_code = CAN_ERR_NONE;
}

void canErrPrint(uint8_t ch)
{
  uint32_t err_code;


  if(ch > CAN_MAX_CH) return;

  err_code = can_tbl[ch].err_code;

  if (err_code & CAN_ERR_PASSIVE) logPrintf("  ERR : CAN_ERR_PASSIVE\n");
  if (err_code & CAN_ERR_WARNING) logPrintf("  ERR : CAN_ERR_WARNING\n");
  if (err_code & CAN_ERR_BUS_OFF) logPrintf("  ERR : CAN_ERR_BUS_OFF\n");
}

void canErrUpdate(uint8_t ch)
{
  uint32_t protocol_status;
  protocol_status = can_tbl[ch].hcan.Instance->ESR &0x00000007;
  can_tbl[ch].err_code = protocol_status ;

  if (protocol_status & (1<<6))
  {
    can_tbl[ch].fifo_full_cnt++;
  }

}



/**
 * @brief This function handles USB low priority or CAN RX0 interrupts.
 */
void USB_LP_CAN1_RX0_IRQHandler(void)
{
  HAL_CAN_IRQHandler(&can_tbl[_DEF_CAN1].hcan);
}



void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(canHandle->Instance==CAN1)
  {
    /* CAN1 clock enable */
    __HAL_RCC_CAN1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**CAN GPIO Configuration
    PB8     ------> CAN_RX
    PB9     ------> CAN_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    __HAL_AFIO_REMAP_CAN1_2();

    /* CAN1 interrupt Init */
    HAL_NVIC_SetPriority(USB_HP_CAN1_TX_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn);
    HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
    HAL_NVIC_SetPriority(CAN1_RX1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);
    HAL_NVIC_SetPriority(CAN1_SCE_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(CAN1_SCE_IRQn);
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* canHandle)
{

  if(canHandle->Instance==CAN1)
  {
    /* Peripheral clock disable */
    __HAL_RCC_CAN1_CLK_DISABLE();

    /**CAN GPIO Configuration
    PB8     ------> CAN_RX
    PB9     ------> CAN_TX
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8|GPIO_PIN_9);
    HAL_NVIC_DisableIRQ(CAN1_RX1_IRQn);
    HAL_NVIC_DisableIRQ(CAN1_SCE_IRQn);
 }
}















#ifdef _USE_HW_CLI
void cliCan(cli_args_t *args)
{
  bool ret = false;



  if (args->argc == 1 && args->isStr(0, "info"))
  {
    for (int i=0; i<CAN_MAX_CH; i++)
    {
      cliPrintf("is_open       : %d\n", can_tbl[i].is_open);

      cliPrintf("q_rx_full_cnt : %d\n", can_tbl[i].q_rx_full_cnt);
      cliPrintf("q_tx_full_cnt : %d\n", can_tbl[i].q_tx_full_cnt);
      cliPrintf("fifo_full_cnt : %d\n", can_tbl[i].fifo_full_cnt);
      cliPrintf("fifo_lost_cnt : %d\n", can_tbl[i].fifo_lost_cnt);
      canErrPrint(i);
    }
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "read"))
  {
    uint32_t index = 0;

    while(cliKeepLoop())
    {
      canUpdate();

      if (canMsgAvailable(_DEF_CAN1))
      {
        can_msg_t msg;

        canMsgRead(_DEF_CAN1, &msg);

        index %= 1000;
        cliPrintf("%03d(R) <- id ", index++);
        if (msg.id_type == CAN_STD)
        {
          cliPrintf("std ");
        }
        else
        {
          cliPrintf("ext ");
        }
        cliPrintf(": 0x%08X, L:%02d, ", msg.id, msg.length);
        for (int i=0; i<msg.length; i++)
        {
          cliPrintf("0x%02X ", msg.data[i]);
        }
        cliPrintf("\n");
      }
    }
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "send"))
  {
    uint32_t pre_time;
    uint32_t index = 0;
    uint32_t err_code;


    err_code = can_tbl[_DEF_CAN1].err_code;

    while(cliKeepLoop())
    {
      can_msg_t msg;

      if (millis()-pre_time >= 1000)
      {
        pre_time = millis();

        msg.frame   = CAN_CLASSIC;
        msg.id_type = CAN_EXT;
        msg.dlc     = CAN_DLC_2;
        msg.id      = 0x314;
        msg.length  = 2;
        msg.data[0] = 1;
        msg.data[1] = 2;
        if (canMsgWrite(_DEF_CAN1, &msg, 10) > 0)
        {
          index %= 1000;
          cliPrintf("%03d(T) -> id ", index++);
          if (msg.id_type == CAN_STD)
          {
            cliPrintf("std ");
          }
          else
          {
            cliPrintf("ext ");
          }
          cliPrintf(": 0x%08X, L:%02d, ", msg.id, msg.length);
          for (int i=0; i<msg.length; i++)
          {
            cliPrintf("0x%02X ", msg.data[i]);
          }
          cliPrintf("\n");
        }
        else
        {
          cliPrintf("err %d \n", can_tbl[_DEF_CAN1].err_code/*mcp2515ReadErrorFlags(can_tbl[_DEF_CAN1].hcan*/);
        }


        if (canGetRxErrCount(_DEF_CAN1) > 0 || canGetTxErrCount(_DEF_CAN1) > 0)
        {
          cliPrintf("ErrCnt : %d, %d\n", canGetRxErrCount(_DEF_CAN1), canGetTxErrCount(_DEF_CAN1));
        }

        if (err_int_cnt > 0)
        {
          cliPrintf("Cnt : %d\n",err_int_cnt);
          err_int_cnt = 0;
        }
      }

      if (can_tbl[_DEF_CAN1].err_code != err_code)
      {
        cliPrintf("ErrCode : 0x%X\n", can_tbl[_DEF_CAN1].err_code);
        canErrPrint(_DEF_CAN1);
        err_code = can_tbl[_DEF_CAN1].err_code;
      }

      if (canUpdate())
      {
        cliPrintf("BusOff Recovery\n");
      }


      if (canMsgAvailable(_DEF_CAN1))
      {
        canMsgRead(_DEF_CAN1, &msg);

        index %= 1000;
        cliPrintf("%03d(R) <- id ", index++);
        if (msg.id_type == CAN_STD)
        {
          cliPrintf("std ");
        }
        else
        {
          cliPrintf("ext ");
        }
        cliPrintf(": 0x%08X, L:%02d, ", msg.id, msg.length);
        for (int i=0; i<msg.length; i++)
        {
          cliPrintf("0x%02X ", msg.data[i]);
        }
        cliPrintf("\n");
      }
    }
    ret = true;
  }

  if (ret == false)
  {
    cliPrintf("can info\n");
    cliPrintf("can read\n");
    cliPrintf("can send\n");
  }
}
#endif

#endif
