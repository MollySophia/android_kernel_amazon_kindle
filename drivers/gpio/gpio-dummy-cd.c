/*
 * Copyright (c) 2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/gpio.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/module.h>

#define AUTHOR 	    "Amazon Inc"
#define DESCRIPTION "Driver to emulate card detect gpio for bcm sdio wifi card"

/* Private driver data */
struct dummy_cd_driver_data {
	struct irq_domain *domain;
	struct gpio_chip *gchip;
	struct irq_chip_generic *gc;
	/* This is the wifi power gpio for bcm chip */
	int gpio_wl_reg_on;
	int data;
	/* Starting irq number allocated for this controller */
	int irq_base;
};

/* Struct to represent device node compatible string in the dt */
static struct of_device_id dummy_cd_gpio_of_match[] = {
	{ .compatible = "amazon,dummy_cd" },
	{ },
};

/* dummy gpio Bank */
static int dummy_cd_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct dummy_cd_driver_data *data = dev_get_drvdata(chip->dev);
	/*
	 * Since mmc sdio subsystem by default expects an active low gpio
	 * we return inverse of current wl_reg_on gpio value.
	 */
	return !gpio_get_value(data->gpio_wl_reg_on);
}

/* Map gpio to irq */
static int dummy_cd_gpio_to_irq(struct gpio_chip *gc, unsigned offset)
{
	struct dummy_cd_driver_data *data = dev_get_drvdata(gc->dev);
	return irq_find_mapping(data->domain, offset);
}

int dummy_cd_gpio_direction_input(struct gpio_chip *chip,
				unsigned offset)
{
	/* make gpio sub system happy */
	return 0;
}

static int dummy_cd_of_gpio_simple_xlate(struct gpio_chip *gc,
			 const struct of_phandle_args *gpiospec, u32 *flags)
{
	if (WARN_ON(gpiospec->args_count < gc->of_gpio_n_cells)) {
		return -EINVAL;
	}
	if (gpiospec->args[0] >= gc->ngpio) {
		return -EINVAL;
	}
	if (flags)
		*flags = gpiospec->args[1];

	return gpiospec->args[0];
}

/* dummy IRQ controller */
static void __init dummy_cd_gpio_init_gc(struct dummy_cd_driver_data *data, int irq_base)
{
	struct irq_chip_generic *gc;
	struct irq_chip_type *ct;

	gc = irq_alloc_generic_chip("dummy-gpio-cd", 1, irq_base,
				    &data->data, handle_level_irq);
	gc->private = data;

	ct = gc->chip_types;
	ct->chip.irq_ack = irq_gc_ack_set_bit;
	ct->chip.irq_mask = irq_gc_mask_clr_bit;
	ct->chip.irq_unmask = irq_gc_mask_set_bit;
	ct->chip.flags = IRQCHIP_MASK_ON_SUSPEND;

	irq_setup_generic_chip(gc, IRQ_MSK(1), IRQ_GC_INIT_NESTED_LOCK,
			       IRQ_NOREQUEST, 0);

	data->gc = gc;
}

static int dummy_cd_gpio_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;
	struct gpio_chip *gchip;
	struct dummy_cd_driver_data *data;
	int irq_base;
	int err;

	match = of_match_device(dummy_cd_gpio_of_match, &pdev->dev);
	if (!match) {
		dev_err(&pdev->dev, "Error: No device match found\n");
		return -ENODEV;
	}

	/* Allocate the gpio chip */
	gchip = devm_kzalloc(&pdev->dev, sizeof(*gchip), GFP_KERNEL);
	if (!gchip)
		return -ENOMEM;

	/* Allocate the per instance driver data */
	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	/* Populate callback functions */
	gchip->dev = &pdev->dev;
	gchip->get = dummy_cd_gpio_get;
	gchip->to_irq = dummy_cd_gpio_to_irq;
	gchip->direction_input = dummy_cd_gpio_direction_input;
	gchip->of_xlate = dummy_cd_of_gpio_simple_xlate;
	/* Set base to negative number to request dynamic allocation of gpio */
	gchip->base = -1;
	/* We manage just 1 dummy gpio */
	gchip->ngpio = 1;
	gchip->label = "dummy-cd-gpio";

	data->gchip = gchip;
	data->gpio_wl_reg_on = of_get_named_gpio(pdev->dev.of_node,
                                                "emul-cd-gpio", 0);
	if (data->gpio_wl_reg_on < 0) {
		dev_err(&pdev->dev, "%s: get emul-cd-gpio failed err=%d\n", __func__,
                data->gpio_wl_reg_on);
		return data->gpio_wl_reg_on;
	}

	/* Register gpio chip with the kernel */
	err = gpiochip_add(gchip);
	if (err)
		return err;

	/* Allocate irq */
	irq_base = irq_alloc_descs(-1, 0, 1, numa_node_id());
	if (irq_base < 0) {
		dev_err(&pdev->dev, "%s: irq_alloc_failed err=%d\n", __func__,
                irq_base);
		WARN_ON(gpiochip_remove(data->gchip) < 0);
		return irq_base;
	}
	data->irq_base = irq_base;

	data->domain = irq_domain_add_legacy(pdev->dev.of_node, 1, irq_base, 0,
					     &irq_domain_simple_ops, NULL);
	if (!data->domain) {
		dev_err(&pdev->dev, "%s: irq_domain_add_legacy failed err=%d\n",
		__func__, -ENODEV);
		irq_free_desc(data->irq_base);
		WARN_ON(gpiochip_remove(data->gchip) < 0);
		return -ENODEV;
	}

	/* dummy-cd-gpio can be a generic irq chip */
	dummy_cd_gpio_init_gc(data, irq_base);

	dev_set_drvdata(&pdev->dev, data);

	return 0;
}

static int dummy_cd_gpio_remove(struct platform_device *pdev)
{
	struct dummy_cd_driver_data *data = dev_get_drvdata(&pdev->dev);
	/* clean up */
	irq_remove_generic_chip(data->gc, IRQ_MSK(1), IRQ_NOREQUEST, 0);
	irq_domain_remove(data->domain);
	irq_free_desc(data->irq_base);
	WARN_ON(gpiochip_remove(data->gchip) < 0);
	return 0;
}

static struct platform_driver dummy_cd_gpio_driver = {
	.driver		= {
		.name	= "dummy-cd-gpio",
		.owner	= THIS_MODULE,
		.of_match_table = dummy_cd_gpio_of_match,
	},
	.probe		= dummy_cd_gpio_probe,
	.remove     	= dummy_cd_gpio_remove,
};

module_platform_driver(dummy_cd_gpio_driver);

MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESCRIPTION);
MODULE_LICENSE("GPL v2");
