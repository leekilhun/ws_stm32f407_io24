/*
 * commRobotro.c
 *
 *  Created on: 2022. 3. 21.
 *      Author: gns2l
 */


#include "commRobotro.h"
#include "util.h"
#include "uart.h"


#ifdef _USE_HW_CMD_ROBOTRO

#define RBTRO_CMD_STX                    0x96
#define RBTRO_CMD_ETX                    0x69

#define RBTRO_CMD_STATE_WAIT_STX          0
#define RBTRO_CMD_STATE_WAIT_ID           1
#define RBTRO_CMD_STATE_WAIT_TYPE         2
#define RBTRO_CMD_STATE_WAIT_LENGTH       3
#define RBTRO_CMD_STATE_WAIT_DATA         4
#define RBTRO_CMD_STATE_WAIT_CHECKSUM     5
#define RBTRO_CMD_STATE_WAIT_ETX          6

void cmdRobotro_Init(robotro_t *p_cmd)
{
  p_cmd->is_init = false;
  p_cmd->state = RBTRO_CMD_STATE_WAIT_STX;

  p_cmd->is_checked = false;
  p_cmd->rx_packet.data = &p_cmd->rx_packet.buffer[RBTRO_CMD_STATE_WAIT_DATA];
}

bool cmdRobotro_Open(robotro_t *p_cmd, uint8_t ch, uint32_t baud)
{
  p_cmd->ch = ch;
  p_cmd->baud = baud;
  p_cmd->is_init = true;
  p_cmd->state = RBTRO_CMD_STATE_WAIT_STX;
  p_cmd->pre_time = millis();

  return uartOpen(ch, baud);
}

bool cmdRobotro_Recovery(robotro_t *p_cmd)
{
  p_cmd->is_init = true;
  p_cmd->state = RBTRO_CMD_STATE_WAIT_STX;
  p_cmd->pre_time = millis();
  return uartOpen(p_cmd->ch, p_cmd->baud);
}

bool cmdRobotro_Close(robotro_t *p_cmd)
{
  return uartClose(p_cmd->ch);
}



bool cmdRobotro_ReceivePacket(robotro_t *p_cmd)
{
  bool ret = false;
  uint8_t rx_data;

  if (uartAvailable(p_cmd->ch) > 0)
  {
    rx_data = uartRead(p_cmd->ch);
  }
  else
  {
    return false;
  }

  if (millis()-p_cmd->pre_time >= 100)
  {
    p_cmd->state = RBTRO_CMD_STATE_WAIT_STX;
  }
  p_cmd->pre_time = millis();


  switch(p_cmd->state)
  {
    case RBTRO_CMD_STATE_WAIT_STX:
    {
      /*robotro packet parsing*/
      if (rx_data == RBTRO_CMD_STX)
      {
        p_cmd->data_len = 0;
        p_cmd->index = 0;
        memset(&p_cmd->rx_packet.buffer[0],0,RBTRO_CMD_MAX_PACKET_BUFF_LENGTH);

        p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
        p_cmd->rx_packet.check_sum = 0;
        p_cmd->state = RBTRO_CMD_STATE_WAIT_ID;
      }
    }
    break;
    case RBTRO_CMD_STATE_WAIT_ID:
    {
      p_cmd->rx_packet.mot_id = rx_data;
      p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
      p_cmd->rx_packet.check_sum += rx_data;
      p_cmd->state = RBTRO_CMD_STATE_WAIT_TYPE;
    }
    break;
    case RBTRO_CMD_STATE_WAIT_TYPE:
      p_cmd->rx_packet.cmd_type = rx_data;
      p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
      p_cmd->rx_packet.check_sum += rx_data;
      p_cmd->rx_packet.length = 0;
      p_cmd->state = RBTRO_CMD_STATE_WAIT_LENGTH;
      break;

    case RBTRO_CMD_STATE_WAIT_LENGTH:
      p_cmd->rx_packet.length = rx_data % RBTRO_CMD_MAX_DATA_LENGTH; // prevent overflow
      p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
      p_cmd->rx_packet.check_sum += rx_data;
      if (p_cmd->rx_packet.length == 0)
        p_cmd->state = RBTRO_CMD_STATE_WAIT_CHECKSUM;
      else
        p_cmd->state = RBTRO_CMD_STATE_WAIT_DATA;
      break;

    case RBTRO_CMD_STATE_WAIT_DATA:
      if (p_cmd->index + 1 > p_cmd->rx_packet.length)
      {
        p_cmd->state = RBTRO_CMD_STATE_WAIT_STX;
        p_cmd->index = 0;
        break;
      }

      p_cmd->rx_packet.data[p_cmd->index++] = rx_data;
      p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
      p_cmd->rx_packet.check_sum += rx_data;

      if (p_cmd->index == p_cmd->rx_packet.length)
      {
        p_cmd->state = RBTRO_CMD_STATE_WAIT_CHECKSUM;
      }
      break;

    case RBTRO_CMD_STATE_WAIT_CHECKSUM:
      p_cmd->rx_packet.buffer[p_cmd->data_len++] = rx_data;
      p_cmd->rx_packet.check_sum_recv = rx_data;
      if (p_cmd->rx_packet.check_sum == p_cmd->rx_packet.check_sum_recv)
      {
        ret = true;
      }
      p_cmd->state = RBTRO_CMD_STATE_WAIT_STX;
      p_cmd->data_len = 0;
      p_cmd->index = 0;
      break;

    case RBTRO_CMD_STATE_WAIT_ETX:
    default:
      p_cmd->state = RBTRO_CMD_STATE_WAIT_STX;
      break;
  }


  return ret;
}

bool cmdRobotro_SendCmd(robotro_t *p_cmd, uint8_t cmd, uint8_t *p_data, uint8_t length)
{
  uint32_t index;

  index = 0;
  uint8_t check_sum = 0;
  p_cmd->tx_packet.buffer[index++] = RBTRO_CMD_STX;
  p_cmd->tx_packet.buffer[index++] = p_cmd->rx_packet.mot_id;
  p_cmd->tx_packet.buffer[index++] = cmd;
  p_cmd->tx_packet.buffer[index++] = length;
  check_sum = p_cmd->rx_packet.mot_id + cmd + length;
  for (int i=0; i<length; i++)
  {
    p_cmd->tx_packet.buffer[index++] = p_data[i];
    check_sum += p_data[i];
  }

  p_cmd->tx_packet.buffer[index++] = check_sum;
  p_cmd->tx_packet.buffer[index++] = RBTRO_CMD_ETX;

  return (uartWrite(p_cmd->ch, p_cmd->tx_packet.buffer, index) > 0);
}

bool cmdRobotro_SendCmdRxResp(robotro_t *p_cmd, uint8_t cmd, uint8_t *p_data, uint8_t length, uint32_t timeout)
{
  bool  ret = false;
  uint32_t time_pre = 0;

  cmdRobotro_SendCmd(p_cmd,cmd,p_data,length);

  time_pre = millis();
  while (1)
  {
    if (cmdRobotro_ReceivePacket(p_cmd))
    {
      ret = true;
      break;
    }

    if (millis() - time_pre >= timeout )
    {
      break;
    }
  }
 return ret;
}


#endif //_USE_HW_CMD_ROBOTRO
