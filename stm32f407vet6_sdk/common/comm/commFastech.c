/*
 * commFastech.c
 *
 *  Created on: 2022. 2. 5.
 *      Author: gns2l
 */


#include "commFastech.h"
#include "util.h"
#include "uart.h"

#ifdef _USE_HW_CMD_FASTECH


#define CMD_STX1                    0xAA
#define CMD_STX2                    0xCC
#define CMD_ETX1                    0xAA
#define CMD_ETX2                    0xEE
#define STUFFING_BYTE               0xAA

#define DEF_FASTECH_MOTOR_ID                0x01

#define DEF_FASTECH_COMM_SUCCESS            0

#define CMD_FASTECH_STATE_WAIT_STX          0
#define CMD_FASTECH_STATE_WAIT_ID           1
#define CMD_FASTECH_STATE_WAIT_SYNC         2
#define CMD_FASTECH_STATE_WAIT_TYPE         3
#define CMD_FASTECH_STATE_WAIT_RESPONSE     4
#define CMD_FASTECH_STATE_SKIP_STUFFING     5
#define CMD_FASTECH_STATE_WAIT_DATA         6
#define CMD_FASTECH_STATE_WAIT_CHECKSUM     7
#define CMD_FASTECH_STATE_WAIT_ETX          8

static void fastech_crc_update(uint32_t* crc_in, uint8_t data);

//extern uint32_t check_pass_ms;
//uint32_t pre_ms = 0;

void cmdFastech_Init(fastech_t *p_cmd)
{
  p_cmd->is_init = false;
  p_cmd->wait_next = false;
  p_cmd->skip_stuffing = false;
  p_cmd->state = CMD_FASTECH_STATE_WAIT_STX;

  p_cmd->check_sync.sync_no = 0;
  p_cmd->check_sync.is_checked = false;
  p_cmd->check_sync.keep_cmd = 0;

  p_cmd->rx_packet.data = NULL;
}

bool cmdFastech_Open(fastech_t *p_cmd, uint8_t ch, uint32_t baud)
{
  p_cmd->ch = ch;
  p_cmd->baud = baud;
  p_cmd->is_init = true;
  p_cmd->state = CMD_FASTECH_STATE_WAIT_STX;
  p_cmd->pre_time = millis();

  return uartOpen(ch, baud);
}

bool cmdFastech_Recovery(fastech_t *p_cmd)
{
  p_cmd->is_init = true;
  p_cmd->state = CMD_FASTECH_STATE_WAIT_STX;
  p_cmd->rx_packet.response = 0;
  p_cmd->pre_time = millis();
  return uartOpen(p_cmd->ch, p_cmd->baud);
}

bool cmdFastech_Close(fastech_t *p_cmd)
{
  return uartClose(p_cmd->ch);
}

#define CRC_POLY 0xA001

void fastech_crc_update(uint32_t* crc_in, uint8_t data)
{
  uint16_t crc = (uint16_t)*crc_in;
  uint8_t i;
  crc ^= data;

  /* Loop through all 8 data bits */
  for (i = 0; i <= 7; i++)
  {
    if (crc & 0x0001)
    {
      crc >>= 1;
      crc ^= CRC_POLY;
    }
    else
      crc >>= 1;
  }
  *crc_in = crc;
}



bool cmdFastech_ReceivePacket(fastech_t *p_cmd)
{
  bool ret = false;
  uint8_t rx_data;

  //uint8_t idx_len = FASTECH_CMD_MAX_DATA_LENGTH + 8;
  uint8_t data_cnt = 0;
  uint8_t data_arr[FASTECH_CMD_MAX_DATA_LENGTH + 8] = {0,};

  while(uartAvailable(p_cmd->ch))
  {
    data_arr[data_cnt++] = uartRead(p_cmd->ch);
  }

  if (data_cnt == 0)
  {
    return false;
  }


  if (millis()-p_cmd->pre_time >= 100)
  {
    p_cmd->state = CMD_FASTECH_STATE_WAIT_STX;
  }
  p_cmd->pre_time = millis();

  for (uint8_t i = 0 ; i < data_cnt ; i ++ )
  {
    rx_data = data_arr[i];
    switch(p_cmd->state)
    {
      case CMD_FASTECH_STATE_WAIT_STX:
      {
        if (rx_data == CMD_STX1)
        {
          p_cmd->wait_next = true;
          p_cmd->data_len = 0;
          memset(&p_cmd->rx_packet.buffer[0],0,FASTECH_CMD_MAX_DATA_LENGTH+8);
          p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
          break;
        }
        if (p_cmd->wait_next)
        {
          p_cmd->wait_next = false;
          if (rx_data == CMD_STX2)
          {
            p_cmd->state = CMD_FASTECH_STATE_WAIT_ID;
            p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
            p_cmd->rx_packet.check_sum = 0xffff;
          }
          else
          {
            p_cmd->state = CMD_FASTECH_STATE_WAIT_STX;
          }
        }
      }
      break;
      case CMD_FASTECH_STATE_WAIT_ID:
      {
        p_cmd->rx_packet.id = rx_data;
        fastech_crc_update(&p_cmd->rx_packet.check_sum, rx_data);
        p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
        p_cmd->state = CMD_FASTECH_STATE_WAIT_SYNC;
      }
      break;
      case CMD_FASTECH_STATE_WAIT_SYNC:
      {
        p_cmd->rx_packet.sync_no = rx_data;
        p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
        fastech_crc_update(&p_cmd->rx_packet.check_sum, rx_data);
        p_cmd->state = CMD_FASTECH_STATE_WAIT_TYPE;
      }
      break;
      case CMD_FASTECH_STATE_WAIT_TYPE:
      {
        p_cmd->rx_packet.type = rx_data;
        fastech_crc_update(&p_cmd->rx_packet.check_sum, rx_data);
        p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;

        switch(p_cmd->rx_packet.type)
        {
          case DEF_FASTECH_COMM_TYPE_GET_ALARM_INFO:
            p_cmd->rx_packet.length = 1;
            break;
          case DEF_FASTECH_COMM_TYPE_GET_RAM_PARAM: //GET_RAM_PARAM
          case DEF_FASTECH_COMM_TYPE_GET_AXIS_STATUS: //GET_AXIS_STATUS
          p_cmd->rx_packet.length = 4;
          break;
          case DEF_FASTECH_COMM_TYPE_GET_MOTION_STATUS: //GET_MOTION_STATUS
            p_cmd->rx_packet.length = 20;
            break;
          case DEF_FASTECH_COMM_TYPE_GET_ALL_STATUS: //GET_ALL_STATUS
            p_cmd->rx_packet.length = 33;
            break;
          default:
            p_cmd->rx_packet.length = 0;
            break;
        }
        p_cmd->state = CMD_FASTECH_STATE_WAIT_RESPONSE;
      }
      break;
      case CMD_FASTECH_STATE_WAIT_RESPONSE:
      {
        p_cmd->rx_packet.response = rx_data;
        fastech_crc_update(&p_cmd->rx_packet.check_sum, rx_data);
        p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;

        // TODO:: error response
        if (p_cmd->rx_packet.response != DEF_FASTECH_COMM_SUCCESS)
        {
          p_cmd->error++;
          p_cmd->state = CMD_FASTECH_STATE_WAIT_STX;
          return false;
        }

        if (p_cmd->rx_packet.length > 0)
        {
          p_cmd->index = 0;
          p_cmd->skip_stuffing = false;
          p_cmd->state = CMD_FASTECH_STATE_WAIT_DATA;
          p_cmd->rx_packet.data = &p_cmd->rx_packet.buffer[p_cmd->data_len];
        }
        else
        {
          p_cmd->state = CMD_FASTECH_STATE_WAIT_CHECKSUM;
        }
      }
      break;
      case CMD_FASTECH_STATE_SKIP_STUFFING:
      {
        p_cmd->rx_packet.data[p_cmd->index++] = rx_data;
        p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;

        fastech_crc_update(&p_cmd->rx_packet.check_sum, rx_data);
        p_cmd->state = CMD_FASTECH_STATE_WAIT_DATA;
      }
      break;
      case CMD_FASTECH_STATE_WAIT_DATA:
      {
        // 입력할 데이터 넣기
        if (p_cmd->index <= p_cmd->rx_packet.length )
        {
          if (rx_data == STUFFING_BYTE)
          {
            p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
            p_cmd->state = CMD_FASTECH_STATE_SKIP_STUFFING;
          }
          else
          {
            p_cmd->rx_packet.data[p_cmd->index++] = rx_data;
            p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;

            p_cmd->wait_next = false;
            fastech_crc_update(&p_cmd->rx_packet.check_sum, rx_data);
          }

          if (p_cmd->index == p_cmd->rx_packet.length)
          {
            p_cmd->wait_next = false;
            p_cmd->state = CMD_FASTECH_STATE_WAIT_CHECKSUM;
          }
        }
        else
        {
          p_cmd->index = 0;
          p_cmd->state = CMD_FASTECH_STATE_WAIT_STX;
        }

      }
      break;
      case CMD_FASTECH_STATE_WAIT_CHECKSUM:
      {
        if (p_cmd->wait_next)
        {
          p_cmd->wait_next = false;
          p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
          p_cmd->rx_packet.check_sum_recv |= rx_data<<8;
          p_cmd->state = CMD_FASTECH_STATE_WAIT_ETX;
        }
        else
        {
          p_cmd->wait_next = true;
          p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
          p_cmd->rx_packet.check_sum_recv = rx_data;
        }
      }
      break;
      case CMD_FASTECH_STATE_WAIT_ETX:
      {
        if (rx_data == CMD_ETX1)
        {
          p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
          p_cmd->wait_next = true;
          break;
        }
        if (p_cmd->wait_next)
        {
          p_cmd->wait_next = false;
          if (rx_data == CMD_ETX2)
          {
            p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
            if (p_cmd->rx_packet.check_sum == p_cmd->rx_packet.check_sum_recv)
            {
              //check_pass_ms = millis() - pre_ms ;
              p_cmd->resp_time =  millis() - p_cmd->pre_time;
              ret = true;
            }
          }
        }

        p_cmd->state = CMD_FASTECH_STATE_WAIT_STX;
      }
      break;
    }
  }//for

  return ret;
}


void cmdFastech_SendGetAllStatus(fastech_t *p_cmd)
{
  uint8_t packet[10] = { 0, };

  uint32_t crc = 0xffff;
  uint8_t index = 0;
  packet[index++] = CMD_STX1;
  packet[index++] = CMD_STX2;

  packet[index] = p_cmd->motor_id;//m_FastechComm.rx_packet.id; // bd id
  fastech_crc_update(&crc, packet[index++]);

  p_cmd->check_sync.sync_no = ++p_cmd->rx_packet.sync_no % 255;
  packet[index] = p_cmd->check_sync.sync_no  ; // sync_no
  fastech_crc_update(&crc, packet[index++]);

  packet[index] = DEF_FASTECH_COMM_TYPE_GET_ALL_STATUS; // cmd
  fastech_crc_update(&crc, packet[index++]);

  packet[index++] = crc & 0xff;
  packet[index++] = (crc >> 8) & 0xff;

  packet[index++] = CMD_ETX1;
  packet[index++] = CMD_ETX2;

  uartWrite(p_cmd->ch, &packet[0], index);
  p_cmd->pre_time =  millis();

}


void cmdFastech_SendAlarmType(fastech_t *p_cmd)
{
  uint8_t packet[10] = { 0, };

  uint32_t crc = 0xffff;
  uint8_t index = 0;
  packet[index++] = CMD_STX1;
  packet[index++] = CMD_STX2;

  packet[index] = p_cmd->motor_id;//m_FastechComm.rx_packet.id; // bd id
  fastech_crc_update(&crc, packet[index++]);

  p_cmd->check_sync.sync_no = ++p_cmd->rx_packet.sync_no % 255;
  packet[index] = p_cmd->check_sync.sync_no  ; // sync_no
  fastech_crc_update(&crc, packet[index++]);

  packet[index] = DEF_FASTECH_COMM_TYPE_GET_ALARM_INFO; // cmd
  fastech_crc_update(&crc, packet[index++]);

  packet[index++] = crc & 0xff;
  packet[index++] = (crc >> 8) & 0xff;

  packet[index++] = CMD_ETX1;
  packet[index++] = CMD_ETX2;

  uartWrite(p_cmd->ch, &packet[0], index);
  p_cmd->pre_time =  millis();
}


bool cmdFastech_SendCmd(fastech_t *p_cmd, uint8_t* p_data, uint8_t length)
{
  uint8_t packet[FASTECH_CMD_MAX_DATA_LENGTH] = { 0, };
  uint8_t cmd = p_data[0];
  uint32_t crc = 0xffff;
  uint8_t index = 0;
  packet[index++] = CMD_STX1;
  packet[index++] = CMD_STX2;

  packet[index] = p_cmd->motor_id;//m_FastechComm.rx_packet.id; // bd id
  fastech_crc_update(&crc, packet[index++]);

  p_cmd->check_sync.sync_no = ++p_cmd->rx_packet.sync_no % 255;
  packet[index] = p_cmd->check_sync.sync_no  ; // sync_no
  fastech_crc_update(&crc, packet[index++]);

  packet[index] = cmd;
  fastech_crc_update(&crc, packet[index++]);
  if (length > 1)
  {
    for (uint8_t i = 1; i < length; i++)
    {
      packet[index] = p_data[i]; // data
      fastech_crc_update(&crc, packet[index++]);
    }
  }
  packet[index++] = crc & 0xff;
  packet[index++] = (crc >> 8) & 0xff;

  packet[index++] = CMD_ETX1;
  packet[index++] = CMD_ETX2;

  uint32_t ret = uartWrite(p_cmd->ch, &packet[0], index);
  p_cmd->pre_time =  millis();
  return (ret>0 ? true : false);


#if 0
  switch (cmd)
  {
    case DEF_FASTECH_COMM_TYPE_SERVO_ENABLE:
    {
    }
    break;
    case DEF_FASTECH_COMM_TYPE_ALARM_RESET:
    {
    }
    break;
    case DEF_FASTECH_COMM_TYPE_MOVE_STOP:
    {
    }
    break;
    case DEF_FASTECH_COMM_TYPE_MOVE_EMG_STOP:
    {
    }
    break;
    case DEF_FASTECH_COMM_TYPE_MOVE_TO_LIMIT:
    case DEF_FASTECH_COMM_TYPE_MOVE_VELOCITY:
    case DEF_FASTECH_COMM_TYPE_MOVE_ABS_OVERRIDE:
    case DEF_FASTECH_COMM_TYPE_MOVE_REL_OVERRIDE:
    case DEF_FASTECH_COMM_TYPE_MOVE_VELOCITY_OVERRIDE:
    case DEF_FASTECH_COMM_TYPE_CLEAR_POSITION:
    case DEF_FASTECH_COMM_TYPE_MOVE_ABS_SINGLE_AXIS:
    case DEF_FASTECH_COMM_TYPE_MOVE_REL_SINGLE_AXIS:
    default:
      break;
  }

#endif

}

#endif
