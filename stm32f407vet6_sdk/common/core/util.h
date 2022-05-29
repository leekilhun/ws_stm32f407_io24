/*
 * util.h
 *
 *  Created on: Feb 19, 2021
 *      Author: gns2l
 */

#ifndef SRC_COMMON_CORE_UTIL_H_
#define SRC_COMMON_CORE_UTIL_H_

#ifdef __cplusplus
 extern "C" {
#endif


#include "def.h"


void utilUpdateCrc(uint16_t *p_crc_cur, uint8_t data_in);
uint8_t utilB2D(uint8_t bcdByte);
uint8_t utilD2B(uint8_t decByte);
int utilDwToInt(uint8_t* bytes);
uint32_t utilDwToUint(uint8_t* bytes);
uint8_t utilParseArgs(char *argStr, char **argv);


#ifdef __cplusplus
}
#endif



#endif /* SRC_COMMON_CORE_UTIL_H_ */
