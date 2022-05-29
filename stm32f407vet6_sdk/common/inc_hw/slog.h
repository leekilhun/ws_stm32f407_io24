/*
 * slog.h
 *
 *  Created on: 2022. 3. 5.
 *      Author: gns2l
 */

#ifndef SRC_COMMON_INC_HW_SLOG_H_
#define SRC_COMMON_INC_HW_SLOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hw_def.h"


#ifdef _USE_HW_SLOG

#define LOG_CH            HW_SLOG_CH


bool slogInit(void);
void slogPrintf(const char *fmt, ...);

#endif

#ifdef __cplusplus
}
#endif

#endif /* SRC_COMMON_INC_HW_SLOG_H_ */
