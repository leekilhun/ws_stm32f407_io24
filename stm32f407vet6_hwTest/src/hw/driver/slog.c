/*
 * slog.h
 *
 *  Created on: 2022. 3. 5.
 *      Author: gns2l
 */


#include "slog.h"
#include "uart.h"

#ifdef _USE_HW_SLOG


static uint8_t log_ch = LOG_CH;
static char print_buf[256];

#ifdef _USE_HW_ROTS
static osMutexId mutex_lock;
#endif


bool slogInit(void)
{
#ifdef _USE_HW_ROTS
  osMutexDef(mutex_lock);
  mutex_lock = osMutexCreate (osMutex(mutex_lock));
#endif

  return true;
}

void slogPrintf(const char *fmt, ...)
{
#ifdef _USE_HW_ROTS
  osMutexWait(mutex_lock, osWaitForever);
#endif

  va_list args;
  int len;

  va_start(args, fmt);
  len = vsnprintf(print_buf, 256, fmt, args);

  uartWrite(log_ch, (uint8_t *)print_buf, len);
  va_end(args);

#ifdef _USE_HW_ROTS
  osMutexRelease(mutex_lock);
#endif
}



#endif
