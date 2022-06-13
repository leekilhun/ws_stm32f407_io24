/*
 * i2c_io.c
 *
 *  Created on: 2022. 6. 5.
 *      Author: gns2l
 */

#include "i2c_io.h"
#include "cli.h"



#ifdef _USE_HW_I2C_IO

#ifdef _USE_HW_CLI
static void cliI2CIO(cli_args_t *args);
#endif


#define SCL_H       LL_GPIO_SetOutputPin(GPIOC , LL_GPIO_PIN_2)
#define SCL_L       LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_2)

#define SDA_H       LL_GPIO_SetOutputPin(GPIOC , LL_GPIO_PIN_3)
#define SDA_L       LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_3)

#define SCL_READ    LL_GPIO_IsInputPinSet(GPIOC,LL_GPIO_PIN_2)
#define SDA_READ    LL_GPIO_IsInputPinSet(GPIOC,LL_GPIO_PIN_3)

#define SDA_IN()    {GPIOC->MODER&=~(3<<(3*2));GPIOC->MODER|=0<<(3*2);}
#define SDA_OUT()   {GPIOC->MODER&=~(3<<(3*2));GPIOC->MODER|=1<<(3*2);}


#define I2C_PageSize  8  /* 24C02 */
#define ADDR_24LC02   0xA0

#define Test_IICadd 6
#define Test_IICtimes 12

#define AT24C01   127
#define AT24C02   255
#define AT24C04   511
#define AT24C08   1023
#define AT24C16   2047
#define AT24C32   4095
#define AT24C64   8191
#define AT24C128  16383
#define AT24C256  32767

#define EE_TYPE AT24C16

static bool is_init = false;

static void i2c_Start(void);
static void i2c_delayUs(uint32_t us);
static void i2c_Stop(void);
static void i2c_Ack(void);
static void i2c_NoAck(void);
static uint8_t i2c_WaitAck(void);

static void i2c_SendByte(uint8_t data);
static uint8_t i2c_ReceiveByte(void);
static uint8_t i2c_Check(void);
static bool i2c_IsDeviceReady(uint8_t dev_addr);


bool i2cIoInit(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOC_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;


  /* Test */
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);

  SDA_L;
  SDA_H;
  SCL_L;
  SCL_H;


  i2c_Ack();
  i2c_NoAck();

  if (i2c_Check())
  {

#ifdef _USE_HW_CLI
  cliAdd("i2cIo", cliI2CIO);
#endif

    is_init = true;
  }

  SDA_H;
  SCL_H;

  return is_init;
}


void i2c_delayUs(uint32_t us)
{
  volatile uint32_t i;

  for (i=0; i<us*1000; i++)
  {

  }
}


void i2c_Start(void)
{
  SDA_OUT();
  SDA_H;
  SCL_H;
  i2c_delayUs(Test_IICtimes);// 70//START:when CLK is high,DATA change form high to low
  SDA_L;
  i2c_delayUs(Test_IICtimes);//70
  SCL_L;
  //i2c_delayUs(Test_IICtimes);//70
}


void i2c_Stop(void)
{
  SDA_OUT();//sda
  SCL_L;
  i2c_delayUs(Test_IICtimes);//70  STOP:when CLK is high DATA change form low to high
  SDA_L;
  i2c_delayUs(Test_IICtimes);//70
  SCL_H;
  i2c_delayUs(Test_IICtimes);//70
  SDA_H;
  i2c_delayUs(Test_IICtimes);//70
}

void i2c_Ack(void)
{
  SCL_L;
  SDA_OUT();
  i2c_delayUs(Test_IICtimes);//70
  SDA_L;
  i2c_delayUs(Test_IICtimes);//70
  SCL_H;
  i2c_delayUs(Test_IICtimes);//70
  SCL_L;
  i2c_delayUs(Test_IICtimes);//70
}

void i2c_NoAck(void)
{
  SCL_L;
  SDA_OUT();
  i2c_delayUs(Test_IICtimes);//70
  SDA_H;
  i2c_delayUs(Test_IICtimes);//70
  SCL_H;
  i2c_delayUs(Test_IICtimes);//70
  SCL_L;
  i2c_delayUs(Test_IICtimes);//70
}

uint8_t i2c_WaitAck(void)
{
  volatile uint8_t ucErrTime = 0;
  SDA_IN();
  SDA_H;
  i2c_delayUs(Test_IICtimes);//70
  SCL_H;
  i2c_delayUs(Test_IICtimes);//70
  while(SDA_READ)
  {
    ucErrTime++;
    if(ucErrTime>200)
    {
      i2c_Stop();
      return 0;
    }
  }
  SCL_L;
  i2c_delayUs(Test_IICtimes);//70
  return 1;
}

void i2c_SendByte(uint8_t data)
{
  uint8_t idx_bit=8;
  SDA_OUT();
  SCL_L;
  while(idx_bit--)
  {
    SCL_L;
    //i2c_delayUs(Test_IICtimes);//70
    if(data & 0x80)
      SDA_H;
    else
      SDA_L;
    data<<=1;
    //i2c_delayUs(Test_IICtimes);//70
    SCL_H;
    i2c_delayUs(Test_IICtimes);//70
  }
  SCL_L;
}

uint8_t i2c_ReceiveByte(void)
{
  uint8_t idx_bit=8;
  uint8_t receive_data=0;
  SDA_IN();  //SDA设置为输入
  SDA_H;
  while(idx_bit--)
  {
    receive_data<<=1;
    SCL_L;
    i2c_delayUs(Test_IICtimes);//70
    SCL_H;
    i2c_delayUs(Test_IICtimes);//70
    if(SDA_READ)
    {
      receive_data|=0x01;
    }
  }
  SCL_L;
  return receive_data;
}

uint8_t i2cIoWriteByte(uint16_t write_addr,uint8_t data)
{
  /*  */
  i2c_Start();
  i2c_SendByte(ADDR_24LC02);
  i2c_WaitAck();
  i2c_SendByte((uint8_t)(write_addr & 0x00FF));
  i2c_WaitAck();
  i2c_SendByte(data);
  i2c_WaitAck();
  i2c_Stop();


  //i2c_delayUsms(10);
  // 注意：因为这里要等待EEPROM写完，可以采用查询或延时方式(10ms)
  ///Systick_Delay_1ms(10);
  return (uint8_t)1;
}

void i2cIoWriteBytes(uint16_t write_addr, uint8_t *p_data, uint16_t len)
{
  uint16_t i;
  for(i=write_addr; i<len ; i++)
  {
    i2cIoWriteByte(i,p_data[i]);
  }
}

uint8_t i2cIoReadByte(uint16_t read_addr)
{
  uint8_t temp=0;

  i2c_Start();

  i2c_SendByte(ADDR_24LC02);
  i2c_WaitAck();
  i2c_SendByte((uint8_t)(read_addr & 0x00FF));   //设置低起始地址

  i2c_WaitAck();
  i2c_Start();
  i2c_SendByte((ADDR_24LC02+1)+((read_addr/256)<<1));           //进入接收模式
  i2c_WaitAck();
  temp=i2c_ReceiveByte();
  i2c_Stop();
  return temp;
}

void i2cIoReadBytes(uint16_t read_addr, uint8_t *p_data,   uint16_t cnt)
{
  while(cnt--)
  {
    *p_data ++= i2cIoReadByte(read_addr++);
  }
}


uint8_t i2c_Check(void)
{
  uint8_t temp;
  temp=i2cIoReadByte((uint16_t)Test_IICadd);

  if(temp==0X55)
    return 1;
  else
  {
    i2cIoWriteByte(Test_IICadd, 0X55);
      delay(50);
      temp=i2cIoReadByte(Test_IICadd);
    if(temp==0X55)
      return 1;
  }
  return 0;
}

bool i2c_IsDeviceReady(uint8_t dev_addr)
{
  bool ret = false;


  return ret;
}




#ifdef _USE_HW_CLI
void cliI2CIO(cli_args_t *args)
{
  bool ret = true;
  bool i2c_ret;

  uint16_t reg_addr;
  uint16_t length;

  uint32_t i;

  if (args->argc == 1)
  {
    if(args->isStr(0, "scan") == true)
    {
      for (i=0x00; i<= 0x7F; i++)
      {
        if (i2c_IsDeviceReady(i) == true)
        {
          cliPrintf("I2CIO Addr 0x%X : OK\n", i);
        }
      }
    }
  }
  else if (args->argc == 3)
  {
    reg_addr = (uint16_t) args->getData(1);
    length   = (uint16_t) args->getData(2);

    if(args->isStr(0, "read") == true)
    {
      for (i=0; i<length; i++)
      {
        i2c_ret = i2cIoReadByte(reg_addr);

        if (i2c_ret)
        {
          cliPrintf("I2CIO - 0x%02X : 0x%02X\n", reg_addr+i, i2c_ret);
        }
        else
        {
          cliPrintf("I2CIO - Fail \n");
          break;
        }
      }
    }
    else
    {
      ret = false;
    }
  }
  else
  {
    ret = false;
  }

  if (ret == false)
  {
    cliPrintf( "i2c scan \n");
    cliPrintf( "i2c read reg_addr length\n");

  }
}

#endif





#endif
