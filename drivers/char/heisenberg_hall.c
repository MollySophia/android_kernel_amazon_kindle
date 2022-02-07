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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/skbuff.h>
#include <linux/mmc/host.h>
#include <linux/device.h>
#include <linux/if.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/random.h>
#include <linux/lab126_hall.h>
#include <linux/sysctl.h>
#include <linux/sched.h>
#include <linux/miscdevice.h>
#include <linux/pm.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/sysfs.h>
#include <linux/workqueue.h>
#include <asm/uaccess.h>
#include <asm/mach-types.h>



#define HALL_DEV_MINOR      163
#define HALL_DRIVER_NAME    "heisenberg_hall"

#define HALL_INIT_DELAY	    1000
#define HALL_EVENT_DELAY    3000
#define HALL_EVENT_DEB      250
/* Hall Sensor States */
#define STATE_OPENED        0
#define STATE_CLOSED        1

#define HALL_CTRL_DISABLE   0
#define HALL_CTRL_ENABLE    1

struct hall_drvdata {
	struct hall_platform_data* pdata;
	int (*enable)(struct device *dev);
	void (*disable)(struct device *dev);
};


extern int heisenberg_pwrkey_ctrl(int); 

static int hall_dbg = 0;
static int is_hall_wkup = 0;		/* set when last wakeup due to hall event (cover open) */
static int hall_event = 0;			/* track hall events (open / close) */
static int hall_pullup_enabled = 0;	/* default=0 to be enabled only for VNI tests */
static int hall_close_delay = 0;	/* delay to avoid multiple transitions within short duration */
static int current_state = 0;		/* OPEN=0, CLOSE=1 */
static int hall_enabled = HALL_CTRL_ENABLE;	/* Enable Hall control by default */
static int hall_irq = -1;
static int g_gpio_hall = -1;

static void hall_close_event_work_handler(struct work_struct *);
static DECLARE_DELAYED_WORK(hall_close_event_work, hall_close_event_work_handler);

static void hall_open_event_work_handler(struct work_struct *);
static DECLARE_DELAYED_WORK(hall_open_event_work, hall_open_event_work_handler);

static void hall_init_work_handler(struct work_struct *);
static DECLARE_DELAYED_WORK(hall_init_work, hall_init_work_handler);

extern int pb_oneshot;
extern void pb_oneshot_unblock_button_events (void);
/*
 * hall sensor gpio value hi is not detected, lo is detected
 */
int gpio_hallsensor_detect(void)
{	
	return !gpio_get_value(g_gpio_hall);
}
static void hall_init_state(void)
{
	if(hall_enabled) {						/* HALL CTRL ENABLED */
		if(gpio_hallsensor_detect()) {
			heisenberg_pwrkey_ctrl(0);	
			current_state = STATE_CLOSED;	/* COVER CLOSE */
		} else {
			heisenberg_pwrkey_ctrl(1);	
			current_state = STATE_OPENED;	/* COVER OPEN */
		}
	} else {								/* HALL CTRL DISABLED */
		heisenberg_pwrkey_ctrl(1);
	}
	return;
}


int gpio_hallsensor_irq(void)
{
	return hall_irq;
}
void gpio_hallsensor_pullup(int enable)
{
#if 0
	if (enable > 0) {
		mxc_iomux_v3_setup_pad(MX6SL_HALL_SNS(PU));
	} else {
		mxc_iomux_v3_setup_pad(MX6SL_HALL_SNS(PD));
	}
#endif
}


static long hall_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int __user *argp = (int __user *)arg;
	int ret = -EINVAL;
	int state = 0;

	switch (cmd) {
		case HALL_IOCTL_GET_STATE:
			state = gpio_hallsensor_detect();
			if (put_user(state, argp))
				return -EFAULT;
			else
				ret = 0;
			break;
		default:
			break;
	}
	return ret;
}

static ssize_t hall_misc_write(struct file *file, const char __user *buf,
								size_t count, loff_t *pos)
{
	return 0;
}

static ssize_t hall_misc_read(struct file *file, char __user *buf,
								size_t count, loff_t *pos)
{
	return 0;
}

static const struct file_operations hall_misc_fops =
{
	.owner = THIS_MODULE,
	.read  = hall_misc_read,
	.write = hall_misc_write,
	.unlocked_ioctl = hall_ioctl,
};

static struct miscdevice hall_misc_device =
{
	.minor = HALL_DEV_MINOR,
	.name  = HALL_MISC_DEV_NAME,
	.fops  = &hall_misc_fops,
};

static ssize_t hall_gpio_pullup_store(struct sys_device *dev, struct sysdev_attribute *attr, const char *buf, size_t size)
{
	int value = 0;
	if (sscanf(buf, "%d", &value) <= 0) {
		printk(KERN_ERR "Could not update hall debug ctrl \n");
		return -EINVAL;
	}
	hall_pullup_enabled = (value > 0) ? 1 : 0;
	gpio_hallsensor_pullup(hall_pullup_enabled);
	return size;
}

static ssize_t hall_gpio_pullup_show(struct sys_device *dev, struct sysdev_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", hall_pullup_enabled);
}

static DEVICE_ATTR(hall_gpio_pullup, 0644, hall_gpio_pullup_show, hall_gpio_pullup_store);

static ssize_t hall_trig_wkup_show(struct sys_device *dev, struct sysdev_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", is_hall_wkup);
}
static DEVICE_ATTR(hall_trig_wkup, 0444, hall_trig_wkup_show, NULL);

static ssize_t hall_debug_store(struct sys_device *dev, struct sysdev_attribute *attr, const char *buf, size_t size)
{
	int value = 0;
	if (sscanf(buf, "%d", &value) <= 0) {
		printk(KERN_ERR "Could not update hall debug ctrl \n");
		return -EINVAL;
	}
	hall_dbg = (value > 0) ? 1 : 0;
	return size;
}

static ssize_t hall_debug_show(struct sys_device *dev, struct sysdev_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", hall_dbg);
}
static DEVICE_ATTR(hall_debug, 0644, hall_debug_show, hall_debug_store);

static ssize_t hall_detect_show(struct sys_device *dev, struct sysdev_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", gpio_hallsensor_detect());
}
static DEVICE_ATTR(hall_detect, 0444, hall_detect_show, NULL);



static ssize_t hall_enable_store(struct sys_device *dev, struct sysdev_attribute *attr, const char *buf, size_t size)
{
	int value = 0;
	if (sscanf(buf, "%d", &value) <= 0) {
		printk(KERN_ERR "Could not update hall enable ctrl \n");
		return -EINVAL;
	}

	if (value > 0 && !hall_enabled) {
		hall_enabled = HALL_CTRL_ENABLE;
	} else if(value <= 0 && hall_enabled) {
		hall_enabled = HALL_CTRL_DISABLE;
	}
	hall_init_state();
	return size;
}
static ssize_t hall_enable_show(struct sys_device *dev, struct sysdev_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", hall_enabled);
}
static DEVICE_ATTR(hall_enable, 0644, hall_enable_show, hall_enable_store);

static void hall_send_close_event(void) 
{
	char *envp[] = {"HALLSENSOR=closed", NULL};
	kobject_uevent_env(&hall_misc_device.this_device->kobj, KOBJ_ONLINE, envp);	
	current_state = STATE_CLOSED;
}

static void hall_close_event_work_handler(struct work_struct *work)
{
	if(hall_enabled) {
		if(gpio_hallsensor_detect() && current_state == STATE_OPENED) {
			heisenberg_pwrkey_ctrl(0);	
			hall_send_close_event();
			if (hall_dbg) 
				printk(KERN_INFO "KERNEL: I hall:closed::current_state=%d\n",current_state);
		} 
	}
}

static void hall_open_event_work_handler(struct work_struct *work)
{
	if(!gpio_hallsensor_detect()) {
		/* hall in open state beyond the timeout so no need for delay */
		hall_close_delay = 0;
	}
}

static void hall_detwq_handler(struct work_struct *dummy)
{
	int irq = gpio_hallsensor_irq();

	if(hall_enabled) {
		if(gpio_hallsensor_detect()) {
			/* Hall - Close */
			cancel_delayed_work_sync(&hall_open_event_work);
			if (hall_close_delay) {
				/*	Note: To avoid framework overload during back-to-back cover open-close events
				 *	defer current close event when previous open event occured immiediately after close  
				 */
				schedule_delayed_work(&hall_close_event_work, msecs_to_jiffies(HALL_EVENT_DELAY));
			} else { 
				/* Hall debounce on a normal (cover close) scenario */
				schedule_delayed_work(&hall_close_event_work, msecs_to_jiffies(HALL_EVENT_DEB));
			}
			if (hall_dbg) 
				printk(KERN_INFO "KERNEL: I hall:close event::current_state=%d\n",current_state);
		} else {
			/* Hall - Open */
			char *envp[] = {"HALLSENSOR=opened", NULL};
			if (hall_dbg) 
				printk(KERN_INFO "KERNEL: I hall:open event::current_state=%d\n",current_state);
			cancel_delayed_work_sync(&hall_close_event_work);
			if (current_state != STATE_OPENED) {
				kobject_uevent_env(&hall_misc_device.this_device->kobj, KOBJ_OFFLINE, envp);
				current_state = STATE_OPENED;
				heisenberg_pwrkey_ctrl(1);
				if (hall_dbg) 
					printk(KERN_INFO "KERNEL: I hall:opened::current_state=%d\n",current_state);
			}
			hall_close_delay = 1;
			schedule_delayed_work(&hall_open_event_work, msecs_to_jiffies(HALL_EVENT_DELAY));
		}
	}
	hall_event = 1;	
	enable_irq(irq);
}

static DECLARE_WORK(hall_detwq, hall_detwq_handler); 

static irqreturn_t hall_isr(int irq, void *dev_id)
{
	disable_irq_nosync(irq);
	schedule_work(&hall_detwq);

	return IRQ_HANDLED;
}

/* Init Hall state during boot 
 * to avoid out of sync issue may due to bcut / wdog / HR
 */
static void hall_init_work_handler(struct work_struct *work)
{
	int irq = gpio_hallsensor_irq();
	hall_init_state();
	enable_irq_wake(irq);
}

static int heisenberg_hall_probe(struct platform_device *pdev)
{
	int error;
	int irq;
	int ret;
	struct device_node *np;

	struct hall_drvdata* ddata;
	struct hall_platform_data *pdata = pdev->dev.platform_data;

	ddata = kzalloc(sizeof(struct hall_drvdata) + sizeof(struct hall_drvdata),GFP_KERNEL);
	if (!ddata) {
		printk(KERN_INFO "ddata error\n");
		error = -ENOMEM;
		goto fail1;
	}
	platform_set_drvdata(pdev, ddata);
	ddata->pdata = pdata;

	if (misc_register(&hall_misc_device)) {
		error = -EBUSY;
		printk(KERN_ERR "%s Couldn't register device %d \n",__FUNCTION__, HALL_DEV_MINOR);
		return -EFAULT;
	}

	np = of_find_compatible_node(NULL, NULL, "amzn,heisenberg_hall");
	if (!np) {
		pr_err("%s: invalid pnode\n", __func__);
		return -EINVAL;
	}

	ret = of_get_named_gpio(np, "hall_int_n", 0);
	if (ret < 0) {
		pr_err("%s: get wl_host_wake GPIO failed err=%d\n", __func__, ret);
		return ret;
	}

	g_gpio_hall = ret;

	ret = gpio_request(g_gpio_hall, "hall_irq");
	if (ret) {
		pr_err("%s: failed to request gpio %d for hall err=%d\n", __func__, g_gpio_hall, ret);
		return ret;
	}

	ret = gpio_direction_input(g_gpio_hall);
	if (ret) {
		pr_err("%s: configuration failure err=%d\n", __func__, ret);
		gpio_free(g_gpio_hall);
		return ret;
	}

	/* Out Of Band interrupt */
	irq = gpio_to_irq(g_gpio_hall);
	if (irq < 0) {
		pr_err("%s: irq failure\n", __func__);
		gpio_free(g_gpio_hall);
		return -EINVAL;
	}

	hall_irq = irq;

	error = request_irq(irq, hall_isr, (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_EARLY_RESUME),HALL_DRIVER_NAME, pdata);
	if (error) {
		printk(KERN_ERR "%s Failed to claim irq %d, error %d \n",__FUNCTION__, irq, error);
		return -EFAULT;
	} 

	/* Configure wakeup capable */
	device_set_wakeup_capable(&pdev->dev, true);

        if (device_create_file(&pdev->dev, &dev_attr_hall_detect))
	{
                dev_err(&pdev->dev, "Unable to create file: hall_detect\n");
		return -EFAULT;
	}
        if (device_create_file(&pdev->dev, &dev_attr_hall_gpio_pullup))
	{
                dev_err(&pdev->dev, "Unable to create file: hall_gpio_pullup\n");
		return -EFAULT;
	}
        if (device_create_file(&pdev->dev, &dev_attr_hall_debug))
	{
                dev_err(&pdev->dev, "Unable to create file: hall_debug\n");
		return -EFAULT;
	}

        if (device_create_file(&pdev->dev, &dev_attr_hall_trig_wkup))
	{
                dev_err(&pdev->dev, "Unable to create file: hall_trig_wkup\n");
		return -EFAULT;
	}

	schedule_delayed_work(&hall_init_work, msecs_to_jiffies(HALL_INIT_DELAY));

	return 0;

fail3:
	misc_deregister(&hall_misc_device);
fail2:
	platform_set_drvdata(pdev, NULL);
	kfree(ddata);
fail1:
	return error;
}

static int heisenberg_hall_remove(struct platform_device *pdev)
{
	struct hall_platform_data *pdata = pdev->dev.platform_data;

	device_remove_file(&pdev->dev, &dev_attr_hall_detect);
	device_remove_file(&pdev->dev, &dev_attr_hall_gpio_pullup);
	device_remove_file(&pdev->dev, &dev_attr_hall_debug);
	device_remove_file(&pdev->dev, &dev_attr_hall_trig_wkup);

	cancel_delayed_work_sync(&hall_init_work);		
	cancel_delayed_work_sync(&hall_open_event_work);		
	cancel_delayed_work_sync(&hall_close_event_work);		

	misc_deregister(&hall_misc_device);
	device_set_wakeup_capable(&pdev->dev, false); 
	disable_irq_wake(pdata->hall_irq);
	free_irq(pdata->hall_irq, pdev->dev.platform_data);	
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static int heisenberg_hall_suspend(struct platform_device *pdev, pm_message_t state)
{
	is_hall_wkup = 0;
	hall_event = 0;
	return 0;
}

static int heisenberg_hall_resume(struct platform_device *pdev)
{
#if 0
	if (hall_event) {
		if (hall_dbg)
			printk(KERN_INFO "%s oneshot: %d\n", __func__, pb_oneshot);
		if (pb_oneshot == HIBER_SUSP) {
			pb_oneshot = HALL_IRQ;
			pb_oneshot_unblock_button_events();
		}

		is_hall_wkup = 1;
		hall_event = 0;
		if (hall_dbg) 
			printk(KERN_INFO "KERNEL: I hall:wakeup event::current_state=%d\n",current_state);
	} 
#endif
	return 0;
}


static void hall_nop_release(struct device* dev)
{
	/* Do Nothing */
}

static const struct of_device_id hall_of_match[] = {
        { .compatible = "heisenberg_hall", },
        {},
};

static struct platform_device heisenberg_hall_device =
{
	.name = HALL_DRIVER_NAME,
	.id   = 0,
	.dev  = {
		.release = hall_nop_release,
  	},
};

static struct platform_driver heisenberg_hall_driver = {
	.driver		= {
		.name	= HALL_DRIVER_NAME,
		.owner	= THIS_MODULE,		 
                .of_match_table = of_match_ptr(hall_of_match),

	},
	.probe		= heisenberg_hall_probe,
	.remove		= heisenberg_hall_remove,
	.suspend 	= heisenberg_hall_suspend,
	.resume 	= heisenberg_hall_resume,
};

static int __init heisenberg_hall_init(void)
{
	printk(KERN_INFO "Registering Heisenberg hall sensor platform device\n");
	platform_device_register(&heisenberg_hall_device);
	platform_driver_register(&heisenberg_hall_driver);
	return 0;
}

static void __exit heisenberg_hall_exit(void)
{	
	platform_driver_unregister(&heisenberg_hall_device);
	platform_driver_unregister(&heisenberg_hall_driver);
}

module_init(heisenberg_hall_init);
module_exit(heisenberg_hall_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lab126");
MODULE_DESCRIPTION("Hall sensor driver for heisenberg platform");
