/*
 * commRobotro.h
 *
 *  Created on: 2022. 3. 21.
 *      Author: gns2l
 */

#ifndef SRC_COMMON_COMM_COMMROBOTRO_H_
#define SRC_COMMON_COMM_COMMROBOTRO_H_

#include "hw_def.h"


#ifdef __cplusplus
 extern "C" {
#endif

#ifdef _USE_HW_CMD_ROBOTRO

#define RBTRO_CMD_MAX_DATA_LENGTH                  46
#define RBTRO_CMD_MAX_PACKET_BUFF_LENGTH           (RBTRO_CMD_MAX_DATA_LENGTH + 5)

   typedef struct
   {
     uint8_t         mot_id;
     uint8_t         cmd_type;
     uint8_t         length;
     uint8_t         check_sum;
     uint8_t         check_sum_recv;
     uint8_t         buffer[RBTRO_CMD_MAX_PACKET_BUFF_LENGTH];
     uint8_t   *data;
   } robotro_packet_t;

   typedef struct
   {
     uint8_t   ch;
     bool      is_init;
     bool      is_checked;
     uint32_t  baud;
     uint8_t   state;
     uint32_t  pre_time;
     uint32_t  data_len;
     uint32_t  index;
     uint8_t   error;

     robotro_packet_t  rx_packet;
     robotro_packet_t  tx_packet;
   } robotro_t;


   void cmdRobotro_Init(robotro_t *p_cmd);
   bool cmdRobotro_Open(robotro_t *p_cmd, uint8_t ch, uint32_t baud);
   bool cmdRobotro_Recovery(robotro_t *p_cmd);
   bool cmdRobotro_Close(robotro_t *p_cmd);

   bool cmdRobotro_ReceivePacket(robotro_t *p_cmd);
   bool cmdRobotro_SendCmd(robotro_t *p_cmd, uint8_t cmd, uint8_t *p_data, uint8_t length);

   bool cmdRobotro_SendCmdRxResp(robotro_t *p_cmd, uint8_t cmd, uint8_t *p_data, uint8_t length, uint32_t timeout);


#endif //_USE_HW_CMD_ROBOTRO

#ifdef __cplusplus
 }
#endif

#endif /* SRC_COMMON_COMM_COMMROBOTRO_H_ */
