/*
 * can.h
 *
 *  Created on: Jul 10, 2021
 *      Author: gns2l
 */

#ifndef SRC_COMMON_INC_HW_CAN_H_
#define SRC_COMMON_INC_HW_CAN_H_


#ifdef __cplusplus
 extern "C" {
#endif

#include "hw_def.h"


#ifdef _USE_HW_CAN


#define CAN_MAX_CH            HW_CAN_MAX_CH
#define CAN_MSG_RX_BUF_MAX    HW_CAN_MSG_RX_BUF_MAX


typedef enum
{
  CAN_100K,
  CAN_125K,
  CAN_250K,
  CAN_500K,
  CAN_1M,
} can_baud_t;

typedef enum
{
  CAN_NORMAL,
  CAN_SILENT,
  CAN_LOOPBACK
}can_mode_t;

typedef enum
{
  CAN_CLASSIC,
  CAN_FD_NO_BRS,
  CAN_FD_BRS
} can_frame_t;

typedef enum
{
  CAN_STD,
  CAN_EXT
} can_id_type_t;

typedef enum
{
  CAN_DLC_0,
  CAN_DLC_1,
  CAN_DLC_2,
  CAN_DLC_3,
  CAN_DLC_4,
  CAN_DLC_5,
  CAN_DLC_6,
  CAN_DLC_7,
  CAN_DLC_8,
} can_dlc_t;

typedef enum
{
  CAN_LAST_ERR_NONE,
  CAN_LAST_ERR_STUFF,
  CAN_LAST_ERR_FORM,
  CAN_LAST_ERR_ACK,
  CAN_LAST_ERR_BIT_RECESSIVE,
  CAN_LAST_ERR_BIT_DOMINANT,
  CAN_LAST_ERR_CRC,
} can_last_err_t;

typedef enum
{
  CAN_ERR_NONE      = 0x00000000,
  CAN_ERR_PASSIVE   = 0x00000001,
  CAN_ERR_WARNING   = 0x00000002,
  CAN_ERR_BUS_OFF   = 0x00000004,
  CAN_ERR_BUS_FAULT = 0x00000008,
  CAN_ERR_ETC       = 0x00000010,
} can_err_t;


typedef struct
{
  uint32_t id;
  uint16_t length;
  uint8_t  data[8];

  can_dlc_t      dlc;
  can_id_type_t  id_type;
  can_frame_t    frame;
} can_msg_t;





bool     canInit(void);
bool     canOpen(uint8_t ch, can_mode_t mode, can_frame_t frame, can_baud_t baud, can_baud_t baud_data);
void     canClose(uint8_t ch);
bool     canConfigFilter(uint8_t ch, uint8_t index, can_id_type_t id_type, uint32_t id, uint32_t id_mask);

bool     canMsgInit(can_msg_t *p_msg, can_frame_t frame, can_id_type_t  id_type, can_dlc_t dlc);
uint32_t canMsgAvailable(uint8_t ch);
bool     canMsgWrite(uint8_t ch, can_msg_t *p_msg, uint32_t timeout);
bool     canMsgRead(uint8_t ch, can_msg_t *p_msg);

uint16_t canGetRxErrCount(uint8_t ch);
uint16_t canGetTxErrCount(uint8_t ch);
uint32_t canGetError(uint8_t ch);
uint32_t canGetState(uint8_t ch);

void     canErrClear(uint8_t ch);
void     canErrPrint(uint8_t ch);
bool     canUpdate(void);

void     canAttachRxInterrupt(uint8_t ch, bool (*handler)(can_msg_t *arg));
void     canDatachRxInterrupt(uint8_t ch);


#endif

#ifdef __cplusplus
 }
#endif


#endif /* SRC_COMMON_INC_HW_CAN_H_ */
