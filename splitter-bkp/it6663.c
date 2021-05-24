/*
 * dp159 redriver and retimer
 * Copyright (C) 2016, 2017 Leon Woestenberg <leon@sidebranch.com>
 *
 * based on code
 * Copyright (C) 2007 Hans Verkuil
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of.h>
#include <linux/clk-provider.h>
#include "i2c_smbus.h"
#include "IT6664.h"

MODULE_DESCRIPTION("i2c device driver for it6663 splitter IC");
MODULE_AUTHOR("jk.menon@ignitarium.com");
MODULE_LICENSE("GPL");

static bool debug;

module_param(debug, bool, 0644);

MODULE_PARM_DESC(debug, "Debugging messages, 0=Off (default), 1=On");


struct class *dlnx_it6663_class;

struct dlnx_it6663 {
        int irq;
        struct device *dev;
        unsigned baseminor;
        unsigned count;
        dev_t devno;
        struct cdev cdev;
        unsigned int flags;
};


static irqreturn_t irq_handler(int irq, void *user_data)
{

  printk(KERN_ERR "irq triggered \n");


  return IRQ_HANDLED;
}
   

static int it6663_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int ret;

	printk(KERN_ERR "%s: probing... \n", __func__);
	/* Check if the adapter supports the needed features */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;
	
 	i2c_client_init(client);

	IT6664_DeviceSelect(0x01);
        it6664_hdmi2sp_initial();
	

	irq = 108;
	ret = request_irq(irq, irq_handler, 0, "dlnx_it6663",NULL) ;
	if ( ret < 0)
		printk(KERN_ERR " IRQ registration failed \n");


	dev_info(&client->dev, "probe successful\n");
	return 0;
}

static int it6663_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id it6663_id[] = {
	{ "it6663", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, it6663_id);

static const struct of_device_id it6663_of_match[] = {
        { .compatible = "ite,it6663", },
        { /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, it6663_of_match);


static struct i2c_driver it6663_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name	= "it6663",
		.of_match_table = of_match_ptr(it6663_of_match),
	},
	.probe		= it6663_probe,
	.remove		= it6663_remove,
	.id_table	= it6663_id,
};

#if 0
static int __init dlnx_it6663_init(void)
{
        dlnx_it6663_class = class_create(THIS_MODULE, "dlnx_it6663_class");
        if (IS_ERR(dlnx_it6663_class))
                return PTR_ERR(dlnx_it6663_class);
        return platform_driver_register(&it6663_driver);
}


static void __exit dlnx_it6663_exit(void)
{
        platform_driver_unregister(&it6663_driver);
        class_destroy(dlnx_it6663_class);

        printk(KERN_ALERT "dlnx-it6663 driver removed.\n");
}

module_init(dlnx_it6663_init);
module_exit(dlnx_it6663_exit);
#endif

module_i2c_driver(it6663_driver);
