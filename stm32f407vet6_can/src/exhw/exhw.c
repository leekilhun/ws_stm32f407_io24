/*
 * exhw.c
 *
 *  Created on: May 6, 2022
 *      Author: gns2l
 */


#include "exhw.h"


bool exhwInit(void)
{
  bool ret = true;
#ifdef _USE_EXT_AT24C64
  ret &= at24c64Init();
#endif
  // logBoot(false);

  return ret;
}
