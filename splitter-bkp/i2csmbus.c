#include <linux/module.h>
#include <linux/types.h>
#include "i2c_smbus.h"


struct i2c_smbus {
	struct i2c_client *client;
};
struct i2c_smbus *dev; 


void i2c_client_init(struct i2c_client *client)
{
 
  dev  = kzalloc(sizeof(*dev), GFP_KERNEL);
  if (dev == NULL) {
	printk(KERN_ERR "%s:allocation failed \n",__func__);
  } 

  dev->client = client; 

}


int i2c_read_byte(u8 slave_addr, u8 reg, u8 byteno, u8 *data, u8 i2c_bus)
{
  int ret;
  
  dev->client->addr = slave_addr >> 1;
 
  if(byteno < 2) {
        *data = i2c_smbus_read_byte_data(dev->client, reg);
        if (*data < 0)
            printk(KERN_ERR "%s: i2c transaction failed \n", __func__);
  }
  else {
        ret = i2c_smbus_read_block_data(dev->client, reg, data);
        if(ret < 0)
           printk(KERN_ERR "%s: i2c read block transaction failed \n", __func__);
  }

  return 0;
}


int i2c_write_byte(u8 slave_addr, u8 reg, u8 byteno, const u8 *val, u8 i2c_bus)
{
  int ret;

  dev->client->addr = slave_addr >> 1;

  if (byteno < 2) {
        ret = i2c_smbus_write_byte_data(dev->client, reg, *val);
        if (ret < 0)
           printk( KERN_ERR "%s: i2c_smbus_write_byte transaction failed \n", __func__);
  }
  else {
       ret = i2c_smbus_write_block_data(dev->client, reg, byteno, val);
        if (ret < 0)
          printk( KERN_ERR "%s:  i2c_smbus_write_burst transaction failed \n", __func__);
 }

 return 0;

}



