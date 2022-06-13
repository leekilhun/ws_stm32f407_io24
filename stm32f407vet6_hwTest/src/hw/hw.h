/*
 * hw.h
 *
 *  Created on: May 6, 2022
 *      Author: gns2l
 */

#ifndef SRC_HW_HW_H_
#define SRC_HW_HW_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "hw_def.h"


#include "slog.h"
#include "cdc.h"
#include "usb.h"
#include "log.h"
#include "led.h"
#include "uart.h"
#include "cli.h"
#include "i2c.h"
#include "i2c_io.h"
#include "can.h"
#include "spi.h"
#include "flash.h"


bool hwInit(void);

#ifdef __cplusplus
}
#endif



#endif /* SRC_HW_HW_H_ */
