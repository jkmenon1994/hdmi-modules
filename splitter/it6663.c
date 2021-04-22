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
#include <unistd.h>

MODULE_DESCRIPTION("i2c device driver for it6663 splitter IC");
MODULE_AUTHOR("jk.menon@ignitarium.com");
MODULE_LICENSE("GPL");

static bool debug;

module_param(debug, bool, 0644);

MODULE_PARM_DESC(debug, "Debugging messages, 0=Off (default), 1=On");

struct clk_tx_linerate {
	struct clk_hw hw;
	struct i2c_client *client;
	struct clk *clk;
	unsigned long rate;
};

static inline int it6663_write(struct i2c_client *client, u8 reg, u8 value)
{
	int rc;
	rc = i2c_smbus_write_byte_data(client, reg, value);
	return rc;
}

static inline int it6663_read(struct i2c_client *client, u8 reg)
{

	return i2c_smbus_read_byte_data(client, reg);
}
#if 0
static int dp159_program(struct i2c_client *client, unsigned long rate)
{
	int r;
	r = dp159_write(client, 0x09, 0x06);

	if ((rate / (1000000)) > 3400) {
//		printk(KERN_INFO "dp159_program(rate = %lu) for HDMI 2.0\n", rate);
		/* SLEW_CTL    = Reg0Bh[7:6] = 10
		 * TX_TERM_CTL = Reg0Bh[4:3] = 11
		*/
		r |= dp159_write(client, 0x0B, 0x9A);

		/* VSWING_DATA & VSWING_CLK to +14%%
		 *            Reg0Ch[7:2] = 100100
		 *  PRE_SEL   Reg0Ch[1:0] = 01 (labeled HDMI_TWPST)
		*/
		r |= dp159_write(client, 0x0C, 0x49);
		r |= dp159_write(client, 0x0D, 0x00);
		r |= dp159_write(client, 0x0A, 0x36); // Automatic retimer for HDMI 2.0
	} else {
//		printk(KERN_INFO "dp159_program(rate = %lu) for HDMI 1.4\n", rate);
		/*datasheet has 0 by default. 0x1 disables DDC training and only
		 * allows HDMI1.4b/DVI, which is OK*/

		r |= dp159_write(client, 0x0B, 0x80); // SLEW_CTL    = Reg0Bh[7:6] = 10
                                              // TX_TERM_CTL = Reg0Bh[4:3] = 00
		// VSWING_DATA & VSWING_CLK to +14%
		//           Reg0Ch[7:2] = 100100
		// PRE_SEL = Reg0Ch[1:0] = 00 (labeled HDMI_TWPST)
		r |= dp159_write(client, 0x0C, 0x48);
		r |= dp159_write(client, 0x0D, 0x00);
        // Automatic redriver to retimer crossover at 1.0 Gbps
		r |= dp159_write(client, 0x0A, 0x35);
	}
	return r;
}
#endif

#if 0

#define to_clk_tx_linerate(_hw) container_of(_hw, struct clk_tx_linerate, hw)

int clk_tx_set_rate(struct clk_hw *hw, unsigned long rate, unsigned long parent_rate)
{
	struct clk_tx_linerate *clk;
	clk = to_clk_tx_linerate(hw);
	//printk(KERN_INFO "dp159: clk_tx_set_rate(): rate = %lu, parent_rate = %lu\n", rate, parent_rate);
	dev_info(&clk->client->dev, "dp159: clk_tx_set_rate(): rate = %lu, parent_rate = %lu\n", rate, parent_rate);
	clk->rate = rate;
	dp159_program(clk->client, rate);
	return 0;
};

unsigned long clk_tx_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	struct clk_tx_linerate *clk;
	clk = to_clk_tx_linerate(hw);
	//printk(KERN_INFO "dp159: clk_tx_recalc_rate(): parent_rate = %lu\n", parent_rate);
	return clk->rate;
};

long clk_tx_round_rate(struct clk_hw *hw,
	unsigned long rate,	unsigned long *parent_rate)
{
	struct clk_tx_linerate *clk;
	clk = to_clk_tx_linerate(hw);
	return rate;
};

#endif

#if 0
struct clk_ops clk_tx_rate_ops = {
	.set_rate 		= &clk_tx_set_rate,
	.recalc_rate	= &clk_tx_recalc_rate,
	.round_rate		= &clk_tx_round_rate,
};
#endif

static int it6663_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int ret;
	unsigned char reg_id, reg_clk, reg_hpd1, reg_hpd2;

	printk(KERN_ERR "%s: probing... \n", __func__);
	/* Check if the adapter supports the needed features */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;

#if 0
	if ((dp159_read(client, 0) != 'D') || (dp159_read(client, 1) != 'P')) {
		dev_err(&client->dev, "Identification registers do not indicate DP159 presence.\n");
		return -ENODEV;
	}
#endif
	reg_id = it6663_read(client,0);
 	printk(KERN_ERR"reg_id :0x%x \n", reg_id); 

	dev_info(&client->dev, "probed\n");

	it6663_write(client,0xf0,0x71);

 	client->addr = 0x38;

	reg_clk = it6663_read(client,0x05);
	printk(KERN_ERR"reg_clk : 0x%x \n", reg_clk);

	client->addr = 0x2c;
	it6663_write(client,0xf1,0x97);

	client->addr = 0x4b;
	it6663_write(client,0x2c,0x69);
	it6663_write(client,0x2d,0x6b); 
	it6663_write(client,0x2e,0x6c);
	it6663_write(client,0x2f,0x6f);


	client->addr = 0x36;
	reg_hpd2 = it6663_read(client,0x03);
	printk(KERN_ERR"reg_hpd2 : 0x%x \n", reg_hpd2);


	client->addr = 0x35;
	reg_hpd1 = it6663_read(client,0x03);
	printk(KERN_ERR"reg_hpd1 : 0x%x \n", reg_hpd1);

	client->addr = 0x36;
	reg_hpd2 = it6663_read(client,0x03);
	printk(KERN_ERR"reg_hpd2 : 0x%x \n", reg_hpd2);



#if 0
	/* initialize to HDMI 1.4 */
	(void)dp159_write(client, 0x0B, 0x80);
	(void)dp159_write(client, 0x0C, 0x48);
	(void)dp159_write(client, 0x0D, 0x00);
	(void)dp159_write(client, 0x0A, 0x35);
#endif

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

module_i2c_driver(it6663_driver);
