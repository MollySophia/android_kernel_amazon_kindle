/*
 * @file bd7181x.c  --  RoHM BD7181X/BD71817 mfd driver
 * 
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 * @author: Tony Luo <luofc@embedinfo.com>
 * Copyright 2014 Embest Technology Co. Ltd. Inc.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/gpio.h>
#include <linux/regmap.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/mfd/core.h>
#include <linux/mfd/bd7181x.h>
#include <linux/delay.h>

/** @brief bd7181x irq resource */
static struct resource rtc_resources[] = {
	{
		.start  = BD7181X_IRQ_ALARM_12,
		.end    = BD7181X_IRQ_ALARM_12,
		.flags  = IORESOURCE_IRQ,
	}
};

static struct resource power_resources[] = {
	// irq# 1 power
	{
		.start	= BD7181X_IRQ_DCIN_03,
		.end	= BD7181X_IRQ_DCIN_03,
		.flags	= IORESOURCE_IRQ,
	},
	// irq# 2 Battery
	{
		.start	= BD7181X_IRQ_BAT_MON_08,
		.end	= BD7181X_IRQ_BAT_MON_08,
		.flags	= IORESOURCE_IRQ,
	},
	// irq# 3 temperature
	{
		.start	= BD7181X_IRQ_TEMPERATURE_11,
		.end	= BD7181X_IRQ_TEMPERATURE_11,
		.flags	= IORESOURCE_IRQ,

	}
	/* JIRA JEIGHT-1522/JEIGHT-2408
	 * remove the charger interrupt for the time being
	 * and will reuse it if Rohm provide us better solution */
#if 0
	// irq# 4 charger
	{
		.start	= BD7181X_IRQ_CHARGE_05,
		.end	= BD7181X_IRQ_CHARGE_05,
		.flags	= IORESOURCE_IRQ,
	}
#endif
};

/** @brief bd7181x multi function cells */
static struct mfd_cell bd7181x_mfd_cells[] = {
	{
		.name = "bd7181x-pmic",
	},
	{
		.name = "bd7181x-power",
		.num_resources = ARRAY_SIZE(power_resources),
		.resources = &power_resources[0],
	},
	{
		.name = "bd7181x-gpo",
	},
	{
		.name = "bd7181x-rtc",
		.num_resources = ARRAY_SIZE(rtc_resources),
		.resources = &rtc_resources[0],
	},
};

/** @brief bd7181x irqs */
static const struct regmap_irq bd7181x_irqs[] = {
	[BD7181X_IRQ_BUCK_01] = {
		.mask = BD7181X_INT_EN_01_BUCKAST_MASK,
		.reg_offset = 1,
	},
	[BD7181X_IRQ_DCIN_02] = {
		.mask = BD7181X_INT_EN_02_DCINAST_MASK,
		.reg_offset = 2,
	},
	[BD7181X_IRQ_DCIN_03] = {
		.mask = BD7181X_INT_EN_03_DCINAST_MASK,
		.reg_offset = 3,
	},
	[BD7181X_IRQ_VSYS_04] = {
		.mask = BD7181X_INT_EN_04_VSYSAST_MASK,
		.reg_offset = 4,
	},
	[BD7181X_IRQ_CHARGE_05] = {
		.mask = BD7181X_INT_EN_05_CHGAST_MASK,
		.reg_offset = 5,
	},
	[BD7181X_IRQ_BAT_06] = {
		.mask = BD7181X_INT_EN_06_BATAST_MASK,
		.reg_offset = 6,
	},
	[BD7181X_IRQ_BAT_MON_07] = {
		.mask = BD7181X_INT_EN_07_BMONAST_MASK,
		.reg_offset = 7,
	},
	[BD7181X_IRQ_BAT_MON_08] = {
		.mask = BD7181X_INT_EN_08_BMONAST_MASK,
		.reg_offset = 8,
	},
	[BD7181X_IRQ_BAT_MON_09] = {
		.mask = BD7181X_INT_EN_09_BMONAST_MASK,
		.reg_offset = 9,
	},
	[BD7181X_IRQ_BAT_MON_10] = {
		.mask = BD7181X_INT_EN_10_BMONAST_MASK,
		.reg_offset = 10,
	},
	[BD7181X_IRQ_TEMPERATURE_11] = {
		.mask = BD7181X_INT_EN_11_TMPAST_MASK,
		.reg_offset = 11,
	},
	[BD7181X_IRQ_ALARM_12] = {
		.mask = BD7181X_INT_EN_12_ALMAST_MASK,
		.reg_offset = 12,
	},
};

/** @brief bd7181x irq chip definition */
static struct regmap_irq_chip bd7181x_irq_chip = {
	.name = "bd7181x",
	.irqs = bd7181x_irqs,
	.num_irqs = ARRAY_SIZE(bd7181x_irqs),
	.num_regs = 13,
	.irq_reg_stride = 1,
	.status_base = BD7181X_REG_INT_STAT,
	.mask_base = BD7181X_REG_INT_EN_01 - 1,
	.mask_invert = true,
	// .ack_base = BD7181X_REG_INT_STAT_00,
};
static int pmic_irq = -1;

/** @brief bd7181x irq initialize 
 *  @param bd7181x bd7181x device to init
 *  @param bdinfo platform init data
 *  @retval 0 probe success
 *  @retval negative error number
 */
static int bd7181x_irq_init(struct bd7181x *bd7181x, struct bd7181x_board* bdinfo) {
	int irq;
	int ret = 0;

	if (!bdinfo) {
		dev_warn(bd7181x->dev, "No interrupt support, no pdata\n");
		return -EINVAL;
	}

    /* Request INTB gpio */
	ret = devm_gpio_request_one(bd7181x->dev, (unsigned)bdinfo->gpio_intr,
	    GPIOF_IN, "bd7181x-intb");

	if (unlikely(ret)) {
        dev_err(bd7181x->dev, "failed to request INTB gpio [%d]\n", ret);
       return ret;
    }
	
    bd7181x->chip_irq = gpio_to_irq(bdinfo->gpio_intr);

    dev_info(bd7181x->dev, "IRQ %d GPIO %d\n", bd7181x->chip_irq,
        bdinfo->gpio_intr);

	ret = regmap_add_irq_chip(bd7181x->regmap, bd7181x->chip_irq,
		IRQF_ONESHOT | IRQF_TRIGGER_FALLING, bdinfo->irq_base,
		&bd7181x_irq_chip, &bd7181x->irq_data);
	if (ret < 0)
		dev_warn(bd7181x->dev, "Failed to add irq_chip %d\n", ret);

	/* Configure wakeup capable */
	device_set_wakeup_capable(bd7181x->dev, 1);
	device_set_wakeup_enable(bd7181x->dev , 1);

	return ret;
}

/** @brief bd7181x irq initialize
 *  @param bd7181x bd7181x device to init
 *  @retval 0 probe success
 *  @retval negative error number
 */
static int bd7181x_irq_exit(struct bd7181x *bd7181x)
{
	if (bd7181x->chip_irq > 0)
		regmap_del_irq_chip(bd7181x->chip_irq, bd7181x->irq_data);
	return 0;
}

/** @brief check whether volatile register 
 *  @param dev kernel device pointer
 *  @param reg register index
 */
static bool is_volatile_reg(struct device *dev, unsigned int reg)
{
	// struct bd7181x *bd7181x = dev_get_drvdata(dev);

	/*
	 * Caching all regulator registers.
	 */
	return true;
}

/** @brief regmap configures */
static const struct regmap_config bd7181x_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.volatile_reg = is_volatile_reg,
	.max_register = BD7181X_MAX_REGISTER - 1,
	.cache_type = REGCACHE_RBTREE,
};

#ifdef CONFIG_OF
static struct of_device_id bd7181x_of_match[] = {
	{ .compatible = "rohm,bd71815", .data = (void *)0},
	{ .compatible = "rohm,bd71817", .data = (void *)1},
	{ },
};
MODULE_DEVICE_TABLE(of, bd7181x_of_match);


/** @brief parse device tree data of bd7181x
 *  @param client client object provided by system
 *  @param chip_id return chip id back to caller
 *  @return board initialize data
 */
static struct bd7181x_board *bd7181x_parse_dt(struct i2c_client *client,
						int *chip_id)
{
	struct device_node *np = client->dev.of_node;
	struct bd7181x_board *board_info;
	unsigned int prop;
	const struct of_device_id *match;
	int r = 0;

	match = of_match_device(bd7181x_of_match, &client->dev);
	if (!match) {
		dev_err(&client->dev, "Failed to find matching dt id\n");
		return NULL;
	}

	*chip_id  = (int)match->data;

	board_info = devm_kzalloc(&client->dev, sizeof(*board_info),
			GFP_KERNEL);
	if (!board_info) {
		dev_err(&client->dev, "Failed to allocate pdata\n");
		return NULL;
	}

	board_info->gpio_intr = of_get_named_gpio(np, "gpio_intr", 0);
        if (!gpio_is_valid(board_info->gpio_intr)) {
		dev_err(&client->dev, "no pmic intr pin available\n");
		goto err_intr;
        }

        r = of_property_read_u32(np, "irq_base", &prop);
        if (!r) {
		board_info->irq_base = prop;
        } else {
		board_info->irq_base = -1;
        }

	return board_info;

err_intr:
	devm_kfree(&client->dev, board_info);
	return NULL;
}
#else
static inline
struct bd7181x_board *bd7181x_parse_dt(struct i2c_client *client,
					 int *chip_id)
{
	return NULL;
}
#endif

/** @brief probe bd7181x device
 *  @param i2c client object provided by system
 *  @param id chip id
 *  @retval 0 probe success
 *  @retval negative error number
 */
static int bd7181x_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct bd7181x *bd7181x;
	struct bd7181x_board *pmic_plat_data;
	struct bd7181x_board *of_pmic_plat_data = NULL;
	int chip_id = id->driver_data;
	int ret = 0;

	pmic_plat_data = dev_get_platdata(&i2c->dev);

	if (!pmic_plat_data && i2c->dev.of_node) {
		pmic_plat_data = bd7181x_parse_dt(i2c, &chip_id);
		of_pmic_plat_data = pmic_plat_data;
	}

	if (!pmic_plat_data)
		return -EINVAL;

	bd7181x = kzalloc(sizeof(struct bd7181x), GFP_KERNEL);
	if (bd7181x == NULL)
		return -ENOMEM;

	bd7181x->of_plat_data = of_pmic_plat_data;
	i2c_set_clientdata(i2c, bd7181x);
	bd7181x->dev = &i2c->dev;
	bd7181x->i2c_client = i2c;
	bd7181x->id = chip_id;
	mutex_init(&bd7181x->io_mutex);

	bd7181x->regmap = devm_regmap_init_i2c(i2c, &bd7181x_regmap_config);
	if (IS_ERR(bd7181x->regmap)) {
		ret = PTR_ERR(bd7181x->regmap);
		dev_err(&i2c->dev, "regmap initialization failed: %d\n", ret);
		return ret;
	}

	bd7181x_irq_init(bd7181x, of_pmic_plat_data);

	ret = mfd_add_devices(bd7181x->dev, -1,
			      bd7181x_mfd_cells, ARRAY_SIZE(bd7181x_mfd_cells),
			      NULL, 0,
			      regmap_irq_get_domain(bd7181x->irq_data));
	if (ret < 0)
		goto err;

	return ret;

err:
	mfd_remove_devices(bd7181x->dev);
	kfree(bd7181x);
	return ret;
}

/** @brief remove bd7181x device
 *  @param i2c client object provided by system
 *  @return 0
 */
static int bd7181x_i2c_remove(struct i2c_client *i2c)
{
	struct bd7181x *bd7181x = i2c_get_clientdata(i2c);

	bd7181x_irq_exit(bd7181x);
	mfd_remove_devices(bd7181x->dev);
	kfree(bd7181x);

	return 0;
}
#ifdef CONFIG_PM_SLEEP

/*
 * helper i2c write register routine to set up an i2c msg and send it out
 * without locking intentionally.
 */
static inline int bd7181x_write_i2c_reg(struct i2c_client *client, int reg, int val)
{
	u8 temp_buf[2] = { reg, val };
	int ret;
	struct i2c_msg msg = {
		.addr = client->addr,
		.flags = 0,
		.len = 2,
		.buf = temp_buf,
	};

	ret = __i2c_transfer(client->adapter, &msg, 1);
	return (ret == 1)? 0 : -EBUSY;
}

/*
 * helper i2c read register routine. Similar to the write reg. routine.
 * This is done without locking intentionally.
 */
static inline int bd7181x_read_i2c_reg(struct i2c_client *client, u8 reg, u8 *val)
{
	int ret;
	struct i2c_msg msg1 = {
		.addr = client->addr,
		.flags = 0,
		.len = 1,
		.buf = &reg,
	};
	struct i2c_msg msg2 = {
		.addr = client->addr,
		.flags = I2C_M_RD,
		.len = 1,
		.buf = val,
	};
	struct i2c_msg msgs[] = { msg1, msg2 };

	ret = __i2c_transfer(client->adapter, msgs, 2);
	return (ret == 2)? 0 : -EBUSY;
}

/*
 * Function to enter PMIC test mode.
 * This requires writing 3 separate values to the
 * BD7181X_REG_TEST_MODE register. If any of these failed,
 * we will not be in test mode. No clean up is required.
 */
static int bd7181x_enter_test_mode(struct i2c_client *client)
{
	int ret;

	ret = bd7181x_write_i2c_reg(client, BD7181X_REG_TEST_MODE,
				    BD7181X_TEST_REG_ACCESS_AREA_1);
	if (ret)
		goto out;
	ret = bd7181x_write_i2c_reg(client, BD7181X_REG_TEST_MODE,
				    BD7181X_TEST_REG_ACCESS_AREA_2);
	if (ret)
		goto out;
	ret = bd7181x_write_i2c_reg(client, BD7181X_REG_TEST_MODE,
				    BD7181X_TEST_REG_ACCESS_AREA_3);
out:
	return ret;
}

/*
 * This function will be used to perform the workarounds suggested by ROHM
 * to be able to suspend and resume the SOC. The suggested sequence
 * requires 5+ I2C writes and must be conducted under PMIC test mode.
 * The sequence goes like this:
 * - Enter the PMIC test mode
 * - workaround 1: adjust the reference voltage off control
 * - workaround 2: adjust the power sequence register
 * - workaround X...: if more
 * - (IMPORTANT) Exit test mode and back to PMIC normal mode.
 * During the duration of this process, all other I2C registers access
 * are invalid hence we need to do this "atomically".
 */
static int bd7181x_workaround_set(struct bd7181x *bd7181x, int suspend)
{
	struct i2c_client *client = bd7181x->i2c_client;
	int ret;
	u8 reg = 0;

	if (unlikely(!client))
		return -EINVAL;

	/*
	 * lock at the adapter level explicitly. If there are any other processes
	 * (including threaded IRQ handlers) who wish to access the i2c, it will
	 * need to wait.
	 * This is a poor man's way to ensure atomicity in executing a
	 * a sequence of i2c operations back to back.
	 */
	i2c_lock_adapter(client->adapter);

	/* Enter PMIC test mode */
	ret = bd7181x_enter_test_mode(client);
	if (ret)
		goto out;

	/* workaround 1. voltage off sequence toggling.
	 * This will address the problem of VDD_ARM voltage drops to 0 too slow (~116ms)
	 * during suspend and if a wakeup interrupt comes in within the ~116ms interval.
	 * VDD_ARM voltage will ramp up too slow and result in a system watchdog.
	 */
	ret = bd7181x_read_i2c_reg(client, BD7181X_TEST_REG_VOLT_OFFSEQ_CTL, &reg);
	if (likely(!ret)) {
		/* Test register 0x02 description
		 * [bit 7]:    Voltage Off sequence control
		 *             0=Enable off sequence  (default, when resume)
		 *               power-off-sequence get disabled sequentially hence slow
		 *             1=Disable off sequence (before suspend)
		 *               power-off-sequence get disabled in parallel
		 * [bit 6:0] : Trimming bit for reference voltage
		 */
		reg = (suspend)? (reg | 0x80) : (reg & 0x7f);
		ret = bd7181x_write_i2c_reg(client, BD7181X_TEST_REG_VOLT_OFFSEQ_CTL, reg);
		/* This is NOT fatal, warm it if failed */
		WARN_ON(ret);
	}

	/* workaround 2. Change power sequence interval time before going into suspend,
	 * set the interval time to minimum so when resume, the VDD_ARM
	 * will be able to power up within 4ms.
	 * After resume, set the the interval time back to normal.
	 */
	if (suspend)
		ret = bd7181x_write_i2c_reg(client, BD7181X_TEST_REG_POWER_SEQ_REG,
					    BD7181X_TEST_REG_SEQ_INTV_TIME_MIN);
	else
		ret = bd7181x_write_i2c_reg(client, BD7181X_TEST_REG_POWER_SEQ_REG,
					    BD7181X_TEST_REG_SEQ_INTV_TIME_INIT);

	/* Exit PMIC test mode, back to normal */
	if (bd7181x_write_i2c_reg(client, BD7181X_REG_TEST_MODE,
				  BD7181X_TEST_REG_ACCESS_USER_AREA)) {
		/* Fail to exit test mode, this should not happen.
		 * Not much we can do, warn it and let's try again.
		 */
		WARN_ON(1);
		bd7181x_write_i2c_reg(client, BD7181X_REG_TEST_MODE,
				      BD7181X_TEST_REG_ACCESS_USER_AREA);
	}

out:
	i2c_unlock_adapter(client->adapter);

	/* everything went ok, delay a bit for test setting to take effect */
	if (!ret)
		udelay(2000);

	return ret;
}

/**@brief Suspend the bd7181x PMIC
 * @param dev bd7181x mfd device
 * @retval 0
 */
static int bd7181x_mfd_suspend_late(struct device *dev)
{
	struct bd7181x *bd7181x = dev_get_drvdata(dev);
	int ret;

	if (device_may_wakeup(dev)) {
		/* decrease power sequence interval. If failed, do not suspend */
	        ret = bd7181x_workaround_set(bd7181x, 1);
		if (ret) {
			dev_err(dev, "suspend failed\n");
			return ret;
		}
		enable_irq_wake(bd7181x->chip_irq);
	}
	return 0;
}

/**@brief Resume the bd7181x PMIC
 * @param dev bd7181x mfd device
 * @retval 0
 */
static int bd7181x_mfd_resume_early(struct device *dev)
{
	struct bd7181x *bd7181x = dev_get_drvdata(dev);

	if (device_may_wakeup(dev)) {
	        if (bd7181x_workaround_set(bd7181x, 0))
			dev_err(dev, "resume failed\n");
		disable_irq_wake(bd7181x->chip_irq);
	}
	return 0;
}
#endif

static struct dev_pm_ops bd7181x_i2c_pm_ops = {
	.suspend_late = bd7181x_mfd_suspend_late,
	.resume_early = bd7181x_mfd_resume_early,
};

static const struct i2c_device_id bd7181x_i2c_id[] = {
	{ "bd71815", 0 },
	{ "bd71817", 1 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, bd7181x_i2c_id);


static struct i2c_driver bd7181x_i2c_driver = {
	.driver = {
		.name = "bd7181x",
		.owner = THIS_MODULE,
		.pm = &bd7181x_i2c_pm_ops,
		.of_match_table = of_match_ptr(bd7181x_of_match),
	},
	.probe = bd7181x_i2c_probe,
	.remove = bd7181x_i2c_remove,
	.id_table = bd7181x_i2c_id,
};

static int __init bd7181x_i2c_init(void)
{
	return i2c_add_driver(&bd7181x_i2c_driver);
}
/* init early so consumer devices can complete system boot */
subsys_initcall(bd7181x_i2c_init);

static void __exit bd7181x_i2c_exit(void)
{
	i2c_del_driver(&bd7181x_i2c_driver);
}
module_exit(bd7181x_i2c_exit);

MODULE_AUTHOR("Tony Luo <luofc@embest-tech.com>");
MODULE_AUTHOR("Peter Yang <yanglsh@embest-tech.com>");
MODULE_DESCRIPTION("BD71815/BD71817 chip multi-function driver");
MODULE_LICENSE("GPL");
