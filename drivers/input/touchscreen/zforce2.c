/*
 * Copyright (c) 2010-2014 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * Nadim Awad (nawad@lab126.com)
 * Modified by Sandeep Marathe (msandeep@lab126.com)
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/freezer.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/poll.h>
#include <linux/byteorder/generic.h>
#include <linux/input/mt.h>
#include <linux/slab.h>
#include <linux/lab126_touch.h>
#include <linux/regulator/driver.h>
#include <linux/cpufreq.h>
//#include <mach/gpio.h>
#include <linux/gpio.h>
//#include <asm/mach-imx/hardware.h>
//#include <mach/iomux-mx6sl.h>
//#include "boardid.h"

#include "zforceint2.h"
#include <llog.h>

/* Reliability test only */
static int reltest = 0;
static int reltest_run = 0;
/* Enable debug logs */
static uint dbglog = ZFORCE2_LOG_ERROR;
static uint bpdr = 0;
static uint ignore_false_header = 1;
static uint touchdatapoll=0;
static uint bpbc=1;
static uint deviceType=2;
#ifdef MODULE
module_param_named(reltest, reltest, int, S_IRUGO);
MODULE_PARM_DESC(reltest, "LED reliability test");
module_param_named(dbglog, dbglog, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dbglog, "Enable Zforce2 debugging");
module_param_named(bpdr, bpdr, uint, S_IRUGO);
MODULE_PARM_DESC(bpdr, "bypass data_ready");
module_param_named(ignore_false_header, ignore_false_header, uint, S_IRUGO);
MODULE_PARM_DESC(ignore_false_header, "ignore false header");
module_param_named(touchdatapoll, touchdatapoll, uint, S_IRUGO);
MODULE_PARM_DESC(touchdatapoll, "ignore false header");
module_param_named(bpbc, bpbc, uint, S_IRUGO);
MODULE_PARM_DESC(bpbc, "bypass boot complete");
module_param_named(deviceType, deviceType, uint, S_IRUGO);
MODULE_PARM_DESC(deviceType, "deviceType: 1--TI, 2--ST");
#endif

ZForceDrvData     zforce_data;    // Main data structure
wait_queue_head_t zf_user_wq;     // Wait queue for user requested info
wait_queue_head_t zf_init_wq;     // Wait queue for starting and stopping driver
DEFINE_MUTEX(comm_mutex);         // Communication mutex
DEFINE_MUTEX(contactframe_mutex); // Contact frame mutex for testing
DEFINE_MUTEX(ledlvl_mutex);       // LED levels mutex
DEFINE_MUTEX(lowlvl_mutex);       // Low Level Signal mutex
DEFINE_MUTEX(stop_mutex);         //protect stop_touch and workqueue delete from reentrance
int initSuccess;                  // Initialize successfull

#if 0 //Fred pad control is done through device tree. no longer needed here
/* touch I/O pad gpio configs */
static iomux_v3_cfg_t zforce2_suspend_enter_pads[] = {
	MX6SL_PAD_KEY_ROW5__GPIO_4_3,
	MX6SL_PAD_KEY_ROW6__GPIO_4_5,
	MX6SL_PAD_KEY_COL0__GPIO_3_24,
	MX6SL_PAD_SD1_DAT4__GPIO_5_12,
	MX6SL_PAD_SD1_DAT5__GPIO_5_9,
};

static iomux_v3_cfg_t
zforce2_suspend_exit_pads[ARRAY_SIZE(zforce2_suspend_enter_pads)];
#endif 


#define IMX_GPIO_NR(bank, nr)           (((bank) - 1) * 32 + (nr))
#define	ASN_RESOLUTION_LEN		18
#define ST_RES_WIDTH			600
#define ST_RES_HEIGHT			800

static struct i2c_client *zforce_i2c_client; // I2C client
int i2c_probe_success;            // I2C probed successfully
static struct proc_dir_entry *proc_entry;



/* Protos */
static void zforce_stop_touch(ZFState state);
static void zforce_restart_touch(void);
static void zforce_set_ready_for_update(void);
static int zforce_suspend_touch(void);
static int zforce_resume_touch(void);
static void sendTouchUpdate(TouchData *tData);

void processData(u8 request, ZFStatus *result);
void processAsnData(ZFASNMsgType request, ZFStatus *result);
ZFStatus sendForceLedLevelRequestY(void);
ZFStatus sendForceLedLevelRequestX(void);
ZFStatus sendForceCalibration(void);
ZFStatus sendPulseSignalInfo(char time, char strength);
ZFStatus initialize(void);
ZFStatus sendMcuSettingsRequest(void);
ZFStatus sendSetScanningFrequency(u16, u16, u16);
ZFStatus stop(void);
ZFStatus sendAsnEnableDevice(u8 enable);
ZFStatus sendAsnGetLedLevel(u8 enable);
ZFStatus SendAsnEnableCalibration(u8 enable);
ZFStatus sendAsnSetDetection();
ZFStatus initializeAsnDevice();
ZFStatus sendAsnSetResolution(u16,u16);
void zforce_free_touch_irq(void);
int zforce_get_touch_irq(void);
void bslReset(ZFState state);

/* Externs */
extern int  gpio_touchcntrl_irq(void);
extern void gpio_touchcntrl_request_irq(int enable);
extern int  gpio_touchcntrl_irq_get_value(void);
extern void gpio_zforce_init_pins(void);
extern int gpio_zforce_reset_ena(int enable);
extern int gpio_zforce_bslpins_ena(int enable);
extern void gpio_zforce_set_reset(int val);
extern void gpio_zforce_set_bsl_test(int val);
extern void gpio_zforce_free_pins(void);
extern void touch_suspend(void);
extern void touch_resume(void);


static void zforce_init_work_handler(struct work_struct *);
DECLARE_WORK(zforce_init_work, zforce_init_work_handler);

#define ZFORCE_DATA_READY (!gpio_touchcntrl_irq_get_value())
static void print_data_block(unsigned char *data, u8 length)
{

	if(dbglog >= ZFORCE2_LOG_INFO) {
		char buf[1024];
		unsigned buf_len = sizeof(buf);
		char *p = buf;
		int i;
		int l;

		for (i = 0; i < length && buf_len; i++, p += l, buf_len -= l) {
			l = snprintf(p, buf_len, "[0x%02X] ", data[i]);
		}
		printk(KERN_INFO "%s\n", buf);
	}
}

static void zforce_use_poll(void)
{
	/* lets wait for current irq to complete */
	disable_irq(zforce_data.irq);
}

static void zforce_use_intr(void)
{
	enable_irq(zforce_data.irq);
}

/* BSL Update */
void bslReset(ZFState state)
{
	DEBUG_INFO("Calling bslReset : %d\n", (int)state);
	gpio_zforce_reset_ena(true);

	switch(state) {
	case ZF_STATE_UPDATE:
	{
		printk(KERN_ERR "Entering BSL Mode..\n");
		DEBUG_INFO("ZForce triggering BSLRESET\n");
		gpio_zforce_bslpins_ena(true);
		msleep(100);
		gpio_zforce_set_reset(0); /* RST  pin: GND */
		gpio_zforce_set_bsl_test(0);  /* TEST pin: GND */
		udelay(10);
		gpio_zforce_set_bsl_test(1);  /* TEST pin: VCC */
		udelay(10);
		gpio_zforce_set_bsl_test(0);  /* TEST pin: GND */
		udelay(20);
		gpio_zforce_set_bsl_test(1);  /* TEST pin: VCC */
		udelay(10);
		gpio_zforce_set_reset(1); /* RST  pin: VCC */
		udelay(10);
		gpio_zforce_set_bsl_test(0);  /* TEST pin: GND */
		msleep(100);
		break;
	}
	case ZF_STATE_RESET:
	{
		/* Trigger ZForce reset */
		DEBUG_INFO("ZForce triggering RESET\n");
		/* turn off BSL lines when triggering 
		 * reset into Normal mode */
		gpio_zforce_bslpins_ena(false);
		mdelay(10);
		gpio_zforce_set_reset(0);
		mdelay(20);
		gpio_zforce_set_reset(1);
		mdelay(10);
		break;
	}
	case ZF_STATE_STOP:
	{
		DEBUG_INFO("ZForce triggering HALT/STOP\n");
		stop();
		/* set ZForce to stay in RST state */
		gpio_zforce_set_reset(0);
		break;
	}
	default:
		DEBUG_ERR("Unknown ZForce state!!");
	}

	gpio_zforce_reset_ena(false);
}

/**** MISC DEVICE ****/
static long
zforce_misc_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int ret = 0;

	DEBUG_INFO("IN IOCTL HANDLING\n");
	switch(cmd) {
	case TOUCH_IOCTL_GET_LOCK:
	{
		int locked = zforce_data.exlock;
		if (copy_to_user((void *)arg, &locked, sizeof(locked)))
			return	-EFAULT;
		break;
	}
	case TOUCH_IOCTL_SET_LOCK:
	{
		int locked;
		if (copy_from_user(&locked, (void *)arg, sizeof(locked)))
			return -EFAULT;
		if (zforce_data.exlock == (bool)locked)
			break;
               	if (locked > 0) {
			ret = zforce_suspend_touch();
			if(unlikely(ret)) return ret;
			zforce_data.exlock = true;
                } else {
			ret = zforce_resume_touch();
			if(unlikely(ret)) return ret;
			zforce_data.exlock = false;
		}
		break;
	}
	case ZF_GET_LED_LVL:
	{
		DEBUG_ERR("ZF_GET_LED_LVL not supported\n");
		break;
	}
	case ZF_FORCE_GET_LED_LVL_X:
	{
		DEFINE_WAIT(wait);

		if(!zforce_data.isConnected)
			return -EFAULT;

		DEBUG_INFO("Got IOCTL FORCE_GET_LED_LVL_X\n");
		sendForceLedLevelRequestX();
		if(!wait_event_timeout(zf_user_wq, zforce_data.ledlvlx_ready != 0, msecs_to_jiffies(30000)))
		{
			DEBUG_ERR("Timeout getting led levels\n");
			return -EFAULT;
		}
		mutex_lock(&ledlvl_mutex);
		zforce_data.ledlvlx_ready = 0;
		if (copy_to_user(argp, &zforce_data.ledLvl, sizeof(LedLevelResponse)))
		{
			DEBUG_ERR("copy_to_user error!!\n");
			mutex_unlock(&ledlvl_mutex);
			return -EFAULT;
		}
		mutex_unlock(&ledlvl_mutex);
		break;
	}
        case ZF_FORCE_GET_LED_LVL_Y:
        {
                DEFINE_WAIT(wait);

                if(!zforce_data.isConnected)
                        return -EFAULT;

                DEBUG_INFO("Got IOCTL FORCE_GET_LED_LVL_Y\n");
                sendForceLedLevelRequestY();
                if(!wait_event_timeout(zf_user_wq, zforce_data.ledlvly_ready != 0, msecs_to_jiffies(30000)))
                {
                        DEBUG_ERR("Timeout getting led levels\n");
                        return -EFAULT;
                }
                mutex_lock(&ledlvl_mutex);
                zforce_data.ledlvly_ready = 0;
                if (copy_to_user(argp, &zforce_data.ledLvl, sizeof(LedLevelResponse)))
                {
                        mutex_unlock(&ledlvl_mutex);
                        return -EFAULT;
                }
                mutex_unlock(&ledlvl_mutex);
                break;
        }

	case ZF_FORCE_CALIBRATION:
	{
		if (!zforce_data.isConnected)
          		return -1;

		sendForceCalibration();
        	if (!wait_event_timeout(zf_user_wq, zforce_data.force_calib_sent != 0, msecs_to_jiffies(2000)))
        	{
			DEBUG_ERR("Timeout sending force calibration\n");
			return -1;
        	}
        	break;
	}
    	case ZF_GET_RAW_DIODE_LVLS:
      	{
		if (!zforce_data.raw_diode_ready)
        	{
			FixedPulseStrengthResponse rawData;
			memset(&rawData, 0, sizeof(FixedPulseStrengthResponse));
			if (copy_to_user(argp, &rawData, sizeof(FixedPulseStrengthResponse)))
          		{
				zforce_data.raw_diode_ready = 0;
				return -EFAULT;
          		}
          		break;
        	}

		mutex_lock(&ledlvl_mutex);
		zforce_data.raw_diode_ready = 0;
		if (copy_to_user(argp, &zforce_data.rawDiode, sizeof(FixedPulseStrengthResponse)))
        	{
			mutex_unlock(&ledlvl_mutex);
			return -EFAULT;
        	}
		mutex_unlock(&ledlvl_mutex);
		break;
	}
	case ZF_GET_LOW_SIG_INFO:
	{
		if(!zforce_data.isConnected)
			return -1;
		DEBUG_INFO("Got IOCTL GET_LOW_LVL_INFO\n");
        	mutex_lock(&lowlvl_mutex);
		if(copy_to_user(argp, &zforce_data.lowSig, sizeof(LowSignalAlert)))
		{
			mutex_unlock(&lowlvl_mutex);
			return -EFAULT;
		}
		mutex_unlock(&lowlvl_mutex);
		break;
	}
	case ZF_SET_FIXED_PULSE:
	{
		PulseSignalInfo pulseSignalInfo;
		if (!zforce_data.isConnected)
			return -EFAULT;

		if(copy_from_user(&pulseSignalInfo, argp, sizeof(PulseSignalInfo)))
			return -EFAULT;
		sendPulseSignalInfo(pulseSignalInfo.time, pulseSignalInfo.strength);
	        break;
	}
	case ZF_SET_STATE_UPDATE:
	{
		if (atomic_read(&zforce_data.state) == ZF_STATE_UPDATE)
		{
			DEBUG_ERR(ZF_ERR_IN_UPDATE);
			return -EINVAL;
		}
		zforce_set_ready_for_update();
		break;
	}
	case ZF_SET_STATE_RUN:
	{
		if (atomic_read(&zforce_data.state) == ZF_STATE_RUN)
			return -EINVAL;
		zforce_restart_touch();
		break;
	}
	default:
		DEBUG_ERR("Unknown ioctl provided\n");
		return -EINVAL;
		break;
	}
	return 0;
}

static ssize_t zforce_misc_write(struct file *file, const char __user *buf,
	size_t count, loff_t *pos)
{
	return 0;
}

static ssize_t zforce_misc_read(struct file *file, char __user *buf,
	size_t count, loff_t *pos)
{
	return 0;
}

static const struct file_operations zforce_misc_fops =
{
	.owner = THIS_MODULE,
	.read  = zforce_misc_read,
	.write = zforce_misc_write,
	.unlocked_ioctl = zforce_misc_unlocked_ioctl,
};

static struct miscdevice zforce_misc_device =
{
	.minor = ZF_DEV_MINOR,
	.name  = TOUCH_MISC_DEV_NAME,
	.fops  = &zforce_misc_fops,
};

/**** END MISC DEVICE ****/

/**** PROC ENTRY ****/
//int zforce_proc_read(char *page, char **start, off_t off,
//	int count, int *eof, void *data)
static ssize_t zforce_proc_read(struct file *file, char __user *buf,
	size_t count, loff_t *off)
{
	int len;
	int state = atomic_read(&zforce_data.state);

	if (off > 0) {
		//*eof = 1;
		return 0;
	}

	if (state == ZF_STATE_STOP)
		len = sprintf(file, "touch is locked\n");
	else
		len = sprintf(file, "touch is unlocked\n");
	return len;
}

//ssize_t zforce_proc_write( struct file *filp, const char __user *buff,
//	unsigned long len, void *data )
ssize_t zforce_proc_write(struct file *filp, const char __user *buff, size_t len, loff_t *data)
{
	int ret = 0;
	char command[ZF_PROC_CMD_LEN];

	if (len > ZF_PROC_CMD_LEN) {
		DEBUG_ERR("zforce_proc_write:proc:command is too long!\n");
		return -ENOSPC;
	}

	if (copy_from_user(command, buff, len)) {
		DEBUG_ERR("zforce_proc_write:proc::cannot copy from user!\n");
		return -EFAULT;
	}

	if ( !strncmp(command, "unlock", 6) ) {
		if (atomic_read(&zforce_data.state) == ZF_STATE_STOP) {
			ret = zforce_resume_touch();
			if(unlikely(ret)) return 0;
			printk("zforce2: I zforce_proc_write::command=unlock:\n");
			zforce_data.exlock = false;
		}
	} else if ( !strncmp(command, "lock", 4) ) {
		if (atomic_read(&zforce_data.state) == ZF_STATE_RUN) {
			ret = zforce_suspend_touch();
			if(unlikely(ret)) return 0;
			printk("zforce2: I zforce_proc_write::command=lock:\n");
			zforce_data.exlock = true;
		}
	} else {
		DEBUG_ERR("zforce_proc_write:proc:command=%s:Unrecognized command\n", command);
	}
	return len;
}

static const struct file_operations proc_file_fops = {
 .owner = THIS_MODULE,
 .read  = zforce_proc_read,
 .write = zforce_proc_write,
};
/**** END PROC ENTRY ****/

/* Sysfs */
static ssize_t
zforce_connected_show(struct device* dev,
	struct device_attribute* attr,
	char* buf)
{
	return sprintf(buf, "%d\n", zforce_data.isConnected);
}
static DEVICE_ATTR(connected, 0444, zforce_connected_show, NULL);

static ssize_t
zforce_version_show(struct device* dev,
	struct device_attribute* attr,
	char* buf)
{
	return sprintf(buf, "%d.%db%dr%d\n",
	zforce_data.statusReqResp.version.major,
	zforce_data.statusReqResp.version.minor,
	zforce_data.statusReqResp.version.build,
	zforce_data.statusReqResp.version.revision);
}
static DEVICE_ATTR(version, 0444, zforce_version_show, NULL);


static ssize_t
zforce_mcusettings_show(struct device* dev,
	struct device_attribute* attr,
	char* buf)
{
	int ret = 0;
	ZFStatus status = ZF_OK;

	atomic_set(&zforce_data.mcusetting_ready, 0);

	zforce_use_poll();

	status = sendMcuSettingsRequest();
        if (status != ZF_OK) {
		zforce_use_intr();
		return 0; /* return zero len buf */
	}

	if(!wait_event_timeout(zf_user_wq, atomic_read(&zforce_data.mcusetting_ready) != 0, msecs_to_jiffies(30000))) {
		zforce_use_intr();
		printk(KERN_ERR "Timeout getting MCU settings response!\n");
		return -EFAULT;
	}

	zforce_use_intr();

	ret += sprintf(buf, "Firmware version: %d.%d.%d.%d\n",
		zforce_data.statusReqResp.version.major,
		zforce_data.statusReqResp.version.minor,
		zforce_data.statusReqResp.version.build,
		zforce_data.statusReqResp.version.revision);

	ret += sprintf(buf+ret, "Current contacts: %d\tScan activated: %s\n",
		(zforce_data.statusReqResp.contact_scanactv & 0x7F),
		((zforce_data.statusReqResp.contact_scanactv >> 7) ? "true": "false"));

	ret += sprintf(buf+ret, "Scanning counter: %d, Prepared touch pkgs: %d\n"
		"Sent touch pkgs: %d, Invalid touch counter: %d\n",
		zforce_data.statusReqResp.scanning_counter,
		zforce_data.statusReqResp.prep_touch_pkgs,
		zforce_data.statusReqResp.sent_touch_pkgs,
		zforce_data.statusReqResp.invld_touch_cntr);

	ret += sprintf(buf+ret, "Configuration: %d\n",
		zforce_data.statusReqResp.config.flags);

	ret += sprintf(buf+ret, "Scanning frequency:-\n"
		"Idle: %d, Finger: %d, Stylus: %d\n",
		zforce_data.statusReqResp.freqData.idle,
		zforce_data.statusReqResp.freqData.finger,
	        zforce_data.statusReqResp.freqData.stylus);

	ret += sprintf(buf+ret, "Resolution width: %d, height %d\n",
		zforce_data.statusReqResp.resData.X,
		zforce_data.statusReqResp.resData.Y);

	ret += sprintf(buf+ret, "Physical width: %d, height %d\n",
		zforce_data.statusReqResp.phys_width,
		zforce_data.statusReqResp.phys_height);

	ret += sprintf(buf+ret, "Active LEDS:-\n"
		"X-axis first activeLED: %d\t"
		"X-axis last activeLED: %d\n"
		"Y-axis first activeLED: %d\t"
		"Y-axis last activeLED: %d\n",
		zforce_data.statusReqResp.activeLeds.x_start,
		zforce_data.statusReqResp.activeLeds.x_end,
		zforce_data.statusReqResp.activeLeds.y_start,
		zforce_data.statusReqResp.activeLeds.y_end);

	ret += sprintf(buf+ret, "Max signal strength: %d\n",
		zforce_data.statusReqResp.max_ss);

	ret += sprintf(buf+ret, "Max Size Enabled: %d\tMax Size: %d\n",
		zforce_data.statusReqResp.maxsz_ena,
		zforce_data.statusReqResp.maxsz);

	ret += sprintf(buf+ret, "Min Size Enabled: %d\tMin Size: %d\n",
		zforce_data.statusReqResp.minsz_ena,
		zforce_data.statusReqResp.minsz);

	ret += sprintf(buf+ret, "Interface Protocol major: %d, minor: %d\n",
		zforce_data.statusReqResp.inf_prtcl_major,
		zforce_data.statusReqResp.inf_prtcl_minor);

	return ret;
}
static DEVICE_ATTR(mcusetting, 0444, zforce_mcusettings_show, NULL);

/* Raw state of data ready gpio */
static ssize_t
zforce_inputgpio_show(struct device* dev,
	struct device_attribute* attr,
	char *buf)
{
	return sprintf(buf, "%d\n", !ZFORCE_DATA_READY);
}
static DEVICE_ATTR(inputgpio, 0444, zforce_inputgpio_show, NULL);

/* Reset and BSLReset hooks */
static ssize_t
zforce_bslreset_store(struct device* dev,
	struct device_attribute* attr,
	const char* buf,
	size_t size)
{
	ZFState state;

	if (sscanf(buf, "%d", (int*)&state) <= 0)
		return -EINVAL;

	switch(state) {
	case(ZF_STATE_UPDATE):
	{
		if (atomic_read(&zforce_data.state) == ZF_STATE_UPDATE)
		{
			DEBUG_ERR(ZF_ERR_IN_UPDATE);
			break;
		}
		zforce_set_ready_for_update();
		break;
	}
	case(ZF_STATE_RUN):
	{
		if (atomic_read(&zforce_data.state) == ZF_STATE_RUN)
			break;
		zforce_restart_touch();
		break;
	}
	case(ZF_STATE_RESET):
	{
		zforce_stop_touch(ZF_STATE_RESET);
		zforce_restart_touch();
		break;
	}
	case(ZF_STATE_STOP):
	{
		if (atomic_read(&zforce_data.state) == ZF_STATE_STOP)
			break;
		zforce_stop_touch(ZF_STATE_STOP);
		break;
	}
	default:
	{
		DEBUG_ERR(ZF_ERR_UNKNOWN_STATE);
		break;
	}
	}
	return size;
}

static ssize_t
zforce_bslreset_show(struct device* dev,
	struct device_attribute* attr,
	char* buf)
{
	return sprintf(buf, "%d\n", (int)atomic_read(&zforce_data.state));
}
static DEVICE_ATTR(bslreset, 0644, zforce_bslreset_show, zforce_bslreset_store);

static ssize_t
zforce_scanningfreq_show(struct device* dev,
			struct device_attribute* attr,
				char* buf)
{
	return sprintf(buf, "idle: %u, finger: %u, stylus: %u", zforce_data.statusReqResp.freqData.idle,
		zforce_data.statusReqResp.freqData.finger,
		zforce_data.statusReqResp.freqData.stylus);
}

static ssize_t
zforce_scanningfreq_store(struct device* dev,
	struct device_attribute* attr,
	const char* buf, size_t size)
{
        ZFStatus status = ZF_OK;
	unsigned int idle = 0, finger = 0, stylus = 0;

	if (sscanf(buf, "%u %u %u", &idle, &finger, &stylus) != 3)
		return -EINVAL;

	zforce_use_poll();

	if((status = sendSetScanningFrequency(idle, finger, stylus))
		!= ZF_OK) {
		zforce_use_intr();
		printk(KERN_ERR "\nsendSetScanningFrequency returned err \
		%d\n", status);
		return -EIO;
	}

	zforce_use_intr();

	return size;
}

static DEVICE_ATTR(scanningfreq, 0644, zforce_scanningfreq_show, zforce_scanningfreq_store);

static ssize_t
zforce_ledlevel_show( struct device* dev,
	struct device_attribute* attr,
	char* buf )
{
	int i;
	int ret = 0;
	ZFStatus result = ZF_OK;
	LedLevelResponse *ledLvl = &zforce_data.ledLvl;

	if(deviceType == 2) { //ST
		DEBUG_ERR("ST MCU LED Level\n");

		zforce_free_touch_irq();
	
		bslReset(ZF_STATE_RESET);

		processAsnData(ZF_ASN_BootComplete, &result);
		if (sendAsnEnableDevice(0) != ZF_OK) {
			DEBUG_ERR("Fail to disable core\n");
			return ZF_ERROR;
		}
		if (sendAsnGetLedLevel(1) != ZF_OK) {
			DEBUG_ERR("Fail to send get led level request\n");
			return ZF_ERROR;
		}
		if (SendAsnEnableCalibration(1) != ZF_OK) {
			DEBUG_ERR("Fail to calibration\n");
			return ZF_ERROR;
		}
		if (sendAsnEnableDevice(1) != ZF_OK) {
			DEBUG_ERR("Fail to enable core\n");
			return ZF_ERROR;
		}

		processAsnData(ZF_ASN_GetLedLevel, &result);

		if (sendAsnEnableDevice(0) != ZF_OK) {
			DEBUG_ERR("Fail to disable core\n");
			return ZF_ERROR;
		}

		ret += sprintf(buf+ret, "ST MCU zforce\n");
		ret += sprintf(buf+ret, "\nLedLevel X(%d):\n",ledLvl->xCount);
		for( i = 0; i < ledLvl->xCount; i++) {
			ret += sprintf(buf+ret, "%2x %2x ", 
				ledLvl->xLedLevel.xAsnLEDS[i].LedLevel1,
				ledLvl->xLedLevel.xAsnLEDS[i].LedLevel2);
		}
		ret += sprintf(buf+ret, "\nLedLevel Y(%d):\n",ledLvl->yCount);
		for( i = 0; i < ledLvl->yCount; i++) {
			ret += sprintf(buf+ret, "%2x %2x ", 
				ledLvl->yLedLevel.yAsnLEDS[i].LedLevel1,
				ledLvl->yLedLevel.yAsnLEDS[i].LedLevel2);
		}
		ret += sprintf(buf+ret, "\n");

		
		if (sendAsnGetLedLevel(0) != ZF_OK) {
			DEBUG_ERR("Fail to send get led level request\n");
			return ZF_ERROR;
		}
		if (SendAsnEnableCalibration(0) != ZF_OK) {
			DEBUG_ERR("Fail to calibration\n");
			return ZF_ERROR;
		}
		if (sendAsnSetDetection() != ZF_OK) {
			DEBUG_ERR("Fail to set detection mode\n");
			return ZF_ERROR;
		}
		if (sendAsnEnableDevice(1) != ZF_OK) {
			DEBUG_ERR("Fail to enable core\n");
			return ZF_ERROR;
		}

		zforce_get_touch_irq();
		
	} else if (deviceType == 1) { 

		DEBUG_ERR("TI MCU LED Level\n");

		zforce_data.ledlvlx_ready = 0;
		zforce_use_poll();
		result = sendForceLedLevelRequestX();
		if (result != ZF_OK) {
			zforce_use_intr();
			return 0;
		}
		if(!wait_event_timeout(zf_user_wq, zforce_data.ledlvlx_ready != 0, msecs_to_jiffies(30000)))
		{
			printk(KERN_INFO "Timeout getting led levels\n");
			return -EFAULT;
		}
		zforce_data.ledlvlx_ready = 0;
		
		DEBUG_INFO("Got IOCTL FORCE_GET_LED_LVL_Y\n");

		zforce_data.ledlvly_ready = 0;
		result = sendForceLedLevelRequestY();
		if (result != ZF_OK) {
			zforce_use_intr();
			return 0;
		}
		if(!wait_event_timeout(zf_user_wq, zforce_data.ledlvly_ready != 0, msecs_to_jiffies(30000)))
		{
			DEBUG_INFO("Timeout getting led levels\n");
			return -EFAULT;
		}
		zforce_data.ledlvly_ready = 0;
		
		zforce_use_intr();

		mutex_lock(&ledlvl_mutex);
		ret += sprintf(buf+ret, "TI MCU zforce\n");
		ret += sprintf(buf+ret, "\nLedLevel X(%d):\nLedLvl0:LedLvl1, Signal 0, Signal 1\n",ledLvl->xCount);
		for(i = 0; i < ledLvl->xCount; i++) {
			ret += sprintf(buf+ret, "%2x, %2x, %2x\n", 
				((ledLvl->xLedLevel.xLEDS[i].LedStrength1<<4)&0xF0) |
				(ledLvl->xLedLevel.xLEDS[i].LedStrength2)&0xF,
				ledLvl->xLedLevel.xLEDS[i].PDSignal1,
				ledLvl->xLedLevel.xLEDS[i].PDSignal2 );
		}
		ret += sprintf(buf+ret, "\nLedLevel Y(%d):\nLedLvl0:LedLvl1, Signal 0, Signal 1\n",ledLvl->yCount);
		for(i = 0; i < ledLvl->yCount; i++) {
			ret += sprintf(buf+ret, "%2x, %2x, %2x\n", 
				((ledLvl->yLedLevel.yLEDS[i].LedStrength1<<4)&0xF0) |
				(ledLvl->yLedLevel.yLEDS[i].LedStrength2)&0xF,
				ledLvl->yLedLevel.yLEDS[i].PDSignal1,
				ledLvl->yLedLevel.yLEDS[i].PDSignal2 );
		}
		mutex_unlock(&ledlvl_mutex);
	}

	return ret;
}

static DEVICE_ATTR(ledlevel, 0444, zforce_ledlevel_show, NULL);

static ssize_t
zforce_mode_show(struct device* dev, struct device_attribute* attr, char* buf)
{
	return sprintf(buf, "%d\n", zforce_data.mode);
}

/* Switch between I2C and Test mode of operation */
static ssize_t
zforce_mode_store(struct device* dev,
	struct device_attribute* attr,
	const char* buf,
	size_t size)
{
	ZFMode mode;

	if (sscanf(buf, "%d", (int*)&mode) <= 0)
		return -EINVAL;
	if (mode == zforce_data.mode)
		return -EINVAL;
	if (mode > MODE_TEST) {
		DEBUG_ERR("Invalid mode of operation\n");
		return -EINVAL;
	}

	zforce_data.tData.count      = 0;
	zforce_data.mode             = MODE_I2C;

	if (mode == MODE_TEST) {
		zforce_stop_touch(ZF_STATE_STOP);
		zforce_data.mode = mode;
	}
	else if (mode == MODE_I2C) {
		zforce_restart_touch();
	}

	return size;
}
static DEVICE_ATTR(mode, 0644, zforce_mode_show, zforce_mode_store);

static ssize_t
zforce_test_show(struct device* dev,
	struct device_attribute* attr,
	char* buf)
{
	return sprintf(buf, "Num active contacts : %d\n", zforce_data.tData.count);
}

/* Virtual contact creation */
static ssize_t
zforce_test_store(struct device* dev,
	struct device_attribute* attr,
	const char*  buf,
	size_t size)
{
	ZFTestEventType type;
	CoordinateData  contact;
	int coord_id;


	coord_id = MAX_CONTACTS;
	contact.X = -1;
	contact.Y = -1;

	if (zforce_data.mode != MODE_TEST)
	{
		DEBUG_ERR("Set the operational mode to Diagnostics in the mode /sys file\n");
		return -EINVAL;
	}

	if (sscanf(buf, "%d %d %hu %hu", (int*)&type, &coord_id,
		(unsigned short*)&contact.X,
		(unsigned short*)&contact.Y) <= 0)
	return -EINVAL;

	mutex_lock(&contactframe_mutex);

	switch(type)
	{
	case(TEST_CONTACT_SET):
	{
		if (coord_id >= MAX_CONTACTS) {
			DEBUG_ERR("Provided id is too high. Max value is %d\n", MAX_CONTACTS);
			mutex_unlock(&contactframe_mutex);
			return -EINVAL;
		}
		if (contact.X >= X_RESOLUTION || contact.X < 0 ||
			contact.Y >= Y_RESOLUTION || contact.Y < 0) {
			DEBUG_ERR("X and Y need to be between 0 and %d excluded\n", X_RESOLUTION);
			mutex_unlock(&contactframe_mutex);
			return -EINVAL;
		}
		contact.touch_state = TOUCH_MOVE;
		contact.id          = coord_id+1;
		zforce_data.tData.coordinateData[coord_id] = contact;
		DEBUG_INFO("Set Contact : [%d,%d,%d]\n", coord_id, contact.X, contact.Y);
		break;
	}
	case(TEST_CONTACT_REMOVE):
	{
		if (coord_id >= MAX_CONTACTS) {
			DEBUG_ERR("Provided id is too high. Max value is %d\n", MAX_CONTACTS);
			mutex_unlock(&contactframe_mutex);
			return -EINVAL;
		}
		zforce_data.tData.coordinateData[coord_id].touch_state = TOUCH_UP;
		DEBUG_INFO("Remove Contact : [%d]\n", coord_id);
		break;
	}
	case(TEST_CONTACT_CLEAR):
	{
		int i;
		for (i = 0; i < MAX_CONTACTS; i++)
		{
			zforce_data.tData.coordinateData[i].id = i+1;
			zforce_data.tData.coordinateData[i].touch_state = TOUCH_UP;
		}
		DEBUG_INFO("Clear all contacts\n");
		break;
	}
	default:
	{
		mutex_unlock(&contactframe_mutex);
		return -EINVAL;
	}
	}

	sendTouchUpdate(&zforce_data.tData);
	mutex_unlock(&contactframe_mutex);

	return size;
}
static DEVICE_ATTR(test, 0644, zforce_test_show, zforce_test_store);

/**** END Sysfs ****/
/* Poll keep the data ready GPIO */
static int zforce_touch_poll_keep(void)
{
	int status = 0;
	uint retry = 0;

	while(status == 0)
	{
		if (atomic_read(&zforce_data.state) != ZF_STATE_RUN)
			break;
		if(bpdr)
			status= 1;
		else 
			status=ZFORCE_DATA_READY;
		DEBUG_INFO("touch_poll: status is %d, retry:%d\n",status,retry);
		if (status)
			break;
		retry++;
		mdelay(1000);
	}
	return (status == 1 ? 0 : -ETIMEDOUT);
}

static zforce_touch_data_poll()
{
	ZFStatus result = ZF_OK;
	while(1)
	{ 
		if (zforce_touch_poll_keep())
			return ZF_ERROR;

		mutex_lock(&comm_mutex);
		processData(TYPE_TOUCH_DATA_RES, &result);
		mutex_unlock(&comm_mutex);
		if(result != ZF_OK) {
			DEBUG_INFO("Touch data data pull result=%d\n",result);
			//return result;
		}
		DEBUG_INFO("Touch data data pull SUCCESS\n");
	}
	return ZF_OK;

}

/* IRQ handler funcs */
static irqreturn_t zforce_irq_thread(int irq, void *unused)
{
	ZFStatus status;

	if(likely(!reltest)) {

		mutex_lock(&comm_mutex);
		if(deviceType==1)
		{
			processData(TYPE_TOUCH_DATA_RES, &status);
		}
		else if(deviceType==2)
		{
			processAsnData(ZF_ASN_TOUCH_NOTIF, &status);
		}
		else
		{
			DEBUG_ERR("unknown Device Type: %d\n",deviceType);
			zforce_stop_touch(ZF_STATE_STOP);
		}
		mutex_unlock(&comm_mutex);
		if(status != ZF_OK) {
			DEBUG_ERR("processData failed with ret code %d", status);
			if(status == ZF_ERR_FORCE_RST) {
				atomic_set(&zforce_data.state, ZF_STATE_RESET);
				queue_work(zforce_data.work_queue, &zforce_init_work);
			}
		}
	} else {
		mutex_lock(&comm_mutex);
		processData(TYPE_RAW_DIODE_DATA_RES, &status);
		mutex_unlock(&comm_mutex);
		if(status != ZF_OK) {
			/* Stop touch and reltest in case error encountered */
			zforce_stop_touch(ZF_STATE_STOP);
			DEBUG_ERR("processData failed with ret code %d", status);
		}
	}
	return IRQ_HANDLED;
}

static irqreturn_t zforce_quick_check_irqhandler(int irq, void *unused)
{
	DEBUG_INFO("******* INTERRUPT ********\n");
	if (atomic_read(&zforce_data.state) != ZF_STATE_RUN)
		return IRQ_HANDLED;

	if (!zforce_data.work_queue)
		return IRQ_HANDLED;

	return IRQ_WAKE_THREAD;
}

/* Poll the data ready GPIO */
static int zforce_touch_poll(void)
{
	int status = 0, retry = 20;
	while(status == 0 && retry > 0)
	{
		if (atomic_read(&zforce_data.state) != ZF_STATE_RUN)
			break;
		if(bpdr)
			status= 1;
		else 
			status=ZFORCE_DATA_READY;
		DEBUG_INFO("touch_poll: status is %d\n",status);
		if (status)
			break;
		retry--;
		mdelay(20);
	}
	return (status == 1 ? 0 : -ETIMEDOUT);
}

static int register_input_device(struct platform_device *pdev)
{
	int i   = 0;
	int ret = 0;
	struct input_dev *tdev;

	tdev = input_allocate_device();
	if (!tdev)
	{
		DEBUG_ERR(ZF_ERR_NO_MEM_INPUT);
		return -ENOMEM;
	}

	tdev->name = ZF_INPUT_NAME;
	tdev->dev.parent = &pdev->dev;
	tdev->phys = "zforce2/input0";

	__set_bit(EV_KEY, tdev->evbit);
	__set_bit(EV_ABS, tdev->evbit);
	__set_bit(BTN_TOUCH, tdev->keybit);
	__set_bit(BTN_TOOL_FINGER, tdev->keybit);
	__set_bit(BTN_TOOL_DOUBLETAP, tdev->keybit);
	__set_bit(BTN_TOOL_TRIPLETAP, tdev->keybit);

	// Register input multi-touch slots
	input_mt_init_slots(tdev, MAX_CONTACTS,INPUT_MT_DIRECT);
	input_set_abs_params(tdev, ABS_MT_SLOT      ,  0, MAX_CONTACTS - 1,  0, 0);
	input_set_abs_params(tdev, ABS_MT_POSITION_X,  0, X_RESOLUTION - 1, 0, 0);
	input_set_abs_params(tdev, ABS_MT_POSITION_Y,  0, Y_RESOLUTION - 1, 0, 0);
	input_set_abs_params(tdev, ABS_MT_TRACKING_ID, 0, 65535             , 0, 0);

	ret = input_register_device(tdev);
	if (ret < 0)
	{
		input_free_device(tdev);
		DEBUG_ERR(ZF_ERR_INPUT_MT);
		return ret;
	}

	// Make sure we clear the MT_SLOT state
	for(i = 0; i < MAX_CONTACTS; i++)
	{
		input_report_abs(tdev, ABS_MT_SLOT, i);
		input_report_abs(tdev, ABS_MT_TRACKING_ID, -1);
	}

	zforce_data.tdev = tdev;
	return 0;
}

static void remove_input_device(void)
{
	if (zforce_data.tdev)
	{
		input_mt_destroy_slots(zforce_data.tdev);
		input_unregister_device(zforce_data.tdev);
		input_free_device(zforce_data.tdev);
		zforce_data.tdev = NULL;
	}
}

struct zforce_info {
	struct i2c_client *client;
};

static int zforce_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct zforce_info *info;
//	iomux_v3_cfg_t *p = zforce2_suspend_enter_pads;
	int ret = 0; //, i = 0;

	DEBUG_INFO("Probing i2c\n");

//Fred todo: need to port boardid module. temporarily disable boardid/regulator check for bringup
//	if(lab126_board_rev_greater(BOARD_ID_BOURBON_WFO_EVT1) ||
//		lab126_board_is(BOARD_ID_WARIO_4_256M_CFG_C) ||
//		lab126_board_is(BOARD_ID_BOURBON_WFO_PREEVT2) ) {
		
		/* To do: maps a supply name to a device in device tree */
		zforce_data.touchreg = regulator_get(NULL, "ldo4");

		if(IS_ERR(zforce_data.touchreg)) {
			printk(KERN_ERR "Zforce2 regulator_get failed!\n");
			return -EIO;
		}
		if(!regulator_is_enabled(zforce_data.touchreg)) {
			printk(KERN_ERR "Zforce2 regulator is not enabled! \
				Enabling Zforce2 touch regulator...\n");
			ret = regulator_enable(zforce_data.touchreg);
			if(ret) {
				printk(KERN_ERR "Cannot enable Zforce2 regulator!\n");
				regulator_put(zforce_data.touchreg);
				return ret;
			}
			ret = regulator_set_voltage(zforce_data.touchreg, 3200000, 3200000);
			if (IS_ERR((void *)ret)) {
				printk(KERN_ERR "Unable to set voltage for ldo4 regulator, err = 0x%x\n", ret);
				regulator_disable(zforce_data.touchreg);
				return ret;
			}
		}
		printk(KERN_ERR "Zforce2 touch regulator is enabled!\n");
//	}
	/* initialise normal pad IOMUX addresses */
//	for (i = 0; i < ARRAY_SIZE(zforce2_suspend_enter_pads); i++) {
//		zforce2_suspend_exit_pads[i] = *p++;
//	}

	gpio_zforce_init_pins();
	mdelay(10);
	/* turn-off BSL lines */
	gpio_zforce_bslpins_ena(false);
	mdelay(10);

	/* Reset the chip */
	bslReset(ZF_STATE_RESET);

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
	{
		//Fred todo: need to port boardid module. temporarily disable boardid/regulator check for bringup
		#if 0
		if(lab126_board_rev_greater(BOARD_ID_BOURBON_WFO_EVT1) || 
			lab126_board_is(BOARD_ID_WARIO_4_256M_CFG_C) ||
			lab126_board_is(BOARD_ID_BOURBON_WFO_PREEVT2) ) {
		{	
			regulator_force_disable(zforce_data.touchreg);
			regulator_put(zforce_data.touchreg);
		}
		#endif
		return -ENOMEM;
	}
	if(deviceType==1)
		client->addr = ZFORCE_I2C_ADDRESS_TI;
	else
		client->addr = ZFORCE_I2C_ADDRESS_ST;
	i2c_set_clientdata(client, info);
	info->client = client;
	zforce_i2c_client = info->client;
	if(deviceType==1)
		zforce_i2c_client->addr = ZFORCE_I2C_ADDRESS_TI;
	else
		zforce_i2c_client->addr = ZFORCE_I2C_ADDRESS_ST;

	DEBUG_INFO("Probing i2c done\n");
	i2c_probe_success = 1;

	return 0;
}

static int zforce_i2c_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret = 0;
	DEBUG_INFO("I2C suspend\n");
	//if not locked, invoke suspend
	if(!zforce_data.exlock)
		ret = zforce_suspend_touch();
	return ret;
}

static int zforce_i2c_resume(struct i2c_client *client)
{
	DEBUG_INFO("I2C resume\n");
	//if locked, sleep in...
	if(zforce_data.exlock)
		return 0;
	
	return zforce_resume_touch();
}

static int zforce_i2c_remove(struct i2c_client *client)
{
	DEBUG_INFO("I2C Removing zforce\n");

	zforce_stop_touch(ZF_STATE_STOP);
	remove_input_device();
	gpio_zforce_free_pins();

	#if 0
	//Fred todo: need to port boardid module. temporarily disable boardid/regulator check for bringup
	if(lab126_board_rev_greater(BOARD_ID_BOURBON_WFO_EVT1) ||
		lab126_board_is(BOARD_ID_BOURBON_WFO_PREEVT2) ||
		lab126_board_is(BOARD_ID_WARIO_4_256M_CFG_C) ) {
	{
		regulator_force_disable(zforce_data.touchreg);
		regulator_put(zforce_data.touchreg);
	}
	#endif

	DEBUG_INFO("I2C Done remove zforce\n");
	return 0;
}

static const struct i2c_device_id zforce_i2c_id[] =
{
  { ZF_DRIVER_NAME, 0 },
  { },
};

static const struct dev_pm_ops zforce_pm_ops = {
	.suspend = zforce_i2c_suspend,
	.resume = zforce_i2c_resume,
};

static struct i2c_driver zforce_i2c_driver =
{
	.driver = {
		.name = ZF_DRIVER_NAME,
		.pm = &zforce_pm_ops,
	},
	.probe = zforce_i2c_probe,
	.remove = zforce_i2c_remove,
	.id_table = zforce_i2c_id,
};

static inline unsigned int zforce_read(u8* data, int len)
{
	struct i2c_msg xfer[1];
	int ret;

	xfer[0].addr = zforce_i2c_client->addr;
	xfer[0].flags = I2C_M_RD;
	xfer[0].len = len;
	xfer[0].buf = data;

	ret = i2c_transfer(zforce_i2c_client->adapter, xfer, 1);
	if (ret != 1 || ret < 0) {
		DEBUG_ERR(ZF_ERR_IO_READ);
		return 0;
	}

	return len;
}

static inline unsigned int zforce_write(u8*data, int len)
{
	struct i2c_msg xfer[1];
	int ret;

	xfer[0].addr = zforce_i2c_client->addr;
	xfer[0].flags = 0;
	xfer[0].len = len;
	xfer[0].buf = data;

	ret = i2c_transfer(zforce_i2c_client->adapter, xfer, 1);
	if (ret != 1 || ret < 0) {
		return 0;
	}

	return len;
}

int writeCommand(u8* command, u32 size)
{
	u32 i;
	int ret;

	for (i = 0; i < size; i++)
	{
		ret = i2c_smbus_write_byte(zforce_i2c_client, command[i]);
		if (ret < 0)
			return ret;
	}
	return i;
}

s32 readByte(void)
{
	s32 result;

	result = i2c_smbus_read_byte(zforce_i2c_client);
	if (result < 0)
	{
		DEBUG_ERR(ZF_ERR_I2C_READ_RETRY);
		return -EIO;
	}

	result = (u8)(result & 0xFF);
	return result;
}
/******* End I2C Setup *******/

#define TOUCH_HOLD_WARN_TIME 1000 // 10 seconds (1 tick is 10 ms)
static bool false_touch_warning = false;
static unsigned long last_touch_time = 0;
static int touch_count = 0;

static void send_user_event(u8 prev, u8 cur)
{
	struct input_dev *tdev = zforce_data.tdev;

#ifdef CONFIG_CPU_FREQ_OVERRIDE_LAB126
	if (cur > 0)
	{
		cpufreq_override(1);
	}
#endif
	// No more touches. Tell user-space.
	if (cur == 0 && prev > 0) {
		char *envp[] = {"ZForce=notouch", NULL};
		kobject_uevent_env(&tdev->dev.kobj, KOBJ_CHANGE, envp);
		DEBUG_INFO("Send user event (ZForce=notouch)\n");
		if (false_touch_warning) {
			false_touch_warning = false;
			last_touch_time = 0;
		}
		touch_count = 0;
	} else if (cur > 0 && prev == 0) {
		char *envp[] = {"ZForce=touch", NULL};
		kobject_uevent_env(&tdev->dev.kobj, KOBJ_CHANGE, envp);
		DEBUG_INFO("Send user event (ZForce=touch)\n");
		last_touch_time = jiffies;
	}
}

#define METRIC_BUFFER_SIZE  100
static void sendTouchUpdate(TouchData *tData)
{
	char buff[METRIC_BUFFER_SIZE];
	struct input_dev *tdev = zforce_data.tdev;
	u8 contacts_left = 0;
	u8 i;

	for(i = 0; i < MAX_CONTACTS; i++)
	{
		CoordinateData *coord = &tData->coordinateData[i];
		int tState = coord->touch_state;

		// An update is pending for this touch. Report slot number
		input_report_abs(tdev, ABS_MT_SLOT, coord->id - 1);

		if (tState == TOUCH_DOWN || tState == TOUCH_MOVE || tState ==
			TOUCH_GHOST) {
			// Specify ID of touch the update applies to
			input_report_abs(tdev, ABS_MT_TRACKING_ID, coord->id - 1);
			input_report_abs(tdev, ABS_MT_POSITION_X, coord->X);
			input_report_abs(tdev, ABS_MT_POSITION_Y, coord->Y);
			DEBUG_INFO("Coord X=%d Y=%d, Track ID=%d\n", coord->X, coord->Y, coord->id - 1);
			contacts_left++;
			touch_count++;

			// For tracing false touch events
			if (i == 1 && last_touch_time > 0 && (jiffies - last_touch_time > TOUCH_HOLD_WARN_TIME) && !false_touch_warning) {
				DEBUG_ERR("possible touch looping error!!!\n");
				false_touch_warning = true;

				snprintf(buff, sizeof(buff)-1, "%d", touch_count);
				LLOG_DEVICE_METRIC(DEVICE_METRIC_HIGH_PRIORITY, DEVICE_METRIC_TYPE_COUNTER, "kernel", "zforce2", "count", 1, buff);

			}
		} else if (tState == TOUCH_UP || tState == TOUCH_INVALID) {
			// Report touch up
			input_report_abs(tdev, ABS_MT_TRACKING_ID, -1);
			DEBUG_INFO("Track ID=-1\n");
		}
	}

	// Notify user space that there is new activity
	send_user_event(tData->count, contacts_left);

	tData->count = contacts_left;

	input_report_key(tdev, BTN_TOUCH, contacts_left > 0);
	input_report_key(tdev, BTN_TOOL_FINGER, contacts_left >= 1);
	input_report_key(tdev, BTN_TOOL_DOUBLETAP, contacts_left >= 2);
	input_report_key(tdev, BTN_TOOL_TRIPLETAP, contacts_left == 3);

	input_sync(tdev);
}

u32 readTouchData(unsigned char* data, TouchData *tData)
{
	u32 totalRead  = 0;
	u8  numTouches = 0;
	u8  i = 0;

	DEBUG_INFO("====== New Touch Data ======\n");
	numTouches = data[totalRead++];

	DEBUG_INFO("Num touches: %d\n", numTouches);
	for (i = 0; i < numTouches; i++)
	{
		CoordinateData coord;

		memcpy(&coord.X, &data[totalRead  ], sizeof(u16));
		memcpy(&coord.Y, &data[totalRead+2], sizeof(u16));
		coord.id          = ((data[totalRead+4] & 0xF0) >> 4);
		coord.touch_state = data[totalRead+4] & 0x0F;
		coord.width       = data[totalRead+5];
		coord.height      = data[totalRead+6];
		coord.pressure    = data[totalRead+7];
		coord.probability =  data[totalRead+8];

		tData->coordinateData[coord.id - 1] = coord;
		totalRead += sizeof(CoordinateData);
		DEBUG_INFO("X:%d __ Y:%d __ State:%d __ ID:%d\n",
			coord.X, coord.Y, coord.touch_state, coord.id);
	}
	DEBUG_INFO("====== End Touch Data =====\n");
	return totalRead;
}

u32 readLowSignalAlert(unsigned char* data, LowSignalAlert* lsalert)
{
	u32 totalRead = 0;
	u8  count     = 0;

	lsalert->xCount = data[totalRead++];
	if (lsalert->xCount > MAX_X_LED_COUNT) {
		DEBUG_ERR(ZF_ERR_X_LED);
		return totalRead;
	}
	lsalert->yCount = data[totalRead++];
	if (lsalert->yCount > MAX_Y_LED_COUNT) {
		DEBUG_ERR(ZF_ERR_Y_LED);
		return totalRead;
	}
	for (count = 0; count < lsalert->xCount; count++) {
		lsalert->xLEDS[count].PDSignal1Low = (data[totalRead]   & 0xF0) >> 4;
		lsalert->xLEDS[count].PDSignal2Low = (data[totalRead++] & 0x0F);
	}
	for (count = 0; count < lsalert->yCount; count++) {
		lsalert->yLEDS[count].PDSignal1Low = (data[totalRead]   & 0xF0) >> 4;
		lsalert->yLEDS[count].PDSignal2Low = (data[totalRead++] & 0x0F);
	}
	return totalRead;
}

u32 readLedLevelResponse(unsigned char* data, LedLevelResponse* lvl)
{
	u32 totalRead = 0, index = 0;
	u8  count     = 0;

	if( deviceType == 1) {
		u8 axis = data[totalRead++];
		u8 numOfAxis = data[totalRead++];
		if( axis == AXIS_X )
		{
			lvl->xCount = (numOfAxis + 1)/2;
			for (count = 0; count < lvl->xCount; count++)
			{
				lvl->xLedLevel.xLEDS[count].LedStrength1 = (data[totalRead]   & 0xF0) >> 4;
				lvl->xLedLevel.xLEDS[count].LedStrength2 = (data[totalRead++] & 0x0F);
				lvl->xLedLevel.xLEDS[count].PDSignal1    = (data[totalRead++]       );
				lvl->xLedLevel.xLEDS[count].PDSignal2    = (data[totalRead++]       );
			}
		} else if( axis == AXIS_Y )
		{
			lvl->yCount = (numOfAxis + 1)/2;
			for (count = 0; count < lvl->yCount; count++)
			{
				lvl->yLedLevel.yLEDS[count].LedStrength1 = (data[totalRead]   & 0xF0) >> 4;
				lvl->yLedLevel.yLEDS[count].LedStrength2 = (data[totalRead++] & 0x0F);
				lvl->yLedLevel.yLEDS[count].PDSignal1    = (data[totalRead++]       );
				lvl->yLedLevel.yLEDS[count].PDSignal2    = (data[totalRead++]       );
			}
		}
	}else if( deviceType == 2) {
		index = ZFORCE_ASN_MSG_DATA_IDX + 1;
		lvl->xCount = (data[index++] + 1) / 2;
		if( lvl->xCount >= MAX_X_LED_COUNT) {
			DEBUG_ERR("%s: Wrong X-axis size %d\n", __func__, lvl->xCount);
		}
		else {
			for( count = 0; count < lvl->xCount; count++) 
			{
				lvl->xLedLevel.xAsnLEDS[count].LedLevel1 = data[index++];
				lvl->xLedLevel.xAsnLEDS[count].LedLevel2 = data[index++];
			}
			index++;
			lvl->yCount = (data[index++] +1) / 2;
			if( lvl->yCount >= MAX_Y_LED_COUNT) {
				DEBUG_ERR("%s: Wrong Y-axis size %d\n", __func__, lvl->yCount);
			}
			else {
				for( count = 0; count < lvl->yCount; count++) 
				{
					lvl->yLedLevel.yAsnLEDS[count].LedLevel1 = data[index++];
					lvl->yLedLevel.yAsnLEDS[count].LedLevel2 = data[index++];
				}
				totalRead = index - ZFORCE_ASN_MSG_DATA_IDX;
			}
		}
	} else {
		DEBUG_ERR("%s: unsupported deviceType %d\n", __func__,deviceType);
	}
	
	return totalRead;
}

u32 readRawDiodeDataResponse(unsigned char *data, RawDiodeData* rawData)
{
	u32 totalRead = 0;
	u8  count     = 0;

	rawData->NumberOfPulses = data[totalRead++];
	rawData->xCount = data[totalRead++];
	rawData->yCount = data[totalRead++];
	for (count = 0; count < rawData->xCount*2; count++)
		rawData->xValues[count] = data[totalRead++];
	for (count = 0; count < rawData->yCount*2; count++)
		rawData->yValues[count] = data[totalRead++];
	return totalRead;
}

u32 readFixedPulseResponse(unsigned char *data, FixedPulseStrengthResponse* rawData)
{
	u32 totalRead = 0;
	u8  count     = 0;

	rawData->xCount = data[totalRead++];
	rawData->yCount = data[totalRead++];
	for (count = 0; count < rawData->xCount; count++)
		rawData->xValues[count] = data[totalRead++];
	for (count = 0; count < rawData->yCount; count++)
		rawData->yValues[count] = data[totalRead++];
	return totalRead;
}

u32 readStatusReqResponse(unsigned char *data, StatusRequestResp *statusData)
{
	u32 totalRead = 0;
	u32 reservedBytesLen = 81;

	memcpy(&statusData->version, &data[totalRead], sizeof(FwVersionData));
	totalRead += sizeof(FwVersionData);

	statusData->contact_scanactv = data[totalRead++];

	statusData->scanning_counter = data[totalRead];
	totalRead += sizeof(u16);

	statusData->prep_touch_pkgs = data[totalRead];
	totalRead += sizeof(u16);

	statusData->sent_touch_pkgs = data[totalRead];
        totalRead += sizeof(u16);

	statusData->invld_touch_cntr = data[totalRead];
        totalRead += sizeof(u16);

	memcpy(&statusData->config, &data[totalRead], sizeof(ConfigurationData));
	totalRead += sizeof(ConfigurationData);

	memcpy(&statusData->freqData, &data[totalRead], sizeof(ScanningFrequencyData));
	totalRead += sizeof(ScanningFrequencyData);

	memcpy(&statusData->resData, &data[totalRead], sizeof(ResolutionData));
	totalRead += sizeof(ResolutionData);

	statusData->phys_width = data[totalRead];
        totalRead += sizeof(u16);

	statusData->phys_height = data[totalRead];
        totalRead += sizeof(u16);

	memcpy(&statusData->activeLeds, &data[totalRead], sizeof(ActiveLeds));
	totalRead += sizeof(ActiveLeds);

	statusData->max_ss = data[totalRead++];
	statusData->maxsz_ena = data[totalRead++];
	statusData->maxsz = data[totalRead++];
	statusData->minsz_ena = data[totalRead++];
	statusData->minsz = data[totalRead++];
	statusData->devtype = data[totalRead++];
	statusData->inf_prtcl_major = data[totalRead++];
        statusData->inf_prtcl_minor = data[totalRead++];

	return (totalRead += reservedBytesLen);
}

void clearTouch(void)
{
	TouchData *tData = &zforce_data.tData;
	int i;

	mutex_lock(&contactframe_mutex);
	for (i = 0; i < MAX_CONTACTS; i++)
	{
		tData->coordinateData[i].id = i+1;
		tData->coordinateData[i].touch_state = TOUCH_UP;
	}
	sendTouchUpdate(tData);
	mutex_unlock(&contactframe_mutex);
}

void zforce_free_touch_irq(void)
{
	if (zforce_data.irq)
	{
		free_irq(zforce_data.irq, NULL);
		zforce_data.irq = 0;
	}
	gpio_touchcntrl_request_irq(0);
}

int zforce_get_touch_irq(void)
{
	int ret;
	unsigned long irq_flags;

	/* Set the IRQ */
	gpio_touchcntrl_request_irq(1); // Enable IRQ

	zforce_data.irq = gpio_touchcntrl_irq();
	/* use edge triggered interrupts */
	irq_flags = IRQF_TRIGGER_FALLING | IRQF_ONESHOT;

	ret = request_threaded_irq(zforce_data.irq, zforce_quick_check_irqhandler,
				zforce_irq_thread, irq_flags, "ZForce2", NULL);
	if (ret < 0) {
		DEBUG_ERR("%s: Error, could not request irq\n", __func__);
	}

	return ret;
}

/* Process device response */
void processData(u8 request, ZFStatus *result)
{
	u8       *data      = NULL;                 // Data buffer
	u8        type      = INVALID_COMMAND_RES;  // Data type 
	int       frameSize = 0;                    // Size of read frame
	int       dataRead  = 0;                    // Bytes processed in frame
	u8        frameHeader[ZFORCE_HEADER_SIZE];  // Frame header buffer

	DEBUG_INFO("In ProcessData\n");
	*result  = ZF_NO_RESPONSE;

	if (atomic_read(&zforce_data.state) != ZF_STATE_RUN)
	{
		*result = ZF_ERROR;
		return;
	}

	if(!zforce_read(frameHeader, ZFORCE_HEADER_SIZE))
	{
		DEBUG_ERR(ZF_ERR_FRAME_HEADER);
		*result = ZF_ERR_FORCE_RST;
		return;
	}

	DEBUG_INFO("process:Frame Header Received:\n");
	print_data_block(frameHeader, 2);

	if (frameHeader[ZFORCE_FRAME_START_IDX] != FRAME_START)
	{
		int i=0;
		DEBUG_ERR(ZF_ERR_START_FRAME, frameHeader[ZFORCE_FRAME_START_IDX], frameHeader[ZFORCE_FRAME_SIZE_IDX]);
		DEBUG_ERR(ZF_ERR_IO_READ);
		if(!ignore_false_header)
		{
			*result = ZF_ERR_FORCE_RST;
		}
		else{
			DEBUG_INFO("process:receive extra data to find more info\n");
			for(i=0;i<5;i++){
				zforce_read(frameHeader, ZFORCE_HEADER_SIZE);
				print_data_block(frameHeader, 2);
				zforce_read(frameHeader, ZFORCE_HEADER_SIZE);
				print_data_block(frameHeader, 2);
			}
		}
		return;
	}

	frameSize = (int)frameHeader[ZFORCE_FRAME_SIZE_IDX];
	DEBUG_INFO("Frame size : %d\n", frameSize);

	data = kzalloc(frameSize * sizeof(u8), GFP_KERNEL);
	if (!data)
	{
		DEBUG_ERR(ZF_ERR_ALLOC);
		*result = ZF_ERROR;
		return;
	}

	/* Read the frame data */
	if (!zforce_read(data, frameSize))
	{
		DEBUG_ERR(ZF_ERR_READ_DATA);
		kfree(data);
		*result = ZF_ERR_FORCE_RST;
		return;
	}

	DEBUG_INFO("process:Frame Data Received:\n");
	print_data_block(data, frameSize);

	while(dataRead < frameSize)
	{
		type = data[dataRead++];
		DEBUG_INFO("process:Request: 0x%02x __ Type received: 0x%02x\n", request, type);
		if (request != type) { 
			printk(KERN_ERR "esd reset triggered\n");
			kfree(data);
			*result = ZF_ERR_FORCE_RST;
			return;
		}

		switch(type) {
		case(TYPE_TOUCH_DATA_RES):
		{
	        	TouchData *tData = &zforce_data.tData;
			mutex_lock(&contactframe_mutex);
			dataRead += readTouchData(&data[dataRead], tData);
			if (request == TYPE_TOUCH_DATA_RES)
				*result = ZF_OK;
			sendTouchUpdate(tData);
			mutex_unlock(&contactframe_mutex);
	        	break;
		}
		case(TYPE_BOOT_COMPLETE_RES):
		{
			BootCompleteResult res;
			DEBUG_INFO("Got BootComplete response\n");
			memcpy(&res, &data[dataRead], sizeof(BootCompleteResult));
			dataRead += sizeof(BootCompleteResult);
			if (request == TYPE_BOOT_COMPLETE_RES)
				*result = ((res.comm_status & 0x1) == SUCCESS_RES) ? ZF_OK : ZF_ERROR;
			break;
		}
		case(TYPE_DEACTIVATE_RES):
		{
			DeactivateResult res;
			DEBUG_INFO("Got DeActivate response\n");
			memcpy(&res, &data[dataRead], sizeof(DeactivateResult));
			dataRead += sizeof(DeactivateResult);
			if (request == TYPE_DEACTIVATE_RES)
				*result = (res.result == SUCCESS_RES) ? ZF_OK : ZF_ERROR;
			break;
		}
		case(TYPE_ACTIVATE_RES):
		{
			ActivateResult res;
			DEBUG_INFO("Got Activate response\n");
			memcpy(&res, &data[dataRead], sizeof(ActivateResult));
			dataRead += sizeof(ActivateResult);
			if (type == TYPE_ACTIVATE_RES)
				*result = (res.result == SUCCESS_RES) ? ZF_OK : ZF_ERROR;
			break;
		}
		case(TYPE_SET_RESOLUTION_RES):
		{
			SetResolutionResult res;
			DEBUG_INFO("Got SetResolution response\n");
			memcpy(&res, &data[dataRead], sizeof(SetResolutionResult));
			dataRead += sizeof(SetResolutionResult);
			if (request == TYPE_SET_RESOLUTION_RES)
				*result = (res.result == SUCCESS_RES) ? ZF_OK : ZF_ERROR;
			break;
	      	}
		case(TYPE_SET_CONFIGURATION_RES):
		{
			SetConfigurationResult res;
			DEBUG_INFO("Got SetConfiguration response\n");
			memcpy(&res, &data[dataRead], sizeof(SetConfigurationResult));
			dataRead += sizeof(SetConfigurationResult);
			if (request == TYPE_SET_CONFIGURATION_RES)
				*result = (res.result == SUCCESS_RES) ? ZF_OK : ZF_ERROR;
			break;
		}
		case(TYPE_SCANNING_FREQ_RES):
		{
			SetScanningFrequencyResult res;
			DEBUG_INFO("Got ScanningFreq response\n");
			memcpy(&res, &data[dataRead], sizeof(SetScanningFrequencyResult));
			dataRead += sizeof(SetScanningFrequencyResult);
			if (request == TYPE_SCANNING_FREQ_RES)
				*result = (res.result == SUCCESS_RES) ? ZF_OK : ZF_ERROR;
			break;
		}
		case(TYPE_LED_LEVEL_RES):
		{
			LedLevelResponse *ledLvl = &zforce_data.ledLvl;
			DEBUG_INFO("Got LedLevel response\n");
			mutex_lock(&ledlvl_mutex);
			if(data[1] == AXIS_X )
				zforce_data.ledlvlx_ready = 1;
			else if( data[1] == AXIS_Y )
				zforce_data.ledlvly_ready = 1;
			dataRead += readLedLevelResponse(&data[dataRead], ledLvl);
			if (request == TYPE_LED_LEVEL_RES)
				*result = ZF_OK;
			wake_up(&zf_user_wq);
			mutex_unlock(&ledlvl_mutex);
			break;
		}
		case(TYPE_LOW_SIGNAL_ALERT):
		{
			/* LowSignalAlert *lowSig = &zforce_data.lowSig; */
			char *envp[] = {"ZForce2=low_signal", NULL};
			DEBUG_INFO("Reading Low Signal Alert\n");

			/* TODO WARIO-430 - Requires additional req-resp to get more data
			 * dataRead += readLowSignalAlert(&data[dataRead], lowSig); */
			kobject_uevent_env(&zforce_data.tdev->dev.kobj, KOBJ_CHANGE, envp);
			if (request == TYPE_LOW_SIGNAL_ALERT)
				*result = ZF_OK;
			break;
		}
		case(TYPE_REL_FRAME_RESP_NUM_RES):
		{
			SetRelFrameRespNumResult res;
			DEBUG_INFO("Got reliability set frame number response\n");
			memcpy(&res, &data[dataRead], sizeof(SetRelFrameRespNumResult));
			dataRead += sizeof(SetRelFrameRespNumResult);
			if (request == TYPE_REL_FRAME_RESP_NUM_RES)
				*result = (res.result == SUCCESS_RES) ? ZF_OK : ZF_ERROR;
			break;
		}
		case(TYPE_FIXED_PULSE_STR_RES):
		{
			FixedPulseStrengthResponse *res = &zforce_data.rawDiode;
			DEBUG_INFO("Got Fixed pulse strength result\n");
			mutex_lock(&ledlvl_mutex);
			dataRead += readFixedPulseResponse(&data[dataRead], res);
			zforce_data.raw_diode_ready = 1;
			mutex_unlock(&ledlvl_mutex);
			*result = ZF_OK;
			break;
		}
		case(TYPE_STATUS_MCU_REQ_RES):
		{
			StatusRequestResp *res = &zforce_data.statusReqResp;
			dataRead += readStatusReqResponse(&data[dataRead], res);
			atomic_set(&zforce_data.mcusetting_ready, 1);
			*result = ZF_OK;
			break;
		}
		case(TYPE_RAW_DIODE_DATA_RES): /* for reltest */
		{
			u8 axis = 0, NoOfSignals = 0, SigBytesRcvd = (frameSize-3);
			*result = ZF_OK;
			/* check for some data sanity in the I2c packet */
			axis = data[dataRead++];
			if((axis != AXIS_X) && (axis != AXIS_Y)) {
				printk(KERN_ERR "Corrupt I2c packet: Unknown axis read:%d\n", axis);
				*result = ZF_IO_ERROR;
			}
			NoOfSignals = data[dataRead++];
			if(NoOfSignals != SigBytesRcvd) {
				printk(KERN_ERR "Incomplete I2c packet: Mismatch! \
				No Of Signals:%d Signal Bytes Recieved:%d\n",
				NoOfSignals, SigBytesRcvd);
				*result = ZF_IO_ERROR;
			}
			/* increment anyways */
			dataRead += SigBytesRcvd;
			reltest_run++;
			if(reltest_run >= RELTEST_RUNCNT) {
				printk(KERN_ERR "Stress I2C test: completed %d runs\n",
				RELTEST_RUNCNT);
				reltest_run = 0;
			}

			break;
		}
		case(TYPE_OVERRUN_NOTIF):
		{
			char *envp[] = {"ZForce2=Overrun", NULL};
			DEBUG_INFO("Overrun detected!\n");
			kobject_uevent_env(&zforce_data.tdev->dev.kobj, KOBJ_CHANGE, envp);
			*result = ZF_OK;
			break;
		}
	      	case(INVALID_COMMAND_RES):
	      	{
			DEBUG_INFO("Invalid command response on command: 0x%x\n", data[dataRead]);
	      		break;
		}
		default:
		{
			DEBUG_ERR(ZF_ERR_UNKNOWN_TYPE, type);
			dataRead = frameSize;
			break;
		}
		} /* end of switch */
	}
	DEBUG_INFO("In process Data. Result is : %d\n", *result);
	kfree(data);

	return;
}

ZFStatus sendActivate(void)
{
	ZFStatus result = ZF_OK;
	u8 command[10];

	DEBUG_INFO("Sending Activate");
	command[0] = FRAME_START;
	command[1] = 1;
	command[2] = ACTIVATE_CMD;

	print_data_block(command, 3);
	if (!zforce_write(command, 3))
	{
		DEBUG_ERR(ZF_ERR_IO_WRITE);
		return ZF_ERROR;
	}

	if (zforce_touch_poll())
		return ZF_ERROR;

	mutex_lock(&comm_mutex);
	processData(ACTIVATE_CMD, &result);
	mutex_unlock(&comm_mutex);
	if (result != ZF_OK) {
		DEBUG_INFO("Activate Failed!\n");
		return result;
	}
	DEBUG_INFO("Activate SUCCESS\n");
	return result;
}

ZFStatus sendDeactivate(void)
{
	ZFStatus result = ZF_OK;
	u8 command[3];

	DEBUG_INFO("Sending deactivate\n");
	command[0] = FRAME_START;
	command[1] = 1;
	command[2] = DEACTIVATE_CMD;

	print_data_block(command, 3);
	if (!zforce_write(command, 3))
	{
		if (atomic_read(&zforce_data.state) == ZF_STATE_RUN)
			DEBUG_ERR(ZF_ERR_IO_WRITE);
		return ZF_ERROR;
	}

	if (zforce_touch_poll())
		return ZF_ERROR;

	mutex_lock(&comm_mutex);
	processData(DEACTIVATE_CMD, &result);
	mutex_unlock(&comm_mutex);
	if (result != ZF_OK) {
		// Deactivate failed. Reset the chip
		bslReset(ZF_STATE_RESET);
		return result;
	}

	DEBUG_INFO("Deactivate SUCCESS\n");
	return result;
}

ZFStatus sendAsnSetResolution(u16 X, u16 Y)
{
	ZFStatus result = ZF_OK;
	u8 command[ASN_RESOLUTION_LEN]={0xEE,0x10,0xEE,0x0E,0x40,0x02,0x01,0x00,0x69,0x08,0x80,0x02,0x03,0xE4,0x81,0x02,0x04,0xC8};

	command[12] = X >> 8;
	command[13] = X & 0xFF;
	command[16] = Y >> 8;
	command[17] = Y & 0xFF;

	print_data_block(command, ASN_RESOLUTION_LEN);
	if (!zforce_write(command, ASN_RESOLUTION_LEN))
	{
		DEBUG_ERR(ZF_ERR_SET_RESOLUTION);
		return ZF_ERROR;
	}

	/* delay before fetch response from touch fw */
	if (zforce_touch_poll())
		return ZF_ERROR;
	
	processAsnData(ZF_ASN_Resolution, &result);
	if (result != ZF_OK) {
		DEBUG_INFO("SetResolution Failed\n");
		return result;
	}

	DEBUG_INFO("SetResolution SUCCESS\n");
	return result;
}

ZFStatus sendSetResolution(u16 X, u16 Y)
{
	ZFStatus result = ZF_OK;
	u8 command[7];

	DEBUG_INFO("Setting Resolution\n");
	command[0] = FRAME_START;
	command[1] = 5;
	command[2] = SET_RESOLUTION_CMD;
	memcpy(&command[3], &X, sizeof(X));
	memcpy(&command[5], &Y, sizeof(Y));

	print_data_block(command, 7);
	if (!zforce_write(command, 7))
	{
		DEBUG_ERR(ZF_ERR_SET_RESOLUTION);
		return ZF_ERROR;
	}

	if (zforce_touch_poll())
		return ZF_ERROR;

	mutex_lock(&comm_mutex);
	processData(SET_RESOLUTION_CMD, &result);
	mutex_unlock(&comm_mutex);
	if (result != ZF_OK) {
		DEBUG_INFO("SetResolution Failed\n");
		return result;
	}
	DEBUG_INFO("SetResolution SUCCESS\n");
	return result;
}

ZFStatus sendMcuSettingsRequest(void)
{
	ZFStatus result = ZF_OK;
	u8 command[3];

	DEBUG_INFO("Requesting MCU settings\n");
	command[0] = FRAME_START;
	command[1] = 1;
	command[2] = STATUS_MCU_REQ_CMD;

	if (!zforce_write(command, 3))
	{
		DEBUG_ERR(ZF_ERR_IO_WRITE);
		return ZF_ERROR;
	}

	if (zforce_touch_poll())
		return ZF_ERROR;

	mutex_lock(&comm_mutex);
	processData(STATUS_MCU_REQ_CMD, &result);
	mutex_unlock(&comm_mutex);
	if (result != ZF_OK) {
		return result;
	}
	DEBUG_INFO("Request MCU Settings SUCCESS\n");
	return result;
}

ZFStatus sendSetConfiguration(u32 flag)
{
	ZFStatus result = ZF_OK;
	ConfigurationData config;
	u8 command[7];

	DEBUG_INFO("Requesting SetConfiguration\n");
	config.flags = flag;
	command[0] = FRAME_START;
	command[1] = 0x5;
	command[2] = SET_CONFIGURATION_CMD;
	command[3] = (u8)flag;
	command[4] = 0x00;
	command[5] = 0x00;
	command[6] = 0x00;

	print_data_block(command, 7);
	if(!zforce_write(command, 7))
	{
		DEBUG_ERR(ZF_ERR_SET_CONFIGURATION);
		return ZF_ERROR;
	}

	if (zforce_touch_poll())
		return ZF_ERROR;

	mutex_lock(&comm_mutex);
	processData(SET_CONFIGURATION_CMD, &result);
	mutex_unlock(&comm_mutex);
	if (result != ZF_OK) {
		return result;
	}

	DEBUG_INFO("Request SetConfiguration SUCCESS\n");
	return result;
}

ZFStatus sendSetScanningFrequency(u16 idle, u16 finger, u16 stylus)
{
	ZFStatus result = ZF_OK;
	u8 command[9];

	command[0] = FRAME_START;
	command[1] = 7;
	command[2] = SCANNING_FREQ_CMD;
	memcpy(&command[3], &idle, sizeof(u16));
	memcpy(&command[5], &finger, sizeof(u16));
	memcpy(&command[7], &stylus, sizeof(u16));

	DEBUG_INFO("Setting Scanning Frequency\n");

	print_data_block(command, 9);
	if (!zforce_write(command, 9))
	{
		DEBUG_ERR(ZF_ERR_SET_SCAN_FREQ);
		return ZF_ERROR;
	}

	if (zforce_touch_poll())
		return ZF_ERROR;

	mutex_unlock(&comm_mutex);
	processData(SCANNING_FREQ_CMD, &result);
	mutex_unlock(&comm_mutex);
	if (result != ZF_OK) {
		return result;
	}
	DEBUG_INFO("Set Scanning Frequency SUCCESS\n");
	return result;
}

ZFStatus sendForceLedLevelRequestX(void)
{
	ZFStatus result = ZF_OK;
	u8 command[4];

	DEBUG_INFO("Sending ForceLedLevelRequest X\n");

	command[0] = FRAME_START;
	command[1] = 2;
	command[2] = LED_LEVEL_CMD;
	command[3] = AXIS_X;
	if (!zforce_write(command, 4))
	{
		DEBUG_ERR(ZF_ERR_IO_WRITE);
		return ZF_ERROR;
	}

	if (zforce_touch_poll())
		return ZF_ERROR;

	mutex_lock(&comm_mutex);
	processData(LED_LEVEL_CMD, &result);
	mutex_unlock(&comm_mutex);

	if (result != ZF_OK) {
		return result;
	}
	DEBUG_INFO("ForceLedLevelRequest X SUCCESS\n");
	return ZF_OK;
}

ZFStatus sendForceLedLevelRequestY(void)
{
	ZFStatus result = ZF_OK;
	u8 command[4];

	DEBUG_INFO("Sending ForceLedLevelRequest Y\n");

	command[0] = FRAME_START;
	command[1] = 2;
	command[2] = LED_LEVEL_CMD;
	command[3] = AXIS_Y;
	if (!zforce_write(command, 4))
	{
		DEBUG_ERR(ZF_ERR_IO_WRITE);
		return ZF_ERROR;
	}
	if (zforce_touch_poll())
		return ZF_ERROR;

	mutex_lock(&comm_mutex);
	processData(LED_LEVEL_CMD, &result);
	mutex_unlock(&comm_mutex);

	if (result != ZF_OK) {
		return result;
	}
	DEBUG_INFO("ForceLedLevelRequest Y SUCCESS\n");
	return ZF_OK;
}



ZFStatus sendTouchDataRequest(void)
{
	u8 command[3];

	DEBUG_INFO("Sending initial Touch data request\n");
	command[0] = FRAME_START;
	command[1] = 1;
	command[2] = TOUCH_DATA_CMD;

	print_data_block(command, 3);
	if (!zforce_write(command, 3))
	{
		DEBUG_ERR(ZF_ERR_IO_WRITE);
		return ZF_IO_ERROR;
	}
	return ZF_OK;
}

ZFStatus sendSetFrameResponseNumber(u16 time)
{
	ZFStatus result;
	u8 command[3];

	DEBUG_INFO("Setting Frame Response Number\n");
	command[0] = SET_REL_FRAME_RESP_NUM_CMD;
	memcpy(&command[1], &time, sizeof(time));

	DEBUG_INFO("Set Frame Response Number");
	if (!zforce_write(command, 3))
	{
		DEBUG_ERR(ZF_ERR_SET_FRAME_RESP);
		return ZF_ERROR;
	}

	if (zforce_touch_poll())
		return ZF_ERROR;

	mutex_lock(&comm_mutex);
	processData(SET_REL_FRAME_RESP_NUM_CMD, &result);
	mutex_unlock(&comm_mutex);
	if (result != ZF_OK) {
		return ZF_ERROR;
	}
	DEBUG_INFO("SetFrameResponseNumber SUCCESS\n");
	return ZF_OK;
}

ZFStatus sendPulseSignalInfo(char time, char strength)
{
	PulseSignalInfo sig;
	u8 command[2];

	command[0] = FIXED_PULSE_STR_CMD;

	sig.strength = strength;
	sig.time     = time;
	sig.reserved = 0;

	DEBUG_INFO("SIG : Strength : %d __ Time : %d\n", sig.strength, sig.time);
	memcpy(&command[1], &sig, sizeof(PulseSignalInfo));

	if (!zforce_write(command, 2))
	{
		DEBUG_ERR(ZF_ERR_SET_PULSE_SIG_INFO);
		return ZF_ERROR;
	}

	return ZF_OK;
}

ZFStatus sendForceCalibration(void)
{
	u8 command[1];

	DEBUG_INFO("Sending Force Calibration\n");
	command[0] = FORCE_CALIBRATION_CMD;

	if (writeCommand(command, 1) < 0)
	{
		DEBUG_ERR(ZF_ERR_IO_WRITE);
		return ZF_IO_ERROR;
	}

	zforce_data.force_calib_sent = 1;
	wake_up(&zf_user_wq);

	DEBUG_INFO("Force Calibration SUCCESS\n");
	return ZF_OK;
}

ZFStatus getBootComplete(void)
{
	ZFStatus result = ZF_OK;
if(!bpbc)
{  
	if (zforce_touch_poll())
		return ZF_ERROR;

	mutex_lock(&comm_mutex);
	if(deviceType==1)
		processData(TYPE_BOOT_COMPLETE_RES, &result);
	else if(deviceType==2)
		processAsnData(ZF_ASN_BootComplete, &result);
	else
	{
		DEBUG_ERR("Unknown DeviceType:%d\n",deviceType);
	}	
	mutex_unlock(&comm_mutex);
	if(result != ZF_OK) {
		return result;
	}
}
	DEBUG_INFO("Bringup -- force bypass BootComplete SUCCESS\n");
	return ZF_OK;
}

ZFStatus zforce_run_reltests(void)
{
	ZFStatus status = ZF_OK;

	if(!reltest)
		return status; /* return quietly */

	switch(reltest)	{
	case RELTEST_RESPONSE_TIME:
	{
		DEBUG_INFO("Set Frame Response Number");
		status = sendSetFrameResponseNumber(REL_RESPONSE_TIME);
		if (status != ZF_OK)
			return status;

		DEBUG_INFO("Set Signal Strength and Time");
		sendPulseSignalInfo(zforce_data.relTime, zforce_data.relStrength);
		break;
	}
	case RELTEST_STRESS_I2C:
	{
		DEBUG_INFO("Set Raw signal configuration");
		if((status = sendSetConfiguration(ZFORCE_RAW_SIGNAL)) != ZF_OK) {
			return status;
		break;
		}
	}
	}
	return status;
}

void parseAsnTouchData(u8 *data, TouchData *tData)
{
	u8 touch_id=0,touch_state=0,touch_sizeX=0,touch_pressure=0;
	u16 touch_LocX=0,touch_LocY=0;
	u32 touch_ts=0;
	u8 touchTAG=0x42, touchTAGIDX=0,totalDataLen=data[1]+2;
	CoordinateData coord;
	
	DEBUG_INFO("In parseAsnTouchData");
	for(touchTAGIDX=0;data[touchTAGIDX]!=touchTAG && touchTAGIDX<totalDataLen;touchTAGIDX++);
	
	if(touchTAGIDX == totalDataLen)
	{
		DEBUG_ERR("ERROR: did not found Touch Data\n");
		return;
	}
	DEBUG_INFO("touchTAGIDX:%d, totalDataLen:%d\n",touchTAGIDX, totalDataLen);
	
	if(touchTAGIDX == totalDataLen- 14 -1)
	{
		touch_id=data[touchTAGIDX+2];
		/* to match the TI mode coord id parsing */
		coord.id = touch_id + 1;
		touch_state=data[touchTAGIDX+3];
		coord.touch_state = touch_state;
		touch_LocX=data[touchTAGIDX+4]<<8 | (data[touchTAGIDX+5]);
		coord.X = touch_LocX;
		touch_LocY=data[touchTAGIDX+6]<<8| (data[touchTAGIDX+7]);
		coord.Y = touch_LocY;
		touch_sizeX=data[touchTAGIDX+9];
		touch_pressure=data[touchTAGIDX+10];
		coord.pressure = touch_pressure;
		touch_ts=data[touchTAGIDX+12]<<16 | data[touchTAGIDX+13]<<8 | data[touchTAGIDX+14];
		DEBUG_INFO("_X_:0x%x, _Y_:0x%x, id:0x%x, state:0x%x, sizeX=0x%x, pressure:0x%x, ts:0x%x\n",
		touch_LocX, touch_LocY,touch_id,touch_state,touch_sizeX,touch_pressure,touch_ts);
		tData->coordinateData[coord.id - 1] = coord;
	}
}

void processAsnData(ZFASNMsgType request, ZFStatus *result)
{
        u8       *data      = NULL;                 // Data buffer
        u8        type      = INVALID_COMMAND_RES;  // Data type 
        int       frameSize = 0;                    // Size of read frame
        int       dataRead  = 0;                    // Bytes processed in frame
        u8        frameHeader[ZFORCE_HEADER_SIZE];  // Frame header buffer
        int i = 0;

        DEBUG_INFO("In ProcessData\n");
        *result  = ZF_NO_RESPONSE;

        if (atomic_read(&zforce_data.state) != ZF_STATE_RUN)
        {
                *result = ZF_ERROR;
                return;
        }

        if(!zforce_read(frameHeader, ZFORCE_HEADER_SIZE))
        {
                DEBUG_ERR(ZF_ERR_FRAME_HEADER);
                *result = ZF_ERR_FORCE_RST;
                return;
        }

        DEBUG_INFO("process:Frame Header Received:\n");
        print_data_block(frameHeader, 2);

        if (frameHeader[ZFORCE_FRAME_START_IDX] != FRAME_START)
        {
        	if (frameHeader[ZFORCE_FRAME_START_IDX+1] != FRAME_START) {
                DEBUG_ERR(ZF_ERR_START_FRAME, frameHeader[ZFORCE_FRAME_START_IDX], frameHeader[ZFORCE_FRAME_SIZE_IDX]);
                DEBUG_ERR(ZF_ERR_IO_READ);
                if(!ignore_false_header)
                {
                        *result = ZF_ERR_FORCE_RST;
                } else {
                	DEBUG_ERR("retry reading header:\n");
					do {
						zforce_read(frameHeader, 1);
						print_data_block(frameHeader, 1);
					} while((frameHeader[ZFORCE_FRAME_START_IDX] != FRAME_START) && (i++ < 50));
					if(frameHeader[ZFORCE_FRAME_START_IDX] == FRAME_START) {
						zforce_read(&frameHeader[ZFORCE_FRAME_SIZE_IDX], 1);
						print_data_block(frameHeader, 2);
					} else {
						*result = ZF_ERR_FORCE_RST;
					}
				}
        	} else {
				DEBUG_ERR("read next byte for size\n");
				frameHeader[ZFORCE_FRAME_START_IDX] = frameHeader[ZFORCE_FRAME_START_IDX+1];
				zforce_read(&frameHeader[ZFORCE_FRAME_START_IDX+1], 1);
				print_data_block(frameHeader, 2);
			}
	}

	frameSize = (int)frameHeader[ZFORCE_FRAME_SIZE_IDX];
	DEBUG_INFO("Frame size : %d\n", frameSize);

	data = kzalloc(frameSize * sizeof(u8), GFP_KERNEL);
	if (!data)
	{
		DEBUG_ERR(ZF_ERR_ALLOC);
		*result = ZF_ERROR;
		return;
	}

	/* Read the frame data */
	if (!zforce_read(data, frameSize))
	{
		DEBUG_ERR(ZF_ERR_READ_DATA);
		kfree(data);
		*result = ZF_ERR_FORCE_RST;
		return;
	}

	DEBUG_INFO("process:Frame Data Received:\n");
	print_data_block(data, frameSize);

	while(dataRead < frameSize)
	{
		type = data[ZFORCE_ASN_MSG_TYPE_IDX];
		DEBUG_INFO("process:Request: 0x%02x __ Type received: 0x%02x\n", request, type);
		
		switch(type) {
		case(ZF_ASN_TOUCH_NOTIF):
		{
	        	TouchData *tData = &zforce_data.tData;
			mutex_lock(&contactframe_mutex);
			parseAsnTouchData(data, tData);
			if (request == type)
				*result = ZF_OK;
			sendTouchUpdate(tData);
			mutex_unlock(&contactframe_mutex);
			dataRead = frameSize;
			break;
	
		}
		case(ZF_ASN_BootComplete):
		{
			DEBUG_INFO("Got ZF_ASN_BootComplete response\n");
			if (request == ZF_ASN_BootComplete)
				*result =  ZF_OK;
			dataRead = frameSize;
			break;
		}
		case(ZF_ASN_EnableDevice):
		{
			DEBUG_INFO("Got ZF_ASN_EnableDevice response\n");
			if (request == ZF_ASN_EnableDevice)
				*result = ZF_OK;
			dataRead = frameSize;
	        	break;
		}
		case(ZF_ASN_GetTouchFormat):
		{
			DEBUG_INFO("Got ZF_ASN_GetTouchFormat response\n");
			if (request == ZF_ASN_GetTouchFormat)
				*result = ZF_OK;
			dataRead = frameSize;
				break;
		}
		case(ZF_ASN_SetDetection):
		{
			DEBUG_INFO("Got ZF_ASN_SetDetection response\n");
			if (request == ZF_ASN_SetDetection)
				*result = ZF_OK;
			dataRead = frameSize;
				break;
		}
		case(ZF_ASN_GetLedLevel):
		{
			LedLevelResponse *ledLvl = &zforce_data.ledLvl;
			DEBUG_INFO("Got ZF_ASN_GetLedLevel response\n");
			if (request == ZF_ASN_GetLedLevel)
				*result = ZF_OK;
			readLedLevelResponse(data, ledLvl);
			dataRead = frameSize;
			
	        	break;
		}
		case(ZF_ASN_SetCalibration):
		{
			DEBUG_INFO("Got ZF_ASN_SetCalibration response\n");
			if (request == ZF_ASN_SetCalibration)
				*result = ZF_OK;
			dataRead = frameSize;
			
				break;
		}
		case(ZF_ASN_EnumerateDevice):
		{
			DEBUG_INFO("Got ZF_ASN_EnumerateDevice response\n");
			if (request == ZF_ASN_EnumerateDevice)
				*result = ZF_OK;
			dataRead = frameSize;
	        	break;
		}
		case(ZF_ASN_Resolution):
		{
			DEBUG_INFO("Got ZF_ASN_RESOLUTION_CMD response\n");
			if (request == ZF_ASN_Resolution)
				*result = ZF_OK;
			dataRead = frameSize;
			break;
		}
	      	case(INVALID_COMMAND_RES):
	      	{
			DEBUG_INFO("Invalid command response on command: 0x%x\n", data[dataRead]);
	      		break;
		}
		default:
		{
			DEBUG_ERR(ZF_ERR_UNKNOWN_TYPE, type);
			dataRead = frameSize;
			break;
		}
		} /* end of switch */
	}
	DEBUG_INFO("In process Data. Result is : %d\n", *result);
	kfree(data);

	return;
}

ZFStatus sendAsnEnableDevice(u8 enable)
{
	u8 MsgLen=12;
	u8 command[13]={0xEE,0x0A,0xEE,0x08,0x40,0x02,0x01,0x00,0x65,0x02,0x80,0x00}; //CORE device
	ZFStatus result = ZF_OK;

	if(enable) command[10]=0x81;

        if (!zforce_write(command, MsgLen))
        {
                DEBUG_ERR("Send ASN Enable Device Error\n");
                return ZF_ERROR;
        }
	
	if (zforce_touch_poll())
		return ZF_ERROR;
	
	processAsnData(ZF_ASN_EnableDevice, &result);

        return result;
}

ZFStatus sendAsnGetTouchFormat()
{
	u8 MsgLen=10;
	u8 command[11]={0xEE,0x08,0xEE,0x06,0x40,0x02,0x01,0x00,0x66,0x00}; //Core Device
	ZFStatus result = ZF_OK;

        if (!zforce_write(command, MsgLen))
        {
                DEBUG_ERR("Send ASN GetTouchFormat Error\n");
                return ZF_ERROR;
        }
	if (zforce_touch_poll())
		return ZF_ERROR;

	processAsnData(ZF_ASN_GetTouchFormat, &result);

        return result;
}

ZFStatus sendAsnSetDetection()
{
	u8 MsgLen=13;
	u8 command[14]={0xEE,0x0B,0xEE,0x09,0x40,0x02,0x01,0x00,0x67,0x03,0x80,0x01,0xFF}; //Core Device
	ZFStatus result = ZF_OK;

        if (!zforce_write(command, MsgLen))
        {
                DEBUG_ERR("Send ASN SetDetection Error\n");
                return ZF_ERROR;
        }
	if (zforce_touch_poll())
		return ZF_ERROR;

	processAsnData(ZF_ASN_SetDetection, &result);

        return result;
}

ZFStatus sendAsnGetLedLevel(u8 enable)
{
//	u8 MsgLen=14;
//	u8 command[15]={0xEE,0x0E,0xEE,0x0C,0x40,0x02,0x01,0x00,0x67,0x03,0x80,0x01,0x82,0x01,0xFF};
	u8 MsgLen=13;
	u8 command[14]={0xEE,0x0B,0xEE,0x09,0x40,0x02,0x01,0x00,0x67,0x03,0x82,0x01,0xFF};
	ZFStatus result = ZF_OK;
	
	if(!enable)
		command[12] = 0x0;
	
	if (!zforce_write(command, MsgLen))
	{
		DEBUG_ERR("Send ASN GetLedLevel Error\n");
		return ZF_ERROR;
	}
	
	if (zforce_touch_poll())
		return ZF_ERROR;
	
	processAsnData(ZF_ASN_SetDetection, &result);

	return result;
}

ZFStatus SendAsnEnableCalibration(u8 enable)
{
	u8 MsgLen=13;
	u8 command[14]={0xEE,0x0B,0xEE,0x09,0x40,0x02,0x01,0x00,0x75,0x03,0x81,0x01,0x00};
	ZFStatus result = ZF_OK;

	if(enable)
		command[12]=0x01;

	if (!zforce_write(command, MsgLen))
	{
		DEBUG_ERR("Send ASN GetLedLevel Error\n");
		return ZF_ERROR;
	}

	if (zforce_touch_poll())
		return ZF_ERROR;
	
	processAsnData(ZF_ASN_SetCalibration, &result);

	return result;
}

ZFStatus sendAsnDeviceInfo()
{
	u8 MsgLen=10;
	u8 command[11]={0xEE,0x08,0xEE,0x06,0x40,0x02,0x00,0x00,0x6c,0x00}; 
	ZFStatus result = ZF_OK;

        if (!zforce_write(command, MsgLen))
        {
                DEBUG_ERR("Send ASN Device Info Error\n");
                return ZF_ERROR;
        }
	if (zforce_touch_poll())
		return ZF_ERROR;
	
	processAsnData(ZF_ASN_DeviceInfo, &result);

        return result;
}

ZFStatus sendAsnEnumerteDevice()
{
	u8 MsgLen=10;
	u8 command[11]={0xEE,0x08,0xEE,0x06,0x40,0x02,0x00,0x00,0x6f,0x00}; 
	ZFStatus result = ZF_OK;

        if (!zforce_write(command, MsgLen))
        {
                DEBUG_ERR("Send ASN Enumerate Device Error\n");
                return ZF_ERROR;
        }
	if (zforce_touch_poll())
		return ZF_ERROR;
	
	processAsnData(ZF_ASN_EnumerateDevice, &result);

        return result;
}


ZFStatus initializeAsnDevice()
{
	ZFStatus status = ZF_OK;
	int bootCmpltFailRst = 0;
	u8	enable=1;

	DEBUG_INFO("Initialize...");

	if (atomic_read(&zforce_data.state) != ZF_STATE_RUN)
		return status;

asnrst_retry:	
	// Need to read BootComplete
	if((status = getBootComplete()) != ZF_OK) {
		if(bootCmpltFailRst++ < ZF_RESET_THRSHLD) {
			DEBUG_ERR("\nASN Boot complete failed! retrying..\n");
			bslReset(ZF_STATE_RESET);
			goto asnrst_retry;
		}
		DEBUG_ERR("\nASN Boot complete failed %d times..giving up!!\n", bootCmpltFailRst);
		return status;
	}

	status=sendAsnEnableDevice(0);
	if(status!=ZF_OK)
		return status;

	status=sendAsnSetResolution(ST_RES_WIDTH, ST_RES_HEIGHT);
	if(status!=ZF_OK)
		return status;
	
	status=sendAsnSetDetection();
	if(status!=ZF_OK)
		return status;

	status=sendAsnGetTouchFormat();
	if(status!=ZF_OK)
		return status;

	status=sendAsnEnableDevice(1);
	if(status!=ZF_OK)
		return status;
	
	printk(KERN_DEBUG "zforce2 device init completed\n");
	return status;
}


ZFStatus initialize(void)
{
	ZFStatus status = ZF_OK;
	int bootCmpltFailRst = 0;

	DEBUG_INFO("Initialize...");

	if (atomic_read(&zforce_data.state) != ZF_STATE_RUN)
		return status;

rst_retry:	
	// Need to read BootComplete
	if((status = getBootComplete()) != ZF_OK) {
		if(bootCmpltFailRst++ < ZF_RESET_THRSHLD) {
			DEBUG_ERR("\nBoot complete failed! retrying..\n");
			bslReset(ZF_STATE_RESET);
			goto rst_retry;
		}
		DEBUG_ERR("\nBoot complete failed %d times..giving up!!\n", bootCmpltFailRst);
		return status;
	}

	status = sendActivate();
	if (status != ZF_OK)
		return status;

	status = sendSetResolution(X_RESOLUTION, Y_RESOLUTION);
	if (status != ZF_OK)
		return status;
	status = sendSetScanningFrequency(ZF_IDLE_FREQ, ZF_FULL_FREQ, ZF_FULL_FREQ);
	if (status != ZF_OK)
		return status;

	if (unlikely(reltest)) {
		zforce_run_reltests();
	} else {
		status = sendSetConfiguration(ZFORCE_DUAL_TOUCH);
		if (status != ZF_OK)
			return status;
	        status = sendMcuSettingsRequest();
		if (status != ZF_OK)
			return status;
	}
	status = sendTouchDataRequest();
	return status;
}

ZFStatus stop(void)
{
	int ret = 0;
	ret = ZF_OK;

	if (zforce_data.mode == MODE_I2C &&
		atomic_read(&zforce_data.state) != ZF_STATE_UPDATE &&
		sendDeactivate() != ZF_OK) {
		DEBUG_INFO("stop:Could not send deactivate\n");
		ret = ZF_ERROR;
	}
	return ret;
}

static void zforce_init_work_handler(struct work_struct *unused)
{
	ZFStatus  status = ZF_OK;

	if(atomic_read(&zforce_data.state) == ZF_STATE_RESET) {
		DEBUG_ERR("reset:Forcing a reset\n");
		zforce_free_touch_irq();
		bslReset(ZF_STATE_RESET);
		clearTouch();
	}

	atomic_set(&zforce_data.state, ZF_STATE_RUN);
	if(deviceType==1)
	{
		if ((status = initialize()) != ZF_OK) {
			if (status == ZF_TIMEOUT)
				DEBUG_ERR(ZF_ERR_TIMEOUT);
			DEBUG_ERR(ZF_ERR_INIT);
			return;
		}
	}
	else if(deviceType==2)
	{
		if ((status = initializeAsnDevice()) != ZF_OK) {
			if (status == ZF_TIMEOUT)
				DEBUG_ERR(ZF_ERR_TIMEOUT);
			DEBUG_ERR(ZF_ERR_INIT);
			return;
		}
	}
	else{
		DEBUG_ERR("Unknown DeviceType: %d\n",deviceType);
		return;
	}

	if(touchdatapoll) //fred debug to keep poll touch data
	{
		zforce_touch_data_poll();
	}
	
	if (zforce_get_touch_irq() != 0) {
		DEBUG_ERR(ZF_ERR_IRQ, zforce_data.irq);
		goto out_free_irq;
	}

	zforce_data.isConnected = 1;

	/* Let user space know we are here */
	kobject_uevent(&zforce_data.tdev->dev.kobj, KOBJ_ADD);

	initSuccess = 1;
	wake_up(&zf_init_wq);
	return;

out_free_irq:
	zforce_data.irq       = 0;
	gpio_touchcntrl_request_irq(0);
	atomic_set(&zforce_data.state, ZF_STATE_STOP);
	initSuccess           = 0;
	wake_up(&zf_init_wq);
}

static void zforce_init_data(void)
{
	int i;

	zforce_data.pdev        = NULL;
	atomic_set(&zforce_data.state, ZF_STATE_STOP);
	zforce_data.irq         = 0;
	zforce_data.mode        = MODE_I2C;
	zforce_data.ledlvl_ready= 0;
	zforce_data.isConnected = 0;
	zforce_data.probed      = 0;
	zforce_data.work_queue  = NULL;
	zforce_data.relTime     = 0;
	zforce_data.relStrength = 3;
	zforce_data.tdev        = NULL;
	zforce_data.exlock	= false;

	/* Initialize touch state to empty */
	zforce_data.tData.count = 0;
	for(i = 0; i < MAX_CONTACTS; i++)
	{
		zforce_data.tData.coordinateData[i].id = i + 1;
		zforce_data.tData.coordinateData[i].touch_state = TOUCH_UP;
	}
}

static int zforce_probe(struct platform_device* pdev)
{
	ZFStatus  status       = ZF_OK;
	initSuccess            = -1;

	zforce_init_data();

	zforce_data.work_queue = create_singlethread_workqueue("zforce_wq");
	if (!zforce_data.work_queue) {
		status = ENOMEM;
		goto out;
	}
	init_waitqueue_head(&zf_user_wq);
	init_waitqueue_head(&zf_init_wq);

	if (register_input_device(pdev) < 0) {
		status = ZF_ERROR;
		goto out_free_irq;
	}

	/* Create sys entry : connected */
	if(device_create_file(&pdev->dev, &dev_attr_connected) < 0) {
		status = ZF_ERROR;
		DEBUG_ERR(ZF_ERR_CONNECT_FILE);
		goto out_remove_input_dev;
	}

	if (device_create_file(&pdev->dev, &dev_attr_version) < 0) {
		status = ZF_ERROR;
		DEBUG_ERR(ZF_ERR_VERSION_FILE);
		goto out_remove_connect_file;
	}

	if (device_create_file(&pdev->dev, &dev_attr_mcusetting) < 0) {
		status = ZF_ERROR;
		DEBUG_ERR(ZF_ERR_MCUSETTING_FILE);
		goto out_remove_version_file;
	}

	if (device_create_file(&pdev->dev, &dev_attr_bslreset) < 0) {
		status = ZF_ERROR;
		DEBUG_ERR(ZF_ERR_BSLRESET_FILE);
		goto out_remove_mcusetting_file;
	}

	if (device_create_file(&pdev->dev, &dev_attr_scanningfreq) < 0) {
		status = ZF_ERROR;
		DEBUG_ERR(ZF_ERR_SCANFREQ_FILE);
		goto out_remove_bslreset_file;
	}

	/* Create sys entry : mode */
	if (device_create_file(&pdev->dev, &dev_attr_mode) < 0) {
		status = ZF_ERROR;
		DEBUG_ERR(ZF_ERR_MODE_FILE);
		goto out_remove_scanfreq_file;
	}

	/* Create sys entry : test */
	if (device_create_file(&pdev->dev, &dev_attr_test) < 0) {
		status = ZF_ERROR;
		DEBUG_ERR(ZF_ERR_TEST_FILE);
		goto out_remove_mode_file;
	}

	if (device_create_file(&pdev->dev, &dev_attr_ledlevel) < 0) {
		status = ZF_ERROR;
		DEBUG_ERR(ZF_ERR_LEDLEVEL_FILE);
		goto out_remove_ledlevel_file;
	}

	if (device_create_file(&pdev->dev, &dev_attr_inputgpio) < 0)
	{
	  status = ZF_ERROR;
	  DEBUG_ERR(ZF_ERR_INPUTGIO_FILE);
	  goto out_remove_test_file;
	}

	/* Create misc device */
	if (misc_register(&zforce_misc_device)) {
		status = ZF_ERROR;
		DEBUG_ERR(ZF_ERR_MISC_FILE);
		goto out_remove_inputgpio_file;
	}

//	proc_entry = create_proc_entry(ZF_PROC_NAME, 0644, NULL );
	proc_entry = proc_create(ZF_PROC_NAME, 0, NULL, &proc_file_fops);
	if (proc_entry == NULL) {
		DEBUG_ERR("create_proc: could not create proc entry\n");
		goto out_remove_misc_file;
	}

	mutex_init(&contactframe_mutex);
	mutex_init(&ledlvl_mutex);

	zforce_data.pdev = pdev;

	zforce_data.probed = 1;

	queue_work(zforce_data.work_queue, &zforce_init_work);

	wait_event_timeout(zf_init_wq, initSuccess >= 0, msecs_to_jiffies(2000));

	return (int)status;

out_remove_misc_file:
	misc_deregister(&zforce_misc_device);
out_remove_inputgpio_file:
	device_remove_file(&pdev->dev, &dev_attr_inputgpio);
out_remove_ledlevel_file:
		device_remove_file(&pdev->dev, &dev_attr_ledlevel);
out_remove_test_file:
	device_remove_file(&pdev->dev, &dev_attr_test);
out_remove_mode_file:
	device_remove_file(&pdev->dev, &dev_attr_mode);
out_remove_scanfreq_file:
	device_remove_file(&pdev->dev, &dev_attr_scanningfreq);
out_remove_bslreset_file:
	device_remove_file(&pdev->dev, &dev_attr_bslreset);
out_remove_mcusetting_file:
	device_remove_file(&pdev->dev, &dev_attr_mcusetting);
out_remove_version_file:
	device_remove_file(&pdev->dev, &dev_attr_version);
out_remove_connect_file:
	device_remove_file(&pdev->dev, &dev_attr_connected);
out_remove_input_dev:
	remove_input_device();
out_free_irq:
	atomic_set(&zforce_data.state, ZF_STATE_STOP);
	zforce_free_touch_irq();
out:
	return -(int)status;
}

static void zforce_stop_touch(ZFState state)
{
	DEBUG_INFO("Stopping thread\n");

	mutex_lock(&stop_mutex);

	atomic_set(&zforce_data.state, ZF_STATE_STOP);

	zforce_free_touch_irq();

	if (zforce_data.work_queue) {
		destroy_workqueue(zforce_data.work_queue);
		zforce_data.work_queue = NULL;
	}

	bslReset(state);

	zforce_data.mode        = MODE_I2C;
	zforce_data.ledlvl_ready= 0;
	zforce_data.isConnected = 0;

	initSuccess             = -1;

	clearTouch();

	mutex_unlock(&stop_mutex);

	DEBUG_INFO("Stopping thread done\n");
}

static void zforce_restart_touch(void)
{
	DEBUG_INFO("Starting thread\n");
	atomic_set(&zforce_data.state, ZF_STATE_RUN);

	// Reset the chip
	bslReset(ZF_STATE_RESET);

	clearTouch();

	if(zforce_data.work_queue == NULL)
		zforce_data.work_queue = create_singlethread_workqueue("zforce_wq");
	queue_work(zforce_data.work_queue, &zforce_init_work);
	wait_event_timeout(zf_init_wq, initSuccess >= 0, msecs_to_jiffies(2000));
	DEBUG_INFO("Starting thread done\n");
}

static void zforce_set_ready_for_update(void)
{
	zforce_stop_touch(ZF_STATE_UPDATE);
	atomic_set(&zforce_data.state, ZF_STATE_UPDATE);
}

static void zforce_set_suspend_pads(void)
{
	/* Put all GPIOs to output low */
	gpio_direction_output( IMX_GPIO_NR(4, 3) , 0);
	gpio_direction_output( IMX_GPIO_NR(4, 5) , 0);
	gpio_direction_output( IMX_GPIO_NR(3, 24) , 0);
	gpio_direction_output( IMX_GPIO_NR(5, 12) , 0);
	gpio_direction_output( IMX_GPIO_NR(5, 9) ,  0);
	mdelay(10);
}

static void zforce_set_normal_pads(void)
{

	mdelay(10);
	/* GPIO Interrupt - is set as input */
	gpio_direction_input(IMX_GPIO_NR(4, 3));
	/* trigger reset - active low */
	gpio_direction_output(IMX_GPIO_NR(4, 5), 0);
	/* turn-off BSL lines */
	gpio_zforce_bslpins_ena(false);
	mdelay(10);
}

static int zforce_suspend_touch(void)
{	
	int ret = 0;
	zforce_stop_touch(ZF_STATE_STOP);

	touch_suspend();

	false_touch_warning = false;
	last_touch_time = 0;
	touch_count = 0;

	msleep(5);
	zforce_set_suspend_pads();

	/* power cut at the end of supsend */
	printk(KERN_ERR "Suspend touch\n");
	msleep(5);

	ret = regulator_disable(zforce_data.touchreg);	
	if(ret) {
		printk(KERN_ERR "%s: Failed to disable touch regulator!\n",
			__func__);
		return -EAGAIN;
	}

	return 0;
}

static int zforce_resume_touch(void)
{
	int ret = 0;

	printk(KERN_ERR "Resume touch\n");

	/* power on at beginning of resume */
	ret = regulator_enable(zforce_data.touchreg);
	if(ret) {
		printk(KERN_CRIT "%s: Touch regulator enable failed with error \
			%d! Touch might not be operational!\n", __func__, ret);
		return -EAGAIN;
	}

	touch_resume();
	msleep(5);

	zforce_set_normal_pads();

	msleep(5);
	zforce_restart_touch();	
	
	return 0;
}

static int zforce_remove(struct platform_device *pdev)
{
	if (zforce_data.probed) {
		zforce_data.probed = 0;

		kobject_uevent(&zforce_data.tdev->dev.kobj, KOBJ_REMOVE);
		misc_deregister(&zforce_misc_device);
		remove_proc_entry(ZF_PROC_NAME, NULL);
		device_remove_file(&pdev->dev, &dev_attr_connected);
		device_remove_file(&pdev->dev, &dev_attr_version);
		device_remove_file(&pdev->dev, &dev_attr_mcusetting);
		device_remove_file(&pdev->dev, &dev_attr_scanningfreq);
		device_remove_file(&pdev->dev, &dev_attr_bslreset);
		device_remove_file(&pdev->dev, &dev_attr_test);
		device_remove_file(&pdev->dev, &dev_attr_mode);
		device_remove_file(&pdev->dev, &dev_attr_ledlevel);
	}
	return 0;
}

static void zforce_shutdown(struct platform_device *pdev)
{
	/* free up resource allocated before sendDeactivate cmd */
	zforce_stop_touch(ZF_STATE_STOP);
}

static struct platform_driver zforce_driver =
{
	.driver = {
		.name = ZF_DRIVER_NAME,
	},
	.probe = zforce_probe,
	.shutdown = zforce_shutdown,
	.remove = zforce_remove,
};

static void zforce_nop_release(struct device* dev)
{
	/* Do Nothing */
}

static struct platform_device zforce_device =
{
	.name = ZF_DRIVER_NAME,
	.id   = 0,
	.dev  = {
		.release = zforce_nop_release,
  	},
};

static int  __init zforce_init(void)
{
	int ret = 0;

	i2c_probe_success = 0;
//        i2c_register_board_info(0, zforce_i2c_devices,ARRAY_SIZE(zforce_i2c_devices));
	ret = i2c_add_driver(&zforce_i2c_driver);
	if (ret < 0) {
		DEBUG_ERR(ZF_ERR_I2C_ADD);
		return -ENODEV;
	}
	if (!i2c_probe_success) {
		i2c_del_driver(&zforce_i2c_driver);
		DEBUG_ERR();
		return -ENODEV;
	}

	DEBUG_INFO("Registering platform device\n");
	platform_device_register(&zforce_device);
	platform_driver_register(&zforce_driver);
	return ret;
}

static void __exit zforce_exit(void)
{
	DEBUG_INFO("Calling exit");
	if (i2c_probe_success) {
		i2c_probe_success = 0;
		platform_driver_unregister(&zforce_driver);
		platform_device_unregister(&zforce_device);
  	}
	i2c_del_driver(&zforce_i2c_driver);
}

module_init(zforce_init);
module_exit(zforce_exit);

MODULE_DESCRIPTION("Neonode ZForce2 driver");
MODULE_AUTHOR("Nadim Awad <nawad@lab126.com>");
MODULE_LICENSE("GPL");
