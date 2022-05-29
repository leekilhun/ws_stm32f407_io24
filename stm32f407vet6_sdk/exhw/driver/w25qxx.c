/*
 * w25qxx.c
 *
 *  Created on: 2021. 12. 19.
 *      Author: gns2l
 */

#include <w25qxx.h>
#include "spi.h"
#include "cli.h"
#include "gpio.h"
#include "tickTimer.h"

//******************************************************************************
//
// W25Q16JV: 16M-bit / 2M-byte (2,097,152)
//
//          STM32F103 |            | W25Q16JV  SSIQ
//            PA2 /CS |----------->| 1st Pin CS
//                    |            |
//            PA7  MO |----------->| 5th Pin DI
//                    |            |
//            PA6  MI |<---------- | 2nd Pin DO
//                    |            |
//            PA5 CLK |----------->| 6th Pin CLK
//                    |            |
//               3.3V |----------->| 7th Slave Reset (GPIO)
//                    |            |
//
//******************************************************************************


//#define W25QXX_FLASH_SIZE                  (0x200000)      16 MBits => 2MBytes

#define W25QXX_SECTOR_SIZE                 0x20           // 32 sectors of 64KBytes
#define W25QXX_SUBSECTOR_SIZE              0x1000         // 4096 subsectors of 4kBytes
#define W25QXX_PAGE_SIZE                   0x100          // 65536 pages of 256 bytes

#define W25QXX_DUMMY_CYCLES_READ           8
#define W25QXX_DUMMY_CYCLES_READ_QUAD      4

#define W25QXX_BULK_ERASE_MAX_TIME         250000
#define W25QXX_SECTOR_ERASE_MAX_TIME       3000
#define W25QXX_SUBSECTOR_ERASE_MAX_TIME    800
#define W25Qxx_SUB_PAGE_COUNT                   16

/**
 * @brief  W25QXX Commands
 */


/* Write Operations */
#define WRITE_ENABLE_CMD                     0x06
#define WRITE_DISABLE_CMD                    0x04


/* Read Operations */
#define READ_CMD                             0x03
#define FAST_READ_CMD                        0x0B


/* Reset Operations */
#define RESET_ENABLE_CMD                     0x66
#define RESET_MEMORY_CMD                     0x99

/* Identification Operations */
#define READ_ID_CMD                          0x9F
#define READ_ID_CMD2                         0x9F
#define MULTIPLE_IO_READ_ID_CMD              0xAF
#define READ_SERIAL_FLASH_DISCO_PARAM_CMD    0x5A

#define DUAL_OUT_FAST_READ_CMD               0x3B
//#define DUAL_INOUT_FAST_READ_CMD             0xBB //w25q128
//#define QUAD_OUT_FAST_READ_CMD               0x6B //w25q128
//#define QUAD_INOUT_FAST_READ_CMD             0xEB //w25q128


/* Register Operations */
#define READ_STATUS_REG_CMD                  0x05
#define WRITE_STATUS_REG_CMD                 0x01

#define READ_STATUS_REG2_CMD                 0x35
#define WRITE_STATUS_REG2_CMD                0x31

#define READ_STATUS_REG3_CMD                 0x15
#define WRITE_STATUS_REG3_CMD                0x11


#define READ_LOCK_REG_CMD                    0xE8
#define WRITE_LOCK_REG_CMD                   0xE5

#define READ_FLAG_STATUS_REG_CMD             0x70
#define CLEAR_FLAG_STATUS_REG_CMD            0x50

#define READ_NONVOL_CFG_REG_CMD              0xB5
#define WRITE_NONVOL_CFG_REG_CMD             0xB1

#define READ_VOL_CFG_REG_CMD                 0x85
#define WRITE_VOL_CFG_REG_CMD                0x81

#define READ_ENHANCED_VOL_CFG_REG_CMD        0x65
#define WRITE_ENHANCED_VOL_CFG_REG_CMD       0x61

/* Program Operations */
#define PAGE_PROG_CMD                        0x02
#define DUAL_IN_FAST_PROG_CMD                0xA2
#define EXT_DUAL_IN_FAST_PROG_CMD            0xD2
//#define QUAD_IN_FAST_PROG_CMD                0x32    //w25q128
#define EXT_QUAD_IN_FAST_PROG_CMD            0x12

/* Erase Operations */
#define SECTOR_ERASE_4K_CMD                  0x20
#define BLOCK_ERASE_32K_CMD                  0x52
#define BLOCK_ERASE_64K_CMD                  0xD8
#define CHIP_ERASE_CMD                       0xC7

#define PROG_ERASE_RESUME_CMD                0x7A
#define PROG_ERASE_SUSPEND_CMD               0x75

/* One-Time Programmable Operations */
#define READ_OTP_ARRAY_CMD                   0x4B
#define PROG_OTP_ARRAY_CMD                   0x42

/**
 * @brief  W25QXX Registers
 */
/* Status Register */
#define W25QXX_SR_WIP                      ((uint8_t)0x01)    /*!< Write in progress */
#define W25QXX_SR_WREN                     ((uint8_t)0x02)    /*!< Write enable latch */
#define W25QXX_SR_BLOCKPR                  ((uint8_t)0x5C)    /*!< Block protected against program and erase operations */
#define W25QXX_SR_PRBOTTOM                 ((uint8_t)0x20)    /*!< Protected memory area defined by BLOCKPR starts from top or bottom */
#define W25QXX_SR_SRWREN                   ((uint8_t)0x80)    /*!< Status register write enable/disable */

/* Nonvolatile Configuration Register */
#define W25QXX_NVCR_LOCK                   ((uint16_t)0x0001) /*!< Lock nonvolatile configuration register */
#define W25QXX_NVCR_DUAL                   ((uint16_t)0x0004) /*!< Dual I/O protocol */
#define W25QXX_NVCR_QUAB                   ((uint16_t)0x0008) /*!< Quad I/O protocol */
#define W25QXX_NVCR_RH                     ((uint16_t)0x0010) /*!< Reset/hold */
#define W25QXX_NVCR_ODS                    ((uint16_t)0x01C0) /*!< Output driver strength */
#define W25QXX_NVCR_XIP                    ((uint16_t)0x0E00) /*!< XIP mode at power-on reset */
#define W25QXX_NVCR_NB_DUMMY               ((uint16_t)0xF000) /*!< Number of dummy clock cycles */

/* Volatile Configuration Register */
#define W25QXX_VCR_WRAP                    ((uint8_t)0x03)    /*!< Wrap */
#define W25QXX_VCR_XIP                     ((uint8_t)0x08)    /*!< XIP */
#define W25QXX_VCR_NB_DUMMY                ((uint8_t)0xF0)    /*!< Number of dummy clock cycles */

/* Enhanced Volatile Configuration Register */
#define W25QXX_EVCR_ODS                    ((uint8_t)0x07)    /*!< Output driver strength */
#define W25QXX_EVCR_VPPA                   ((uint8_t)0x08)    /*!< Vpp accelerator */
#define W25QXX_EVCR_RH                     ((uint8_t)0x10)    /*!< Reset/hold */
#define W25QXX_EVCR_DUAL                   ((uint8_t)0x40)    /*!< Dual I/O protocol */
#define W25QXX_EVCR_QUAD                   ((uint8_t)0x80)    /*!< Quad I/O protocol */

/* Flag Status Register */
#define W25QXX_FSR_PRERR                   ((uint8_t)0x02)    /*!< Protection error */
#define W25QXX_FSR_PGSUS                   ((uint8_t)0x04)    /*!< Program operation suspended */
#define W25QXX_FSR_VPPERR                  ((uint8_t)0x08)    /*!< Invalid voltage during program or erase */
#define W25QXX_FSR_PGERR                   ((uint8_t)0x10)    /*!< Program error */
#define W25QXX_FSR_ERERR                   ((uint8_t)0x20)    /*!< Erase error */
#define W25QXX_FSR_ERSUS                   ((uint8_t)0x40)    /*!< Erase operation suspended */
#define W25QXX_FSR_READY                   ((uint8_t)0x80)    /*!< Ready or command in progress */


/*  */
#define W25Q_DUMMY_BYTE 0x00


typedef enum
{
  W25Q_ID_10    = ((uint8_t)0x11),
  W25Q_ID_20    = ((uint8_t)0x12),
  W25Q_ID_40    = ((uint8_t)0x13),
  W25Q_ID_80    = ((uint8_t)0x14),
  W25Q_ID_16    = ((uint8_t)0x15),
  W25Q_ID_32    = ((uint8_t)0x16),
  W25Q_ID_64    = ((uint8_t)0x17),
  W25Q_ID_128   = ((uint8_t)0x18),
  W25Q_ID_256   = ((uint8_t)0x19),
  W25Q_ID_512   = ((uint8_t)0x20),

} w25qId;

typedef enum
{
  W25Q_SR_IDLE    = ((uint8_t)0x00),
  W25Q_SR_BUSY    = ((uint8_t)0x01),
  W25Q_SR_WEL     = ((uint8_t)0x02),
  W25Q_SR_BP0     = ((uint8_t)0x04),
  W25Q_SR_BP1     = ((uint8_t)0x08),
  W25Q_SR_BP2     = ((uint8_t)0x10),
  W25Q_SR_TB      = ((uint8_t)0x20),
  W25Q_SR_SEC     = ((uint8_t)0x40),
  W25Q_SR_SRP     = ((uint8_t)0x80),

} w25qStatus;



typedef struct
{
  w25qId          id;
  uint8_t         uniq_id[8];
  uint16_t        page_size;
  uint32_t        page_cnt;
  uint32_t        sector_size;
  uint32_t        sector_cnt;
  uint32_t        block_size;
  uint32_t        block_cnt;
  uint32_t        capacityKb;
  w25qStatus      status_reg1;
  w25qStatus      status_reg2;
  w25qStatus      status_reg3;
  uint8_t         lock;

} w25q_t;



#ifdef _USE_MODULE_W25QXX

static bool is_init;
const uint8_t spi_ch =  _SPI_W25Q16;
w25q_t w25qxx;


#define _PIN_W25Q16_CS      0

static void w25qxx_csPinWrite(bool value);

static uint8_t w25qxx_ReadId(void);
static uint8_t w25qxx_ReadUniqId(void);

static bool w25qxx_Reset(void);
static void w25qxx_WriteEnable(void);

/*static void w25qxx_WriteDisable(void);*/

static uint8_t w25qxx_ReadStatusReg(uint8_t reg_no);

/*
static uint8_t w25qxx_ReadReg(uint8_t addr);
static bool w25qxx_ReadRegs(uint8_t addr, uint8_t *p_data, uint16_t length);
static bool w25qxx_WriteReg(uint8_t addr, uint8_t data);
static bool w25qxx_WriteRegs(uint8_t addr, uint8_t *p_data, uint16_t length);
*/

static bool w25qxx_WaitForWriteCplt(void);

#ifdef _USE_HW_CLI
static void cliW25QXX(cli_args_t *args);
#endif



static void w25qxx_TransCpltISR(void)
{

}

void w25qxx_csPinWrite(bool value)
{
  gpioPinWrite(_PIN_W25Q16_CS, value);

}


bool w25qxx_Init(void)
{
  bool ret = true;

  ret = spiBegin(spi_ch);
  spiAttachTxInterrupt(spi_ch, w25qxx_TransCpltISR);

  w25qxx.lock = ENABLE;

  w25qxx_csPinWrite(_DEF_HIGH);
  ret = w25qxx_Reset();
  uint32_t id;
  id = w25qxx_ReadId();

  switch (id)
  {
    case W25Q_ID_512:
      w25qxx.block_cnt = 1024;
      break;
    case W25Q_ID_256:
      w25qxx.block_cnt = 512;
      break;
    case W25Q_ID_128:
      w25qxx.block_cnt = 256;
      break;
    case W25Q_ID_64:
      w25qxx.block_cnt = 128;
      break;
    case W25Q_ID_32:
      w25qxx.block_cnt = 64;
      break;
    case W25Q_ID_16:
      w25qxx.block_cnt = 32;
      break;
    case W25Q_ID_80:
      w25qxx.block_cnt = 16;
      break;
    case W25Q_ID_40:
      w25qxx.block_cnt = 8;
      break;
    case W25Q_ID_20:
      w25qxx.block_cnt = 4;
      break;
    case W25Q_ID_10:
      w25qxx.block_cnt = 2;
      break;
    default:
      w25qxx.lock = DISABLE;
      logPrintf("w25q16 : read id fail!");
      return false;
  }

  w25qxx.id = id;
  w25qxx.page_size = W25QXX_PAGE_SIZE;
  w25qxx.sector_size = W25QXX_SUBSECTOR_SIZE;
  w25qxx.sector_cnt = w25qxx.block_cnt * 16;
  w25qxx.page_cnt = (w25qxx.sector_cnt * w25qxx.sector_size) / w25qxx.page_size;
  w25qxx.block_size = w25qxx.sector_size * 16;
  w25qxx.capacityKb = (w25qxx.sector_cnt * w25qxx.sector_size) / 1024;
  w25qxx_ReadUniqId();
  w25qxx.status_reg1 = (w25qStatus)w25qxx_ReadStatusReg(1);
  w25qxx.status_reg2 = (w25qStatus)w25qxx_ReadStatusReg(2);
  w25qxx.status_reg3 = (w25qStatus)w25qxx_ReadStatusReg(3);

  is_init = true;

  w25qxx.lock = DISABLE;

#ifdef _USE_HW_CLI
  cliAdd("w25qxx", cliW25QXX);
#endif

  return ret;
}

bool w25qxx_Reset(void)
{
  bool ret = false;

  w25qxx_csPinWrite(_DEF_LOW);
  spiTransfer8(spi_ch, RESET_ENABLE_CMD);
  w25qxx_csPinWrite(_DEF_HIGH);
  delay(10);

  w25qxx_csPinWrite(_DEF_LOW);
  uint8_t buf = RESET_MEMORY_CMD;
  ret = spiTransfer(spi_ch, &buf, &buf, 1, 10);
  w25qxx_csPinWrite(_DEF_HIGH);
  delay(10);

  return ret;
}


bool w25qxx_EraseChip(void)
{
  bool ret = false;
  while (w25qxx.lock == ENABLE)
    delay(1);

  w25qxx.lock = ENABLE;

  w25qxx_WriteEnable();
  w25qxx_csPinWrite(_DEF_LOW);

  spiTransfer8(spi_ch,CHIP_ERASE_CMD);

  w25qxx_csPinWrite(_DEF_HIGH);
  ret = w25qxx_WaitForWriteCplt();
  if(ret == false)
    logPrintf("w25q16 : erase timeout fail! ");

  delay(10);
  w25qxx.lock  = DISABLE;
  //w25qxx_WriteDisable();
  return ret;
}


bool w25qxx_EraseSector(uint32_t sector_addr)
{
  bool ret = false;
  uint8_t buf[4] ={0,};

  while (w25qxx.lock == ENABLE)
    delay(1);

  w25qxx.lock = ENABLE;

  w25qxx_WaitForWriteCplt();

  if ((sector_addr < w25qxx.sector_cnt) == false)
  {
    logPrintf("w25q16 : [%d] is erase sector over range![max 511] "
        ,sector_addr );
    return ret;
  }

  sector_addr = sector_addr * w25qxx.sector_size;

  w25qxx_WriteEnable();
  w25qxx_csPinWrite(_DEF_LOW);

  if (w25qxx.id == W25Q_ID_256 || w25qxx.id ==W25Q_ID_512 )
  {
    //TODO:
    //spiTransfer8(spi_ch,0x21);
    //spiTransfer8(spi_ch,((sector_addr & 0xFF000000) >> 24);
  }
  else
  {
    buf[0] = SECTOR_ERASE_4K_CMD;
    buf[1] = (sector_addr & 0xff0000) >> 16;
    buf[2] = (sector_addr & 0xff00) >> 8;
    buf[3] = (sector_addr & 0xff);
    if (spiTransfer(spi_ch, buf, buf, 4, 10) == true)
    {
      ret = true;
    }

  }

  w25qxx_csPinWrite(_DEF_HIGH);
  if (ret)
    ret = w25qxx_WaitForWriteCplt();

  delay(1);
  w25qxx.lock = DISABLE;

  return ret;
}


bool w25qxx_EraseBlock(uint32_t block_addr)
{
  bool ret = false;
  uint8_t buf[4] ={0,};

  while (w25qxx.lock == ENABLE)
    delay(1);
  w25qxx.lock = ENABLE;

  ret = w25qxx_WaitForWriteCplt();
  if (ret == false)
  {
    logPrintf("w25q16 : status check timeout fail! ");
    w25qxx.lock = DISABLE;
    return ret;
  }

  if ((block_addr < w25qxx.block_cnt) == false)
  {
    logPrintf("w25q16 : [%d] is erase block over range![max 31] "
        ,block_addr );
    return ret;
  }

  block_addr = block_addr * w25qxx.sector_size * 16;
  w25qxx_WriteEnable();

  w25qxx_csPinWrite(_DEF_LOW);

  if (w25qxx.id == W25Q_ID_256 || w25qxx.id ==W25Q_ID_512 )
  {
    //TODO:
    //spiTransfer8(spi_ch,0xDC);
    //spiTransfer8(spi_ch,((block_addr & 0xFF000000) >> 24);
  }
  else
  {
    buf[0] = BLOCK_ERASE_64K_CMD;
    buf[1] = (block_addr & 0xff0000) >> 16;
    buf[2] = (block_addr & 0xff00) >> 8;
    buf[3] = (block_addr & 0xff);
    if (spiTransfer(spi_ch, buf, buf, 4, 10) == true)
    {
      ret = true;
    }

  }
  w25qxx_csPinWrite(_DEF_HIGH);

  ret = w25qxx_WaitForWriteCplt();
  if (ret == false)
  {
    logPrintf("w25q16 : status check timeout fail! ");
  }
  delay(1);
  w25qxx.lock = DISABLE;
  return ret;
}



uint32_t w25qxx_PageToSector(uint32_t page_addr)
{
  return ((page_addr * w25qxx.page_size) / w25qxx.sector_size);
}

uint32_t w25qxx_PageToBlock(uint32_t page_addr)
{
  return ((page_addr * w25qxx.page_size) / w25qxx.block_size);
}

uint32_t w25qxx_SectorToBlock(uint32_t sector_addr)
{
  return ((sector_addr * w25qxx.sector_size) / w25qxx.block_size);
}


uint32_t w25qxx_SectorToPage(uint32_t sector_addr)
{
  return (sector_addr * w25qxx.sector_size) / w25qxx.page_size;
}


uint32_t w25qxx_BlockToPage(uint32_t block_addr)
{
  return (block_addr * w25qxx.block_size) / w25qxx.page_size;
}


bool w25qxx_IsEmptyPage(uint32_t page_addr, uint32_t offset, uint32_t length)
{
  bool ret = false;

  while (w25qxx.lock == ENABLE)
    delay(1);
  w25qxx.lock = ENABLE;

  ret = w25qxx_WaitForWriteCplt();
  if (ret == false)
  {
    logPrintf("w25q16 : status check timeout fail! ");
    w25qxx.lock = DISABLE;
    return ret;
  }
  bool is_morethan = ((length + offset) > w25qxx.page_size);
  bool is_pagezero = length == 0;

  if (is_morethan || is_pagezero)
    length = w25qxx.page_size - offset;

  uint8_t buf[32];
  uint32_t work_addr;
  uint32_t i;
  for (i = offset; i < w25qxx.page_size; i += sizeof(buf))
  {
    w25qxx_csPinWrite(_DEF_LOW);
    work_addr = (i + page_addr * w25qxx.page_size);

    if (w25qxx.id == W25Q_ID_256 || w25qxx.id ==W25Q_ID_512 )
    {
      //TODO:
      //spiTransfer8(spi_ch,0x0C);
      //spiTransfer8(spi_ch,((work_addr & 0xFF000000) >> 24);
    }
    else
    {
      buf[0] = FAST_READ_CMD;
      buf[1] = (work_addr & 0xff0000) >> 16;
      buf[2] = (work_addr & 0xff00) >> 8;
      buf[3] = (work_addr & 0xff);
      if (spiTransfer(spi_ch, buf, buf, 4, 10) == true)
      {
        ret = true;
      }
    }
    memset(&buf[0],0x00,sizeof(buf));
    spiTransfer8(spi_ch,0x00);
    if (spiTransfer(spi_ch, buf, buf, 8, 100) == true)
    {
      ret = true;
    }

    w25qxx_csPinWrite(_DEF_HIGH);
    for (uint8_t x = 0; x < sizeof(buf); x++)
    {
      if (buf[x] != 0xFF)
        goto NOT_EMPTY;
    }
  }
  if((w25qxx.page_size+offset)%sizeof(buf) != 0)
  {
    i -= sizeof(buf);
    for (;i < w25qxx.page_size; i++)
    {
      w25qxx_csPinWrite(_DEF_LOW);
      work_addr = (i + page_addr * w25qxx.page_size);

      if (w25qxx.id == W25Q_ID_256 || w25qxx.id ==W25Q_ID_512 )
      {
        //TODO:
        //spiTransfer8(spi_ch,0x0C);
        //spiTransfer8(spi_ch,((work_addr & 0xFF000000) >> 24);
      }
      else
      {
        buf[0] = FAST_READ_CMD;
        buf[1] = (work_addr & 0xff0000) >> 16;
        buf[2] = (work_addr & 0xff00) >> 8;
        buf[3] = (work_addr & 0xff);
        if (spiTransfer(spi_ch, buf, buf, 4, 10) == true)
        {
          ret = true;
        }
      }
      memset(&buf[0],0x00,sizeof(buf));
      spiTransfer8(spi_ch,0x00);
      if (spiTransfer(spi_ch, buf, buf, 1, 100) == true)
      {
        ret = true;
      }

      w25qxx_csPinWrite(_DEF_HIGH);
      if (buf[0] != 0xFF)
        goto NOT_EMPTY;
    }
  }

  w25qxx.lock = DISABLE;
  return true;

 NOT_EMPTY:
  w25qxx.lock = DISABLE;
  return false;
}


bool w25qxx_IsEmptySector(uint32_t sector_addr, uint32_t offset, uint32_t length)
{
  bool ret = false;
  while (w25qxx.lock == ENABLE)
    delay(1);
  w25qxx.lock = ENABLE;

  ret = w25qxx_WaitForWriteCplt();
  if (ret == false)
  {
    logPrintf("w25q16 : status check timeout fail! ");
    w25qxx.lock = DISABLE;
    return ret;
  }
  bool is_morethan = (length > w25qxx.sector_size);
  bool is_sectorzero = length == 0;

  if (is_morethan || is_sectorzero)
    length = w25qxx.sector_size;

  uint8_t buf[32];
  uint32_t work_addr;
  uint32_t i;
  for (i = offset; i < w25qxx.sector_size; i += sizeof(buf))
  {
    w25qxx_csPinWrite(_DEF_LOW);
    work_addr = (i + sector_addr * w25qxx.sector_size);
    if (w25qxx.id == W25Q_ID_256 || w25qxx.id ==W25Q_ID_512 )
    {
      //TODO:
      //spiTransfer8(spi_ch,0x0C);
      //spiTransfer8(spi_ch,((work_addr & 0xFF000000) >> 24);
    }
    else
    {
      buf[0] = FAST_READ_CMD;
      buf[1] = (work_addr & 0xff0000) >> 16;
      buf[2] = (work_addr & 0xff00) >> 8;
      buf[3] = (work_addr & 0xff);
      if (spiTransfer(spi_ch, buf, buf, 4, 10) == true)
      {
        ret = true;
      }
    }
    memset(&buf[0],0x00,sizeof(buf));
    spiTransfer8(spi_ch,0x00);
    if (spiTransfer(spi_ch, buf, buf, 8, 100) == true)
    {
      ret = true;
    }

    w25qxx_csPinWrite(_DEF_HIGH);
    for (uint8_t x = 0; x < sizeof(buf); x++)
    {
      if (buf[x] != 0xFF)
        goto NOT_EMPTY;
    }
  }
  if((w25qxx.sector_size+offset)%sizeof(buf) != 0)
  {
    i -= sizeof(buf);
    for (;i < w25qxx.sector_size; i++)
    {
      w25qxx_csPinWrite(_DEF_LOW);
      work_addr = (i + sector_addr * w25qxx.sector_size);

      if (w25qxx.id == W25Q_ID_256 || w25qxx.id ==W25Q_ID_512 )
      {
        //TODO:
        //spiTransfer8(spi_ch,0x0C);
        //spiTransfer8(spi_ch,((work_addr & 0xFF000000) >> 24);
      }
      else
      {
        buf[0] = FAST_READ_CMD;
        buf[1] = (work_addr & 0xff0000) >> 16;
        buf[2] = (work_addr & 0xff00) >> 8;
        buf[3] = (work_addr & 0xff);
        if (spiTransfer(spi_ch, buf, buf, 4, 10) == true)
        {
          ret = true;
        }
      }
      memset(&buf[0],0x00,sizeof(buf));
      spiTransfer8(spi_ch,0x00);
      if (spiTransfer(spi_ch, buf, buf, 1, 100) == true)
      {
        ret = true;
      }

      w25qxx_csPinWrite(_DEF_HIGH);
      if (buf[0] != 0xFF)
        goto NOT_EMPTY;
    }
  }
  w25qxx.lock = DISABLE;
  return true;

  NOT_EMPTY:
  w25qxx.lock = DISABLE;
  return false;
}


bool w25qxx_IsEmptyBlock(uint32_t block_addr, uint32_t offset, uint32_t length)
{
  bool ret = false;
  while (w25qxx.lock == ENABLE)
    delay(1);
  w25qxx.lock = ENABLE;

  ret = w25qxx_WaitForWriteCplt();
  if (ret == false)
  {
    logPrintf("w25q16 : status check timeout fail! ");
    w25qxx.lock = DISABLE;
    return ret;
  }
  bool is_morethan = (length > w25qxx.block_size);
  bool is_blockzero = length == 0;

  if (is_morethan || is_blockzero)
    length = w25qxx.block_size;

  uint8_t buf[32];
  uint32_t work_addr;
  uint32_t i;
  for (i = offset; i < w25qxx.block_size; i += sizeof(buf))
  {
    w25qxx_csPinWrite(_DEF_LOW);
    work_addr = (i + block_addr * w25qxx.block_size);
    if (w25qxx.id == W25Q_ID_256 || w25qxx.id ==W25Q_ID_512 )
    {
      //TODO:
      //spiTransfer8(spi_ch,0x0C);
      //spiTransfer8(spi_ch,((work_addr & 0xFF000000) >> 24);
    }
    else
    {
      buf[0] = FAST_READ_CMD;
      buf[1] = (work_addr & 0xff0000) >> 16;
      buf[2] = (work_addr & 0xff00) >> 8;
      buf[3] = (work_addr & 0xff);
      if (spiTransfer(spi_ch, buf, buf, 4, 10) == true)
      {
        ret = true;
      }
    }
    memset(&buf[0],0x00,sizeof(buf));
    spiTransfer8(spi_ch,0x00);
    if (spiTransfer(spi_ch, buf, buf, 8, 100) == true)
    {
      ret = true;
    }

    w25qxx_csPinWrite(_DEF_HIGH);
    for (uint8_t x = 0; x < sizeof(buf); x++)
    {
      if (buf[x] != 0xFF)
        goto NOT_EMPTY;
    }
  }
  if((w25qxx.block_size+offset)%sizeof(buf) != 0)
  {
    i -= sizeof(buf);
    for (;i < w25qxx.block_size; i++)
    {
      w25qxx_csPinWrite(_DEF_LOW);
      work_addr = (i + block_addr * w25qxx.block_size);

      if (w25qxx.id == W25Q_ID_256 || w25qxx.id ==W25Q_ID_512 )
      {
        //TODO:
        //spiTransfer8(spi_ch,0x0C);
        //spiTransfer8(spi_ch,((work_addr & 0xFF000000) >> 24);
      }
      else
      {
        buf[0] = FAST_READ_CMD;
        buf[1] = (work_addr & 0xff0000) >> 16;
        buf[2] = (work_addr & 0xff00) >> 8;
        buf[3] = (work_addr & 0xff);
        if (spiTransfer(spi_ch, buf, buf, 4, 10) == true)
        {
          ret = true;
        }
      }
      memset(&buf[0],0x00,sizeof(buf));
      spiTransfer8(spi_ch,0x00);
      if (spiTransfer(spi_ch, buf, buf, 1, 100) == true)
      {
        ret = true;
      }

      w25qxx_csPinWrite(_DEF_HIGH);
      if (buf[0] != 0xFF)
        goto NOT_EMPTY;
    }
  }
  w25qxx.lock = DISABLE;
  return true;

  NOT_EMPTY:
  w25qxx.lock = DISABLE;
  return false;

}


uint8_t w25qxx_WriteByte(uint8_t data, uint32_t addr)
{
  uint8_t ret = 0;
  while (w25qxx.lock == ENABLE)
    delay(1);
  w25qxx.lock = ENABLE;

  ret = w25qxx_WaitForWriteCplt();
  if (ret == false)
  {
    logPrintf("w25q16 : status check timeout fail! ");
    w25qxx.lock = DISABLE;
    return ret;
  }
  w25qxx_WriteEnable();
  w25qxx_csPinWrite(_DEF_LOW);

  if (w25qxx.id == W25Q_ID_256 || w25qxx.id ==W25Q_ID_512 )
  {
    //TODO:
    //spiTransfer8(spi_ch,0x12);
    //spiTransfer8(spi_ch,((addr & 0xFF000000) >> 24);
  }
  else
  {
    spiTransfer8(spi_ch,PAGE_PROG_CMD);
    spiTransfer8(spi_ch, (addr & 0xFF0000) >> 16);
    spiTransfer8(spi_ch, (addr & 0xFF00) >> 8);
    spiTransfer8(spi_ch, (addr & 0xFF));

    if (spiTransfer(spi_ch, &data, NULL, 1 , 100) == true)
    {
      ret = true;
    }
  }

  w25qxx_csPinWrite(_DEF_HIGH);

  if (w25qxx_WaitForWriteCplt() == false)
  {
    logPrintf("w25q16 : status check timeout fail! ");
    w25qxx.lock = DISABLE;
    return 0;
  }
  w25qxx.lock = DISABLE;
  return ret;
}



uint8_t w25qxx_WriteSubpage(uint8_t *p_data, uint32_t sub_no, uint32_t offset, uint32_t length)
{
  bool ret = false;
  while (w25qxx.lock == ENABLE)
    delay(1);
  w25qxx.lock = ENABLE;

  ret = w25qxx_WaitForWriteCplt();
  if (ret == false)
  {
    logPrintf("w25q16 : status check timeout fail! ");
    w25qxx.lock = DISABLE;
    return ret;
  }

  if ((offset < W25Qxx_SUB_PAGE_COUNT) == false)
    offset = offset % W25Qxx_SUB_PAGE_COUNT;

  bool is_morethan = ((length + offset) > W25Qxx_SUB_PAGE_COUNT /*w25qxx.page_size*/);
  bool is_pagezero = length == 0;

  if (is_morethan || is_pagezero)
    length = /*w25qxx.page_size*/ W25Qxx_SUB_PAGE_COUNT - offset;


  w25qxx_WriteEnable();
  w25qxx_csPinWrite(_DEF_LOW);

  spiTransfer8(spi_ch,PAGE_PROG_CMD);

  sub_no = sub_no*W25Qxx_SUB_PAGE_COUNT+ offset ;   /*(addr * w25qxx.page_size) + offset*/;
  spiTransfer8(spi_ch, (sub_no & 0xFF0000) >> 16);
  spiTransfer8(spi_ch, (sub_no & 0xFF00) >> 8);
  spiTransfer8(spi_ch, (sub_no & 0xFF));

  if (spiTransfer(spi_ch, p_data, NULL, length , 100) == true)
  {
    ret = true;
  }

  w25qxx_csPinWrite(_DEF_HIGH);

  if (w25qxx_WaitForWriteCplt() == false)
  {
    logPrintf("w25q16 : status check timeout fail! ");
    w25qxx.lock = DISABLE;
    return 0;
  }
  w25qxx.lock = DISABLE;
  return ret;

}



uint8_t w25qxx_WritePage(uint8_t *p_data, uint32_t addr, uint32_t offset, uint32_t length)
{
  bool ret = false;
  while (w25qxx.lock == ENABLE)
    delay(1);
  w25qxx.lock = ENABLE;

  ret = w25qxx_WaitForWriteCplt();
  if (ret == false)
  {
    logPrintf("w25q16 : status check timeout fail! ");
    w25qxx.lock = DISABLE;
    return ret;
  }

  bool is_morethan = ((length + offset) > w25qxx.page_size);
  bool is_pagezero = length == 0;

  if (is_morethan || is_pagezero)
    length = w25qxx.page_size - offset;


  w25qxx_WriteEnable();
  w25qxx_csPinWrite(_DEF_LOW);

  uint8_t buf[4] = {0,};
  spiTransfer8(spi_ch,PAGE_PROG_CMD);

  addr = (addr * w25qxx.page_size) + offset;
  buf[0] = spiTransfer8(spi_ch, (addr & 0xFF0000) >> 16);
  buf[0] = spiTransfer8(spi_ch, (addr & 0xFF00) >> 8);
  buf[0] = spiTransfer8(spi_ch, (addr & 0xFF));

  if (spiTransfer(spi_ch, p_data, NULL, length , 100) == true)
  {
    ret = true;
  }
  UNUSED(buf[4]);

#if 0
  addr = (addr * w25qxx.page_size) + offset;


  if (w25qxx.id == W25Q_ID_256 || w25qxx.id ==W25Q_ID_512 )
  {
    //TODO:
    //spiTransfer8(spi_ch,0x12);
    //spiTransfer8(spi_ch,((addr & 0xFF000000) >> 24);
  }
  else
  {
    buf[0] = PAGE_PROG_CMD;
    buf[1] = (addr & 0xff0000) >> 16;
    buf[2] = (addr & 0xff00) >> 8;
    buf[3] = (addr & 0xff);
    if (spiTransfer(spi_ch, buf, buf, 4, 10) == true)
    {
      ret = true;
    }

    if (spiTransfer(spi_ch, p_data, NULL, length , 10) == true)
    {
      ret = true;
    }
  }
#endif


  w25qxx_csPinWrite(_DEF_HIGH);

  if (w25qxx_WaitForWriteCplt() == false)
  {
    logPrintf("w25q16 : status check timeout fail! ");
    w25qxx.lock = DISABLE;
    return 0;
  }
  w25qxx.lock = DISABLE;
  return ret;

}



uint8_t w25qxx_WriteSector(uint8_t *p_data, uint32_t addr, uint32_t offset, uint32_t length)
{
  uint8_t ret = 0;
  bool is_morethan = ((length) > w25qxx.sector_size);
  bool is_sectorzero = length == 0;

  if (is_morethan || is_sectorzero)
    length = w25qxx.sector_size;

  if (offset >= w25qxx.sector_size)
    return 0;

  uint32_t start_page;
  int32_t bytes2write;
  uint32_t local_offset;
  if ((offset + length) > w25qxx.sector_size)
    bytes2write = w25qxx.sector_size - offset;
  else
    bytes2write = length;

  start_page = w25qxx_SectorToPage(addr) + (offset / w25qxx.page_size);
  local_offset = offset % w25qxx.page_size;
  do
  {
    ret = w25qxx_WritePage(p_data, start_page, local_offset, bytes2write);
    start_page++;
    bytes2write -= w25qxx.page_size - local_offset;
    p_data += w25qxx.page_size - local_offset;
    local_offset = 0;
  } while (bytes2write > 0);

  return ret;
}


void w25qxx_ReadByte(uint8_t *p_data, uint32_t addr)
{
  uint8_t buf = 0;
  while (w25qxx.lock == ENABLE)
    delay(1);
  w25qxx.lock = ENABLE;
  w25qxx_csPinWrite(_DEF_LOW);
  if (w25qxx.id == W25Q_ID_256 || w25qxx.id ==W25Q_ID_512 )
  {
    //TODO:
    //spiTransfer8(spi_ch,0x0C);
    //spiTransfer8(spi_ch,((addr & 0xFF000000) >> 24);
  }
  else
  {
    spiTransfer8(spi_ch,FAST_READ_CMD);
    spiTransfer8(spi_ch, (addr & 0xFF0000) >> 16);
    spiTransfer8(spi_ch, (addr & 0xFF00) >> 8);
    spiTransfer8(spi_ch, (addr & 0xFF));
    spiTransfer8(spi_ch, 0x00);

    if (spiTransfer(spi_ch, NULL, &buf, 1, 10) == true)
    {
      *p_data = buf;
    }
  }
  w25qxx_csPinWrite(_DEF_HIGH);
  w25qxx.lock = DISABLE;
}



void w25qxx_ReadBytes(uint8_t *p_data, uint32_t addr, uint32_t length)
{
  while (w25qxx.lock == ENABLE)
    delay(1);
  w25qxx.lock = ENABLE;
  w25qxx_csPinWrite(_DEF_LOW);
  if (w25qxx.id == W25Q_ID_256 || w25qxx.id ==W25Q_ID_512 )
  {
    //TODO:
    //spiTransfer8(spi_ch,0x0C);
    //spiTransfer8(spi_ch,((addr & 0xFF000000) >> 24);
  }
  else
  {
    spiTransfer8(spi_ch,FAST_READ_CMD);
    spiTransfer8(spi_ch, (addr & 0xFF0000) >> 16);
    spiTransfer8(spi_ch, (addr & 0xFF00) >> 8);
    spiTransfer8(spi_ch, (addr & 0xFF));
    spiTransfer8(spi_ch, 0x00);

    if (spiTransfer(spi_ch, NULL, p_data, length, 100) == false)
    {
      logPrintf("Read byte Fail!");
    }
  }
  w25qxx_csPinWrite(_DEF_HIGH);
  w25qxx.lock = DISABLE;
}

void w25qxx_ReadPage(uint8_t *p_data, uint32_t addr, uint32_t offset, uint32_t length)
{
  uint8_t buf[4] ={0,};
  while (w25qxx.lock == ENABLE)
    delay(1);
  w25qxx.lock = ENABLE;

  bool is_morethan = (length > w25qxx.page_size);
  bool is_pagezero = length == 0;

  if (is_morethan || is_pagezero)
    length = w25qxx.page_size;

  if ((offset + length) > w25qxx.page_size)
    length = w25qxx.page_size - offset;

  addr = addr * w25qxx.page_size + offset;

  w25qxx_csPinWrite(_DEF_LOW);

  if (w25qxx.id == W25Q_ID_256 || w25qxx.id ==W25Q_ID_512 )
  {
    //TODO:
    //spiTransfer8(spi_ch,0x0C);
    //spiTransfer8(spi_ch,((addr & 0xFF000000) >> 24);
  }
  else
  {
    buf[0] = FAST_READ_CMD;
    buf[1] = (addr & 0xff0000) >> 16;
    buf[2] = (addr & 0xff00) >> 8;
    buf[3] = (addr & 0xff);
    if (spiTransfer(spi_ch, buf, buf, 4, 10) == false)
    {
      logPrintf("w25qxx_ReadPage Fail!");
    }
    memset(&buf[0],0x00,sizeof(buf));
    spiTransfer8(spi_ch,0x00);
    if (spiTransfer(spi_ch, NULL, p_data, length, 100) == false)
    {
      logPrintf("w25qxx_ReadPage Fail!");
    }
  }
  w25qxx_csPinWrite(_DEF_HIGH);
  w25qxx.lock = DISABLE;

}


void w25qxx_ReadSector(uint8_t *p_data, uint32_t addr, uint32_t offset, uint32_t length)
{
  bool is_morethan = (length > w25qxx.sector_size);
  bool is_sectorzero = length == 0;

  if (is_morethan || is_sectorzero)
    length = w25qxx.sector_size;

  /*if ((offset + length) >= w25qxx.sector_size)
    return;*/

  uint32_t start_page;
  int32_t bytes2read;
  uint32_t local_offset;

  if ((offset + length) > w25qxx.sector_size)
    bytes2read =  w25qxx.sector_size - offset;
  else
    bytes2read = length;

  start_page = w25qxx_SectorToPage(addr) + (offset / w25qxx.page_size);
  local_offset = offset % w25qxx.page_size;
  do
  {
    w25qxx_ReadPage(p_data, start_page, local_offset, bytes2read);
    start_page++;
    bytes2read -= w25qxx.page_size - local_offset;
    p_data += w25qxx.page_size - local_offset;
    local_offset = 0;
  } while (bytes2read > 0);

}




void w25qxx_ReadBlock(uint8_t *p_data, uint32_t addr, uint32_t offset, uint32_t length)
{
  bool is_morethan = (length > w25qxx.block_size);
  bool is_blockzero = length == 0;

  if (is_morethan || is_blockzero)
    length = w25qxx.block_size;

  if (offset >= w25qxx.block_size)
    return;

  uint32_t start_page;
  int32_t bytes2read;
  uint32_t local_offset;

  if ((offset + length) > w25qxx.block_size)
    bytes2read =  w25qxx.block_size - offset;
  else
    bytes2read = length;

  start_page = w25qxx_BlockToPage(addr) + (offset / w25qxx.page_size);
  local_offset = offset % w25qxx.page_size;
  do
  {
    w25qxx_ReadPage(p_data, start_page, local_offset, bytes2read);
    start_page++;
    bytes2read -= w25qxx.page_size - local_offset;
    p_data += w25qxx.page_size - local_offset;
    local_offset = 0;
  } while (bytes2read > 0);

}





uint8_t w25qxx_ReadId(void)
{

  uint8_t ret = 0;

  /*uint32_t Temp = 0, Temp0 = 0, Temp1 = 0, Temp2 = 0;
  w25qxx_csPinWrite(_DEF_LOW);
  spiTransfer8(spi_ch,0x9F);
  Temp0 = (uint32_t)spiTransfer8(spi_ch,W25Q_DUMMY_BYTE);
  Temp1 = (uint32_t)spiTransfer8(spi_ch,W25Q_DUMMY_BYTE);
  Temp2 = (uint32_t)spiTransfer8(spi_ch,W25Q_DUMMY_BYTE);
  w25qxx_csPinWrite(_DEF_HIGH);
  Temp = (Temp0 << 16) | (Temp1 << 8) | Temp2;
  return (uint8_t)Temp;*/

  uint8_t tx[4] ={0,};
  uint8_t rx[4] ={0,};

  tx[0] = READ_ID_CMD;

  w25qxx_csPinWrite(_DEF_LOW);
  if (spiTransfer(spi_ch, tx, rx, 4, 10) == true)
  {
    ret = rx[3];
  }
  w25qxx_csPinWrite(_DEF_HIGH);

  return ret;
}

uint8_t w25qxx_ReadUniqId(void)
{
  uint8_t ret = 0;
  uint8_t buf[8] ={0,};
  w25qxx_csPinWrite(_DEF_LOW);

  ret = spiTransfer8(spi_ch,READ_OTP_ARRAY_CMD);
  for (uint8_t i = 0; i < 4; i++)
    spiTransfer8(spi_ch,W25Q_DUMMY_BYTE);

  if (spiTransfer(spi_ch, buf, buf, 8, 10) == true)
  {
    for (uint8_t i = 0; i < 8; i++)
      w25qxx.uniq_id[i] = buf[i];

    ret = true;
  }

  /*
  spiTransfer8(spi_ch,READ_OTP_ARRAY_CMD);
  for (uint8_t i = 0; i < 4; i++)
    spiTransfer8(spi_ch,W25Q_DUMMY_BYTE);
  for (uint8_t i = 0; i < 8; i++)
    w25qxx.uniq_id[i] = spiTransfer8(spi_ch, W25Q_DUMMY_BYTE);
   */


  w25qxx_csPinWrite(_DEF_HIGH);
  return ret;
}



uint8_t w25qxx_ReadStatusReg(uint8_t reg_no)
{
  uint8_t ret = 0;
  uint8_t buf[2] ={0,};

  switch(reg_no)
  {
    case 1: buf[0] = READ_STATUS_REG_CMD; break;
    case 2: buf[0] = READ_STATUS_REG2_CMD; break;
    case 3: buf[0] = READ_STATUS_REG3_CMD; break;
  }

  w25qxx_csPinWrite(_DEF_LOW);
  if (spiTransfer(spi_ch, buf, buf, 2, 10) == true)
  {
    ret = buf[1];
  }

  //buf[1] = spiTransfer8(spi_ch,buf[0]);
  //ret = spiTransfer8(spi_ch,W25Q_DUMMY_BYTE);

  w25qxx_csPinWrite(_DEF_HIGH);

  return ret;
}


void w25qxx_WriteEnable(void)
{
  w25qxx_csPinWrite(_DEF_LOW);
  spiTransfer8(spi_ch,WRITE_ENABLE_CMD);
  w25qxx_csPinWrite(_DEF_HIGH);
  delay(1);
}

/*void w25qxx_WriteDisable(void)
{
  w25qxx_csPinWrite(_DEF_LOW);
  spiTransfer8(spi_ch,WRITE_DISABLE_CMD);
  w25qxx_csPinWrite(_DEF_HIGH);
  delay(1);
}*/



bool w25qxx_WaitForWriteCplt(void)
{
  uint8_t buf[2] = {0,};
  uint8_t timer = tickTimer_Start();
  bool ret = true;

  delay(1);
  w25qxx_csPinWrite(_DEF_LOW);
  buf[0] = READ_STATUS_REG_CMD;
  if (spiTransfer(spi_ch, buf, buf, 2, 10) == true)
  {
    w25qxx.status_reg1 = (w25qStatus)buf[1];
  }

  do
  {
    w25qxx.status_reg1 = spiTransfer8(spi_ch,W25Q_DUMMY_BYTE);
    delay(1);
    if (tickTimer_MoreThan(timer, 1000))
    {
      ret = false;
      break;
    }
  } while ( w25qxx.status_reg1 == W25Q_SR_BUSY);
  w25qxx_csPinWrite(_DEF_HIGH);

  return ret;
}



#ifdef _USE_HW_CLI
void w25qxxInfo(void)
{
  cliPrintf("is_init\t: %d\n", is_init);
  cliPrintf("W25Qxx information\n");
  cliPrintf("W25Q Chip ID \t\t: ");
  switch (w25qxx.id)
  {
    case W25Q_ID_512: cliPrintf("W25Q_ID_512\n");break;
    case W25Q_ID_256: cliPrintf("W25Q_ID_256\n");break;
    case W25Q_ID_128: cliPrintf("W25Q_ID_128\n");break;
    case W25Q_ID_64:  cliPrintf("W25Q_ID_64\n");break;
    case W25Q_ID_32:  cliPrintf("W25Q_ID_32\n");break;
    case W25Q_ID_16:  cliPrintf("W25Q_ID_16\n");break;
    case W25Q_ID_80:  cliPrintf("W25Q_ID_80\n");break;
    case W25Q_ID_40:  cliPrintf("W25Q_ID_40\n");break;
    case W25Q_ID_20:  cliPrintf("W25Q_ID_20\n");break;
    case W25Q_ID_10:  cliPrintf("W25Q_ID_512\n");break;
    default: cliPrintf("W25Q_ID : read id fail!");break;
  }
  cliPrintf("Unique ID    \t\t: [0x%02X][0x%02X]", w25qxx.uniq_id[7],w25qxx.uniq_id[6]);
  cliPrintf("[0x%02X][0x%02X][0x%02X]", w25qxx.uniq_id[5],w25qxx.uniq_id[4],w25qxx.uniq_id[3]);
  cliPrintf("[0x%02X][0x%02X][0x%02X]  \n", w25qxx.uniq_id[2],w25qxx.uniq_id[1],w25qxx.uniq_id[0]);
  cliPrintf("Page Size    \t\t: %d \n", w25qxx.page_size);
  cliPrintf("Page Count   \t\t: %d \n", w25qxx.page_cnt);
  cliPrintf("Sector Size  \t\t: %d \n", w25qxx.sector_size);
  cliPrintf("Sector Count \t\t: %d \n", w25qxx.sector_cnt);
  cliPrintf("Block Size   \t\t: %d \n", w25qxx.block_size);
  cliPrintf("Block Count  \t\t: %d \n", w25qxx.block_cnt);
  cliPrintf("Capacity[KB] \t\t: %d \n", w25qxx.capacityKb);
  cliPrintf("Status_Reg[1]\t\t: 0x%02X \n", (uint8_t)w25qxx.status_reg1);
  cliPrintf("Status_Reg[2]\t\t: 0x%02X \n", (uint8_t)w25qxx.status_reg2);
  cliPrintf("Status_Reg[3]\t\t: 0x%02X \n", (uint8_t)w25qxx.status_reg3);

#if 0
  w25qxx.lock = ENABLE;
  w25qxx_csPinWrite(_DEF_HIGH);

  uint32_t id;
  id = w25qxx_ReadId();

  switch (id)
  {
    case W25Q_ID_512:
      cliPrintf("W25Q_ID_512\n");
      break;
    case W25Q_ID_256:
      cliPrintf("W25Q_ID_256\n");
      break;
    case W25Q_ID_128:
      cliPrintf("W25Q_ID_128\n");
      break;
    case W25Q_ID_64:
      cliPrintf("W25Q_ID_64\n");
      break;
    case W25Q_ID_32:
      cliPrintf("W25Q_ID_32\n");
      break;
    case W25Q_ID_16:
      cliPrintf("W25Q_ID_16\n");
      break;
    case W25Q_ID_80:
      cliPrintf("W25Q_ID_80\n");
      break;
    case W25Q_ID_40:
      cliPrintf("W25Q_ID_40\n");
      break;
    case W25Q_ID_20:
      cliPrintf("W25Q_ID_20\n");
      break;
    case W25Q_ID_10:
      cliPrintf("W25Q_ID_512\n");
      break;
    default:
      cliPrintf("W25Q_ID : read id fail!");
  }

  w25qxx.lock = DISABLE;
#endif

}

void cliW25QXX(cli_args_t *args)
{
  uint32_t i;
  bool ret = false;
  uint32_t addr;
  uint32_t offset;
  uint32_t length;
  //uint8_t  data;
  char *pdata;
  uint32_t pre_time;

  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    w25qxxInfo();
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "reset") == true)
  {
    w25qxx_Init();

    if (is_init == true)
    {
      cliPrintf("w25q reset OK\n");
      ret = true;
    }
    else
    {
      cliPrintf("w25q reset Fail\n");
    }
  }

  if (args->argc == 1 && args->isStr(0, "erase") == true)
  {
    if (w25qxx_EraseChip() == true)
    {
      cliPrintf("w25q Erase OK\n");
      ret = true;
    }
    else
    {
      cliPrintf("w25q Erase Fail\n");
    }
  }

  if (args->argc == 2 && args->isStr(0, "erase_sector") == true)
  {
    addr = (uint32_t)args->getData(1);
    if (w25qxx_EraseSector(addr) == true)
    {
      cliPrintf("w25q Erase Sector OK\n");
      ret = true;
    }
    else
    {
      cliPrintf("w25q Erase Sector Fail\n");
    }
  }

  if (args->argc == 2 && args->isStr(0, "erase_block") == true)
  {
    addr = (uint32_t)args->getData(1);
    if (w25qxx_EraseBlock(addr) == true)
    {
      cliPrintf("w25q Erase Block OK\n");
      ret = true;
    }
    else
    {
      cliPrintf("w25q Erase Block Fail\n");
    }
  }

  if (args->argc == 3 && args->isStr(0, "read") == true)
  {
    addr   = (uint32_t)args->getData(1);
    length = (uint32_t)args->getData(2);
    pdata =(char *)malloc(sizeof(char) * length);
    if (length > w25qxx.block_size)
    {
      cliPrintf( "length error\n");
    }
    else
    {
      w25qxx_ReadBytes((uint8_t*)pdata, addr , length);
      for (i=0; i<length; i++)
      {
        //w25qxx_ReadByte(&data, addr+i);
        cliPrintf( "addr : %d\t0x%02X |", addr + i, pdata[i]);
        if (pdata[i] > 0x1f && pdata[i] < 0x7f)
          cliPrintf("%c", pdata[i]);
        else
          cliPrintf(".");
        cliPrintf("| \n");
      }
      ret = true;
    }
    free(pdata);
  }


  if (args->argc == 2 && args->isStr(0, "read_sector") == true)
  {
    addr   = (uint32_t)args->getData(1);
    uint8_t buff[256] ={0,};
    uint32_t n = 16;

    cliPrintf("  ");

    for(int t =0; t<16; t++)
    {
      memset(&buff[0],0x00,256);
      w25qxx_ReadSector(&buff[0], addr, t*w25qxx.page_size, 256);
      cliPrintf( "page[%02d]\n  ", t);
      for (i=0; i<16; i++)
      {
        cliPrintf( "     0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X "
            ,buff[i*n]
            ,buff[(i*n)+1]
            ,buff[(i*n)+2]
            ,buff[(i*n)+3]
            ,buff[(i*n)+4]
            ,buff[(i*n)+5]
            ,buff[(i*n)+6]
            ,buff[(i*n)+7]);
        cliPrintf (" |");
        for ( int idx = 0;  idx < 8; idx++)
        {
          if (buff[(i*n)+idx] > 0x1f && buff[(i*n)+idx] < 0x7f)
            cliPrintf("%c", buff[(i*n)+idx]);
          else
            cliPrintf(".");
        }
        cliPrintf("|\n  ");
        cliPrintf( "     0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X "
            ,buff[(i*n)+8]
            ,buff[(i*n)+9]
            ,buff[(i*n)+10]
            ,buff[(i*n)+11]
            ,buff[(i*n)+12]
            ,buff[(i*n)+13]
            ,buff[(i*n)+14]
            ,buff[(i*n)+15]);
        cliPrintf (" |");
        for ( int idx = 8;  idx < 16; idx++)
        {
          if (buff[(i*n)+idx] > 0x1f && buff[(i*n)+idx] < 0x7f)
            cliPrintf("%c", buff[(i*n)+idx]);
          else
            cliPrintf(".");
        }
        cliPrintf("|\n  ");
      }
    }

    ret = true;
  }


  if (args->argc == 2 && args->isStr(0, "read_page") == true)
  {
    addr   = (uint32_t)args->getData(1);
    uint8_t buff[256] ={0,};

    w25qxx_ReadPage(&buff[0], addr, 0, 256);
    uint32_t n = 16;
    cliPrintf("  ");
    for (i=0; i<16; i++)
    {
      cliPrintf( "Sub [%02d]  ", i);
      cliPrintf( "0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X "
          ,buff[i*n]
          ,buff[(i*n)+1]
          ,buff[(i*n)+2]
          ,buff[(i*n)+3]
          ,buff[(i*n)+4]
          ,buff[(i*n)+5]
          ,buff[(i*n)+6]
          ,buff[(i*n)+7]);
      cliPrintf (" |");
      for ( int idx = 0;  idx < 8; idx++)
      {
        if (buff[(i*n)+idx] > 0x1f && buff[(i*n)+idx] < 0x7f)
        {
          cliPrintf("%c", buff[(i*n)+idx]);
        }
        else
        {
          cliPrintf(".");
        }
      }
      cliPrintf("|\n  ");
      cliPrintf( "          0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X "
          ,buff[(i*n)+8]
          ,buff[(i*n)+9]
          ,buff[(i*n)+10]
          ,buff[(i*n)+11]
          ,buff[(i*n)+12]
          ,buff[(i*n)+13]
          ,buff[(i*n)+14]
          ,buff[(i*n)+15]);
      cliPrintf (" |");
      for ( int idx = 8;  idx < 16; idx++)
      {
        if (buff[(i*n)+idx] > 0x1f && buff[(i*n)+idx] < 0x7f)
          cliPrintf("%c", buff[(i*n)+idx]);
        else
          cliPrintf(".");
      }
      cliPrintf("|\n  ");
    }
    ret = true;
  }


  if (args->argc == 3 && args->isStr(0, "write") == true)
  {
    addr = (uint32_t)args->getData(1);
    pdata = args->getStr(2);

    pre_time = millis();
    if (w25qxx_WriteByte(pdata[0], addr))
    {
      cliPrintf( "addr : %d\t 0x%02X %dms\n", addr, pdata[0], millis()-pre_time);
      ret = true;
    }

  }

  if (args->argc == 4 && args->isStr(0, "write_sector") == true)
  {
    addr = (uint32_t)args->getData(1);
    offset = (uint8_t)args->getData(2);
    pdata = args->getStr(3);
    if (addr > w25qxx.sector_cnt)
    {
      cliPrintf( "length error [max sector_cnt %d]\n",w25qxx.sector_cnt);
    }
    else
    {
      pre_time = millis();
      w25qxx_WriteSector((uint8_t*)pdata, addr, offset, strlen(pdata));
      cliPrintf( "addr : %d\t 0x%02X %dms\n", addr, pdata[0], millis()-pre_time);

      ret = true;
    }
  }

  if (args->argc == 4 && args->isStr(0, "write_page") == true)
  {
    addr = (uint32_t)args->getData(1);
    offset = (uint8_t)args->getData(2);
    pdata = args->getStr(3);
    if (addr > w25qxx.page_cnt)
    {
      cliPrintf( "length error [max page %d]\n",w25qxx.page_cnt);
    }
    else
    {
      pre_time = millis();
      w25qxx_WritePage((uint8_t*)pdata, addr, offset, strlen(pdata));
      cliPrintf( "addr : %d\t 0x%02X %dms\n", addr, pdata[0], millis()-pre_time);

      ret = true;
    }
  }

  if (args->argc == 4 && args->isStr(0, "write_subpage") == true)
  {
    addr = (uint32_t)args->getData(1);
    offset = (uint8_t)args->getData(2);
    pdata = args->getStr(3);
    if (addr > w25qxx.page_cnt * W25Qxx_SUB_PAGE_COUNT)
    {
      cliPrintf( "length error [max page %d]\n"
          ,w25qxx.page_cnt * W25Qxx_SUB_PAGE_COUNT);
    }
    else
    {
      pre_time = millis();
      w25qxx_WriteSubpage((uint8_t*)pdata, addr, offset, strlen(pdata));
      cliPrintf( "subpage : %d\t 0x%02X %dms\n", addr, pdata[0], millis()-pre_time);
      ret = true;
    }
  }

  if (ret != true)
  {
    cliPrintf( "w25qxx info\n");
    cliPrintf( "w25qxx reset\n");
    cliPrintf( "w25qxx erase\n");
    cliPrintf( "w25qxx erase_block [block 0:31]\n");
    cliPrintf( "w25qxx erase_sector [sector 0:511]\n");

    cliPrintf( "w25qxx read [addr] [length]\n");
    cliPrintf( "w25qxx read_sector [sector 0:511]\n");
    cliPrintf( "w25qxx read_page [page 0:8191]\n");

    cliPrintf( "w25qxx write [addr] [data]\n");
    cliPrintf( "w25qxx write_sector [sector 0:511] [0ffset] [data]\n");
    cliPrintf( "w25qxx write_page [page 0:8191] [0ffset] [data]\n");
    cliPrintf( "w25qxx write_subpage [subpage 0:131071] [0ffset] [data]\n");
  }
}
#endif
























#endif
