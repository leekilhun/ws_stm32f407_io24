/*
 * mcp23017.c
 *
 *  Created on: Nov 27, 2021
 *      Author: gns2l
 */


#include "mcp23017.h"
#include "cli.h"


#ifdef _USE_MODULE_MCP23017

static bool is_init[MCP23017_MAX_CH];
const uint8_t i2c_ch = _DEF_I2C1;
//static uint8_t i2c_addr = (MCP23017_ADDRESS >>1);

#ifdef _USE_HW_CLI
static void cliMCP23017(cli_args_t *args);
#endif


static void mcp23017reset();

/*

static void mcp23017closeI2c();
static void mcp23017pinMode(uint8_t p, uint8_t d);
static void mcp23017digitalWrite(uint8_t p, uint8_t d);
static void mcp23017pullUp(uint8_t p, uint8_t d);
static bool mcp23017digitalRead(uint8_t p);

static void mcp23017writeGPIOAB(uint16_t);
static uint16_t mcp23017readGPIOAB();
static uint8_t mcp23017readGPIO(uint8_t b);

static void mcp23017setupInterrupts(uint8_t mirroring, uint8_t open, uint8_t polarity);
static void mcp23017setupInterruptPin(uint8_t p, uint8_t mode);
static uint8_t mcp23017getLastInterruptPin();
static uint8_t mcp23017getLastInterruptPinValue();


static uint8_t mcp23017bitForPin(uint8_t pin);
static uint8_t mcp23017regForPin(uint8_t pin, uint8_t portAaddr, uint8_t portBaddr);

static uint8_t mcp23017readRegister(uint8_t addr);
static uint8_t mcp23017writeRegister(uint8_t addr, uint8_t value);
static uint8_t mcp23017readByte();
static uint8_t mcp23017writeByte(uint8_t value);

static void mcp23017updateRegisterBit(uint8_t p, uint8_t pValue, uint8_t portAaddr, uint8_t portBaddr);

static bool mcp23017bitRead(uint8_t num, uint8_t index);

static void mcp23017bitWrite(uint8_t *var, uint8_t index, uint8_t bit);

*/













bool mcp23017Init(void)
{
  bool ret = false;

  mcp23017reset();

  ret = i2cBegin(i2c_ch, I2C_FREQ_400KHz);
  //spiAttachTxInterrupt(spi_ch, TransferDoneISR);
  for (int ch=0; ch<MCP23017_MAX_CH; ch++ )
  {

  }

#ifdef _USE_HW_CLI
  cliAdd("mcp23017", cliMCP23017);
#endif
  return ret;
}


uint8_t mcp23017ReadByte(uint32_t addr)
{
  return 0;
}
uint8_t mcp23017WriteByte(uint32_t addr, uint8_t data_in)
{
  return 0;
}



void mcp23017reset()
{
  gpioPinWrite(_GPIO_EX_IO_RST,_DEF_LOW);
  delay(50);
  gpioPinWrite(_GPIO_EX_IO_RST,_DEF_HIGH);
  delay(50);
  gpioPinWrite(_GPIO_EX_IO_RST,_DEF_LOW);

}







#ifdef _USE_HW_CLI

void mcp23017Info(uint8_t ch)
{
  cliPrintf("is_init[ch%d]\t: %d\n", ch, is_init[ch]);
}

void cliMCP23017(cli_args_t *args)
{
  bool ret = false;
  uint8_t ch;

  if (args->argc == 2 && args->isStr(1, "info") == true)
  {
    ch = (uint8_t)args->getData(0);
    mcp23017Info(ch);
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "reset") == true)
  {
    mcp23017Init();

    for (int ch=0; ch<MCP23017_MAX_CH; ch++ )
    {
      if (is_init[ch] == true)
      {
        cliPrintf("ch%d reset OK\n", ch);
      }
      else
      {
        cliPrintf("ch%d reset Fail\n", ch);
      }
    }

    ret = true;
  }


  if (ret != true)
  {
    cliPrintf("mcp23017 ch[0~%d] info\n", MCP23017_MAX_CH-1);
    cliPrintf("mcp23017 reset\n");
  }
}
#endif

#endif
