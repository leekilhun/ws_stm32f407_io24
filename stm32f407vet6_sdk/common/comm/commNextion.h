/*
 * commNextion.h
 *
 *  Created on: 2022. 1. 4.
 *      Author: gns2.lee
 */

#ifndef SRC_COMMON_COMM_COMMNEXTION_H_
#define SRC_COMMON_COMM_COMMNEXTION_H_


#include "hw_def.h"


#ifdef __cplusplus
 extern "C" {
#endif

#ifdef _USE_HW_CMD_NEXTION



#define NX_LCD_CMD_MAX_DATA_LENGTH                  24
#define NX_LCD_CMD_MAX_PACKET_LENGTH                (NX_LCD_CMD_MAX_DATA_LENGTH + 8)
#define NX_LCD_RXCMD_TYPE_REQ_BEEP                  0x00
#define NX_LCD_RXCMD_TYPE_WRITE_OUT_REG             0x03
#define NX_LCD_RXCMD_TYPE_CURR_PAGE_NO              0x04
#define NX_LCD_RXCMD_TYPE_UPATE_MCU_REG             0x05
#define NX_LCD_RXCMD_TYPE_CTRL_MOTOR                0x06
#define NX_LCD_RXCMD_TYPE_SAVE_MOTOR_DATA           0x07
#define NX_LCD_RXCMD_TYPE_REQ_MOTOR_DATA            0x08
#define NX_LCD_RXCMD_TYPE_SAVE_AP_CFG               0x09
#define NX_LCD_RXCMD_TYPE_REQ_AP_CFG                0x0a
#define NX_LCD_RXCMD_TYPE_SAVE_CYL_DATA             0x0b
#define NX_LCD_RXCMD_TYPE_REQ_CYL_DATA              0x0c
#define NX_LCD_RXCMD_TYPE_SAVE_VAC_DATA             0x0d
#define NX_LCD_RXCMD_TYPE_REQ_VAC_DATA              0x0e
#define NX_LCD_RXCMD_TYPE_SAVE_SEQ_DATA             0x0f
#define NX_LCD_RXCMD_TYPE_REQ_SEQ_DATA              0x10
#define NX_LCD_RXCMD_TYPE_REQ_SEQ_INIT              0x11
#define NX_LCD_RXCMD_TYPE_CTRL_CYL                  0x12
#define NX_LCD_RXCMD_TYPE_CTRL_VAC                  0x13

#define NX_LCD_RXCMD_TYPE_RESP_BKCMD                0xaa

#define NX_LCD_TXCMD_TYPE_MOTOR_CURR_POS            0x01
#define NX_LCD_TXCMD_TYPE_MOTOR_CURR_VEL            0x02
#define NX_LCD_TXCMD_TYPE_IO_DATA_REG               0x03
#define NX_LCD_TXCMD_TYPE_MCU_DATA_REG              0x04
#define NX_LCD_TXCMD_TYPE_MCU_MOTOR_DATA            0x05
#define NX_LCD_TXCMD_TYPE_MCU_AP_CFG                0x06
#define NX_LCD_TXCMD_TYPE_MCU_CYLINDER_DATA         0x07
#define NX_LCD_TXCMD_TYPE_MCU_VACUUM_DATA           0x08
#define NX_LCD_TXCMD_TYPE_MCU_SEQUENCE_DATA         0x09
#define NX_LCD_TXCMD_TYPE_LCD_BKCMDLEVEL            0x11
#define NX_LCD_TXCMD_TYPE_LCD_START_REPARSEMODE     0x12
#define NX_LCD_TXCMD_TYPE_LCD_END_REPARSEMODE       0x13
#define NX_LCD_TXCMD_TYPE_LCD_CHANGE_PAGE           0x14
#define NX_LCD_TXCMD_TYPE_LCD_REQUSET_PAGE          0x15
#define NX_LCD_TXCMD_TYPE_LCD_RESET_RECEIVE_CNT     0x16
#define NX_LCD_TXCMD_TYPE_POP_ALARM_MSG             0x50
#define NX_LCD_TXCMD_TYPE_OK_RESPONSE               0x70

#define NX_LCD_REPLY_BKCMD_OK                       0x66
#define NX_LCD_REPLY_GETVALUE_TXT                   0x70
#define NX_LCD_REPLY_GETVALUE_NUM                   0x71

/*typedef enum
{
  NEX_PAGE_INIT,
  NEX_PAGE_MAIN,
  NEX_PAGE_MANUAL,
  NEX_PAGE_TEACH,
  NEX_PAGE_IO,
  NEX_PAGE_DATA,
  NEX_PAGE_LOG,
}nextionpage_t;*/


/*
– Level 0 is Off – no pass/fail will be returned
– Level 1 is OnSuccess, only when last serial command successful.
– Level 2 is OnFailure, only when last serial command failed
– Level 3 is Always, returns 0x00 to 0x23 result of serial command.
*/
typedef enum
{
  BKCMD_NO_RETURN,
  BKCMD_ONLY_SUCCESS,
  BKCMD_ONLY_FAIL,
  BKCMD_ALWAYS_RETURN,
}bkCmdLevel;


typedef struct
{
  uint8_t   page_no;
  uint8_t   type;
  uint8_t   obj_id;
  uint16_t  length;
  uint16_t  check_sum;
  uint16_t  check_sum_recv;
  uint8_t   buffer[NX_LCD_CMD_MAX_PACKET_LENGTH];
  uint8_t   *data;
} nextion_packet_t;

typedef struct
{
  uint8_t   ch;
  bool      is_init;
  uint32_t  baud;
  uint8_t   state;
  uint32_t  pre_time;
  uint32_t  data_len;
  uint32_t  index;
  uint8_t   error;

  nextion_packet_t  rx_packet;
} nextion_t;


void cmdNextion_Init(nextion_t *p_cmd);
bool cmdNextion_Open(nextion_t *p_cmd, uint8_t ch, uint32_t baud);
bool cmdNextion_Recovery(nextion_t *p_cmd);
bool cmdNextion_Close(nextion_t *p_cmd);

//Sets the level of Return Data on commands processed
bool cmdNextion_ReceivePacket(nextion_t *p_cmd);
bool cmdNextion_SendCmd(nextion_t *p_cmd, uint8_t cmd, uint8_t *p_data, uint32_t length);
bool cmdNextion_SendCmdRxResp(nextion_t *p_cmd, uint8_t cmd, uint8_t *p_data, uint32_t length, uint32_t timeout);
bool cmdNextion_SendData(nextion_t *p_cmd,uint8_t cmd,uint8_t idx, uint8_t *p_data, uint8_t length);

#endif

#ifdef __cplusplus
 }
#endif



#endif /* SRC_COMMON_COMM_COMMNEXTION_H_ */
