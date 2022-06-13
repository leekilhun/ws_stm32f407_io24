/*
 * i2c_io.h
 *
 *  Created on: 2022. 6. 5.
 *      Author: gns2l
 */

#ifndef SRC_COMMON_INC_HW_I2C_IO_H_
#define SRC_COMMON_INC_HW_I2C_IO_H_


#ifdef _USE_HW_I2C_IO


#ifdef __cplusplus
extern "C" {
#endif


bool i2cIoInit(void);

uint8_t i2cIoWriteByte(uint16_t write_addr,uint8_t data);
uint8_t i2cIoReadByte(uint16_t read_addr);
void i2cIoWriteBytes(uint16_t write_addr, uint8_t *p_data, uint16_t len);
void i2cIoReadBytes(uint16_t read_addr, uint8_t *p_data,   uint16_t cnt);


#ifdef __cplusplus
}
#endif

#endif

#endif /* SRC_COMMON_INC_HW_I2C_IO_H_ */
