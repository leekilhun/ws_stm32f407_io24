/*
 * w25qxx.h
 *
 *  Created on: Dec 11, 2021
 *      Author: gns2l
 */

#ifndef SRC_COMMON_INC_MODULE_W25QXX_H_
#define SRC_COMMON_INC_MODULE_W25QXX_H_





#include "exhw_def.h"


#ifdef _USE_MODULE_W25QXX

#ifdef __cplusplus
extern "C" {
#endif


  // @https://github.com/nimaltd/w25qxx/blob/master/w25qxx.c

  /** @defgroup w25qxx_Exported_Functions
   * @{
   */

  bool w25qxx_Init(void);
  bool w25qxx_EraseChip(void);
  bool w25qxx_EraseSector(uint32_t sector_addr);
  bool w25qxx_EraseBlock(uint32_t block_addr);

  uint32_t w25qxx_PageToSector(uint32_t page_addr);
  uint32_t w25qxx_PageToBlock(uint32_t page_addr);
  uint32_t w25qxx_SectorToBlock(uint32_t sector_addr);
  uint32_t w25qxx_SectorToPage(uint32_t sector_addr);
  uint32_t w25qxx_BlockToPage(uint32_t block_addr);

  bool w25qxx_IsEmptyPage(uint32_t page_addr, uint32_t offset, uint32_t length);
  bool w25qxx_IsEmptySector(uint32_t sector_addr, uint32_t offset, uint32_t length);
  bool w25qxx_IsEmptyBlock(uint32_t block_addr, uint32_t offset, uint32_t length);

  uint8_t w25qxx_WriteByte(uint8_t data, uint32_t addr);

  uint8_t w25qxx_WriteSubpage(uint8_t *p_data, uint32_t sub_no, uint32_t offset, uint32_t length);
  uint8_t w25qxx_WritePage(uint8_t *p_data, uint32_t addr, uint32_t offset, uint32_t length);
  uint8_t w25qxx_WriteSector(uint8_t *p_data, uint32_t addr, uint32_t offset, uint32_t length);


  void w25qxx_ReadByte(uint8_t *p_data, uint32_t addr);
  void w25qxx_ReadBytes(uint8_t *p_data, uint32_t addr, uint32_t length);
  void w25qxx_ReadPage(uint8_t *p_data, uint32_t addr, uint32_t offset, uint32_t length);
  void w25qxx_ReadSector(uint8_t *p_data, uint32_t addr, uint32_t offset, uint32_t length);
  void w25qxx_ReadBlock(uint8_t *p_data, uint32_t addr, uint32_t offset, uint32_t length);

  //bool w25qxx_Write(uint32_t addr, uint8_t *p_data, uint32_t length);
  //bool w25qxx_Read(uint32_t addr, uint8_t *p_data, uint32_t length);



#ifdef __cplusplus
}
#endif

#endif

#endif /* SRC_COMMON_INC_MODULE_W25QXX_H_ */
