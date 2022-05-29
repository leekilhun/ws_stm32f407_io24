/*
 * commNextion.c
 *
 *  Created on: 2022. 1. 4.
 *      Author: gns2.lee
 */

#include "commNextion.h"
#include "util.h"
#include "uart.h"

#ifdef _USE_HW_CMD_NEXTION

#define NX_LCD_CMD_OK                      0

#define NX_LCD_CMD_STX1                    0x48
#define NX_LCD_CMD_STX2                    0x4A
#define NX_LCD_CMD_ETX1                    0x4A
#define NX_LCD_CMD_ETX2                    0x4C


#define NX_LCD_CMD_STATE_WAIT_STX          0
#define NX_LCD_CMD_STATE_WAIT_PAGE_NO      1
#define NX_LCD_CMD_STATE_WAIT_TYPE         2
#define NX_LCD_CMD_STATE_WAIT_OBJ_ID       3
#define NX_LCD_CMD_STATE_WAIT_LENGTH_L     4
#define NX_LCD_CMD_STATE_WAIT_LENGTH_H     5
#define NX_LCD_CMD_STATE_WAIT_DATA         6
#define NX_LCD_CMD_STATE_WAIT_CHECKSUM     7
#define NX_LCD_CMD_STATE_WAIT_ETX          8

#define NX_LCD_CMD_STATE_REPLY_PAGE_NO     11

#define WRITE_TX_STR_LEN_MAX             30

void crc_update(uint16_t* crc_in, uint8_t data);

uint8_t wait_next = 0;


void cmdNextion_Init(nextion_t *p_cmd)
{
  p_cmd->is_init = false;
  p_cmd->state = NX_LCD_CMD_STATE_WAIT_STX;

  p_cmd->rx_packet.data = &p_cmd->rx_packet.buffer[NX_LCD_CMD_STATE_WAIT_DATA];
}


bool cmdNextion_Open(nextion_t *p_cmd, uint8_t ch, uint32_t baud)
{
  p_cmd->ch = ch;
  p_cmd->baud = baud;
  p_cmd->is_init = true;
  p_cmd->state = NX_LCD_CMD_STATE_WAIT_STX;
  p_cmd->pre_time = millis();

  return uartOpen(ch, baud);
}

bool cmdNextion_Recovery(nextion_t *p_cmd)
{
  p_cmd->is_init = true;
  p_cmd->state = NX_LCD_CMD_STATE_WAIT_STX;
  p_cmd->pre_time = millis();
  return uartOpen(p_cmd->ch, p_cmd->baud);
}

bool cmdNextion_Close(nextion_t *p_cmd)
{
  return uartClose(p_cmd->ch);
}


#if 1
#define NX_LCD_CRC_POLY 0xA001

void crc_update(uint16_t* crc_in, uint8_t data)
{
  uint16_t crc = *crc_in;
  uint8_t i;
  crc ^= data;

  /* Loop through all 8 data bits */
  for (i = 0; i <= 7; i++)
  {
    if (crc & 0x0001)
    {
      crc >>= 1;
      crc ^= NX_LCD_CRC_POLY;
    }
    else
      crc >>= 1;
  }
  *crc_in = crc;
}

#endif


bool cmdNextion_ReceivePacket(nextion_t *p_cmd)
{
  bool ret = false;
  uint8_t rx_data = 0;

  if (uartAvailable(p_cmd->ch) > 0)
  {
    rx_data = uartRead(p_cmd->ch);
  }
  else
  {
    return false;
  }

  if (millis() - p_cmd->pre_time >= 100)
  {
    p_cmd->state = NX_LCD_CMD_STATE_WAIT_STX;
  }
  p_cmd->pre_time = millis();

  switch (p_cmd->state)
  {
  case NX_LCD_CMD_STATE_WAIT_STX:
  {

    /*nenxtion return packet parsing*/
    if (rx_data == NX_LCD_REPLY_BKCMD_OK)
    {
      p_cmd->rx_packet.type = rx_data;
      p_cmd->state = NX_LCD_CMD_STATE_REPLY_PAGE_NO;
      break;
    }
    /* else
    {
      bool is_avail = (data_cnt - i) > 3;
      bool is_nex_resp = ((uint8_t) ~(data_arr[i + 1] & data_arr[i + 2] & data_arr[i + 3])) == 0;
      if (is_avail && is_nex_resp)
      {
        p_cmd->rx_packet.type = NX_LCD_RXCMD_TYPE_RESP_BKCMD;
        p_cmd->rx_packet.buffer[0] = data_arr[0];
        return true;
      }
    } */

    /*HJ packet parsing*/
    if (rx_data == NX_LCD_CMD_STX1)
    {
      wait_next = 1;
      p_cmd->data_len = 0;
      memset(&p_cmd->rx_packet.buffer[0], 0, NX_LCD_CMD_MAX_PACKET_LENGTH);
      p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
      break;
    }
    if (wait_next)
    {
      wait_next = 0;
      if (rx_data == NX_LCD_CMD_STX2)
      {
        p_cmd->state = NX_LCD_CMD_STATE_WAIT_PAGE_NO;
        p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
        p_cmd->rx_packet.check_sum = 0xffff;
      }
      else
      {
        p_cmd->state = NX_LCD_CMD_STATE_WAIT_STX;
      }
    }
  }
  break;
  case NX_LCD_CMD_STATE_REPLY_PAGE_NO:
  {
    p_cmd->rx_packet.page_no = rx_data;
    p_cmd->state = NX_LCD_CMD_STATE_WAIT_STX;
    ret = true;
  }
  break;
  case NX_LCD_CMD_STATE_WAIT_PAGE_NO:
  {
    p_cmd->rx_packet.page_no = rx_data;
    crc_update(&p_cmd->rx_packet.check_sum, rx_data);
    p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
    p_cmd->state = NX_LCD_CMD_STATE_WAIT_TYPE;
  }
  break;
  case NX_LCD_CMD_STATE_WAIT_TYPE:
    p_cmd->rx_packet.type = rx_data;
    p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
    crc_update(&p_cmd->rx_packet.check_sum, rx_data);

    if (p_cmd->rx_packet.type == NX_LCD_RXCMD_TYPE_CURR_PAGE_NO)
    {
      p_cmd->state = NX_LCD_CMD_STATE_WAIT_CHECKSUM;
      break;
    }
    else if (p_cmd->rx_packet.type == NX_LCD_RXCMD_TYPE_REQ_BEEP)
    {
      p_cmd->state = NX_LCD_CMD_STATE_WAIT_STX;
      ret = true;
      break;
    }

    p_cmd->state = NX_LCD_CMD_STATE_WAIT_OBJ_ID;
    break;

  case NX_LCD_CMD_STATE_WAIT_OBJ_ID:
    p_cmd->rx_packet.obj_id = rx_data;
    crc_update(&p_cmd->rx_packet.check_sum, rx_data);
    p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;

    p_cmd->rx_packet.length = 0;
    p_cmd->state = NX_LCD_CMD_STATE_WAIT_LENGTH_L;
    break;

  case NX_LCD_CMD_STATE_WAIT_LENGTH_L:
    p_cmd->rx_packet.length = rx_data;
    crc_update(&p_cmd->rx_packet.check_sum, rx_data);
    p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
    p_cmd->state = NX_LCD_CMD_STATE_WAIT_LENGTH_H;
    break;

  case NX_LCD_CMD_STATE_WAIT_LENGTH_H:
    p_cmd->rx_packet.length |= (rx_data << 8);
    crc_update(&p_cmd->rx_packet.check_sum, rx_data);
    p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;

    if (p_cmd->rx_packet.length == 0)
    {
      p_cmd->state = NX_LCD_CMD_STATE_WAIT_CHECKSUM;
    }
    else if (p_cmd->rx_packet.length >= NX_LCD_CMD_MAX_DATA_LENGTH)
    {
      p_cmd->state = NX_LCD_CMD_STATE_WAIT_STX;
    }
    else
    {
      p_cmd->index = 0;
      p_cmd->state = NX_LCD_CMD_STATE_WAIT_DATA;
      p_cmd->rx_packet.data = &p_cmd->rx_packet.buffer[p_cmd->data_len];
    }

    break;

  case NX_LCD_CMD_STATE_WAIT_DATA:
    if (p_cmd->index <= p_cmd->rx_packet.length) // check overflow
    {
      p_cmd->rx_packet.data[p_cmd->index++] = rx_data;
      crc_update(&p_cmd->rx_packet.check_sum, rx_data);
      p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;

      if (p_cmd->index == p_cmd->rx_packet.length)
      {
        p_cmd->state = NX_LCD_CMD_STATE_WAIT_CHECKSUM;
      }
    }
    else
    {
      p_cmd->state = NX_LCD_CMD_STATE_WAIT_STX;
      p_cmd->index = 0;
      break;
    }

    break;

  case NX_LCD_CMD_STATE_WAIT_CHECKSUM:
    if (wait_next)
    {
      wait_next = 0;
      p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
      p_cmd->rx_packet.check_sum_recv |= rx_data << 8;
      p_cmd->state = NX_LCD_CMD_STATE_WAIT_ETX;
    }
    else
    {
      wait_next = 1;
      p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
      p_cmd->rx_packet.check_sum_recv = rx_data;
    }

    break;

  case NX_LCD_CMD_STATE_WAIT_ETX:
    if (rx_data == NX_LCD_CMD_ETX1)
    {
      p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
      wait_next = 1;
      break;
    }
    if (wait_next)
    {
      wait_next = 0;
      if (rx_data == NX_LCD_CMD_ETX2)
      {
        p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
        if (p_cmd->rx_packet.check_sum == p_cmd->rx_packet.check_sum_recv)
        {
          ret = true;
        }
      }
    }

    p_cmd->state = NX_LCD_CMD_STATE_WAIT_STX;
    break;
  }

  return ret;
}


bool cmdNextion_SendCmd(nextion_t *p_cmd, uint8_t cmd, uint8_t *p_data, uint32_t length)
{
  uint32_t value = 0;
  uint8_t temp[WRITE_TX_STR_LEN_MAX] = { 0, };
  bool ret = false;

  if (length > 4)
  {
    value = utilDwToInt(p_data);
  }
  else if (length == 1)
  {
    value = (uint32_t)p_data[0];
  }

  //value = p_data[3]<<24|p_data[2]<<16|p_data[1]<<8|p_data[0];

  int len = 0;
  switch (cmd)
  {
    case NX_LCD_TXCMD_TYPE_IO_DATA_REG:
    {
      sprintf((char*)temp,"cc_in_out_reg=%d",(int)value);//"cc_in_out_reg=%d"
      ret = true;
    }
    break;
    case NX_LCD_TXCMD_TYPE_MCU_DATA_REG:
    {
      sprintf((char*)temp, "cc_mcu_state_reg=%d",(int)value);
      ret = true;
    }
    break;
    case NX_LCD_TXCMD_TYPE_MOTOR_CURR_POS:
    {
      sprintf((char*)temp, "cc_mot1_act_pos=%d",(int)value);
      ret = true;
    }
    break;
    case NX_LCD_TXCMD_TYPE_MOTOR_CURR_VEL:
    {
      sprintf((char*)temp, "cc_mot1_act_vel=%d",(int)value);
      ret = true;
    }
    break;
    case NX_LCD_TXCMD_TYPE_LCD_BKCMDLEVEL:
    {
      sprintf((char*)temp, "bkcmd=%d",(int)value);
      ret =true;
    }
    break;
    case NX_LCD_TXCMD_TYPE_LCD_START_REPARSEMODE:
    {
      sprintf((char*)temp, "recmod=%d",(int)value);
      ret =true;
    }
    break;
    case NX_LCD_TXCMD_TYPE_LCD_END_REPARSEMODE:
    {
      sprintf((char*)temp, "DRAKJHSUYDGBNCJHGJKSHBDN");
      ret =true;
    }
    break;
    case NX_LCD_TXCMD_TYPE_LCD_CHANGE_PAGE:
    {
      sprintf((char*)temp, "page %d",(int)value);
      ret =true;
    }
    break;
    case NX_LCD_TXCMD_TYPE_LCD_REQUSET_PAGE:
    {
      sprintf((char*)temp, "sendme");
      ret =true;
    }
    break;
    case NX_LCD_TXCMD_TYPE_LCD_RESET_RECEIVE_CNT:
    {
      sprintf((char*)temp, "nDatCnt.val=1");
      ret =true;
    }
    break;
    case NX_LCD_TXCMD_TYPE_POP_ALARM_MSG:
    {
    }
    break;
    case NX_LCD_TXCMD_TYPE_OK_RESPONSE:
    {
      sprintf((char*)temp, "isResp.val=1");
      ret =true;
    }
    break;
    default:
      break;
  }

  if (ret)
  {
    value = 0;
    len = strnlen((const char*)temp, WRITE_TX_STR_LEN_MAX);
    temp[len++ % WRITE_TX_STR_LEN_MAX] = 0xff;
    temp[len++ % WRITE_TX_STR_LEN_MAX] = 0xff;
    temp[len++ % WRITE_TX_STR_LEN_MAX] = 0xff;
    value = uartWrite(p_cmd->ch, &temp[0], len);
    ret = (value > 0)?true:false;
  }
  return ret;
}


bool cmdNextion_SendData(nextion_t *p_cmd,uint8_t cmd, uint8_t idx, uint8_t *p_data, uint8_t length)
{
  bool ret = false;
  ret = uartWrite(p_cmd->ch, p_data, length);
  return (bool)ret;
}


bool cmdNextion_SendCmdRxResp(nextion_t *p_cmd, uint8_t cmd, uint8_t *p_data, uint32_t length, uint32_t timeout)
{
  bool  ret = false;
  uint32_t time_pre;

  cmdNextion_SendCmd(p_cmd, cmd, p_data, length);

  time_pre = millis();

  while(1)
  {
    if (cmdNextion_ReceivePacket(p_cmd) == true)
    {
      ret = true;
      break;
    }

    if (millis()-time_pre >= timeout)
    {
      break;
    }
  }

  return ret;
}







#endif //_USE_HW_CMD_NEXTION
