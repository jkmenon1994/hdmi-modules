#ifndef _I2C_SMBUS_H_
#define _I2C_SMBUS_H_

#include <linux/types.h>
#include <linux/i2c.h>


typedef unsigned char u8;


void i2c_client_init(struct i2c_client *client);
int i2c_read_byte(u8 slave_addr, u8 reg, u8 byteno, u8 *data, u8 i2c_bus);
int i2c_write_byte(u8 slave_addr, u8 reg, u8 byteno, const u8 *val, u8 i2c_bus);


#endif
