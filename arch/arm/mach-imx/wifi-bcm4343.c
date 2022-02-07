/*
 * Copyright (c) 2015-2016 Amazon.com, Inc. or its affiliates.
 * All Rights Reserved.
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/skbuff.h>
#include <linux/mmc/host.h>
#ifdef CONFIG_LAB126
#include <linux/mmc/sdhci.h>
#endif
#include <linux/if.h>
#include <linux/of.h>

#include <linux/of_gpio.h>

#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/random.h>
static int g_gpio_wl_reg_on = -1;
static int g_gpio_wl_host_wake = -1;

#ifndef _LINUX_WLAN_PLAT_H_
#define _LINUX_WLAN_PLAT_H_
/* This structure is currently defined in broadcom DHD driver
 * dhd_linux_platdev.c and must be kept in sync
 */
struct wifi_platform_data {
	int (*set_power)(int val);
	int (*set_reset)(int val);
	int (*set_carddetect)(int val);
	void *(*mem_prealloc)(int section, unsigned long size);
	int (*get_mac_addr)(unsigned char *buf);
	void *(*get_country_code)(char *ccode);
};

#endif

/* Customized Locale table : OPTIONAL feature */
#define WLC_CNTRY_BUF_SZ        4
struct cntry_locales_custom {
	char iso_abbrev[WLC_CNTRY_BUF_SZ];
	char custom_locale[WLC_CNTRY_BUF_SZ];
	int  custom_locale_rev;
};

static struct cntry_locales_custom brcm_wlan_translate_custom_table[] = {
	/* Table should be filled out based on custom platform regulatory requirement */
	{"",   "XT", 49},  /* Universal if Country code is unknown or empty */
	{"US", "US", 176},
	{"AE", "AE", 1},
	{"AR", "AR", 21},
	{"AT", "AT", 4},
	{"AU", "AU", 40},
	{"BE", "BE", 4},
	{"BG", "BG", 4},
	{"BN", "BN", 4},
	{"BR", "BR", 4},
	{"CA", "US", 176},   /* Previousely was CA/31 */
	{"CH", "CH", 4},
	{"CY", "CY", 4},
	{"CZ", "CZ", 4},
	{"DE", "DE", 7},
	{"DK", "DK", 4},
	{"EE", "EE", 4},
	{"ES", "ES", 4},
	{"FI", "FI", 4},
	{"FR", "FR", 5},
	{"GB", "GB", 6},
	{"GR", "GR", 4},
	{"HK", "HK", 2},
	{"HR", "HR", 4},
	{"HU", "HU", 4},
	{"IE", "IE", 5},
	{"IN", "IN", 28},
	{"IS", "IS", 4},
	{"IT", "IT", 4},
	{"ID", "ID", 13},
	{"JP", "JP", 86},
	{"KR", "KR", 57},
	{"KW", "KW", 5},
	{"LI", "LI", 4},
	{"LT", "LT", 4},
	{"LU", "LU", 3},
	{"LV", "LV", 4},
	{"MA", "MA", 2},
	{"MT", "MT", 4},
	{"MX", "MX", 20},
	{"MY", "MY", 16},
	{"NL", "NL", 4},
	{"NO", "NO", 4},
	{"NZ", "NZ", 4},
	{"PL", "PL", 4},
	{"PT", "PT", 4},
	{"PY", "PY", 2},
	{"RO", "RO", 4},
	{"RU", "RU", 13},
	{"SE", "SE", 4},
	{"SG", "SG", 19},
	{"SI", "SI", 4},
	{"SK", "SK", 4},
	{"TH", "TH", 5},
	{"TR", "TR", 7},
	{"TW", "TW", 1},
	{"VN", "VN", 4},
};

static void *brcm_wlan_get_country_code(char *ccode)
{
	struct cntry_locales_custom *locales;
	int size;
	int i;

	if (!ccode)
		return NULL;

	locales = brcm_wlan_translate_custom_table;
	size = ARRAY_SIZE(brcm_wlan_translate_custom_table);

	for (i = 0; i < size; i++)
		if (strcmp(ccode, locales[i].iso_abbrev) == 0)
			return &locales[i];
	return &locales[0];
}

/*
 * JEIGHT-1371: On Eanab, kmalloc() fails randomly at __alloc_pages_nodemask().
 * Workaround: pre-allocate the pages at system init and use it in wifi module.
 */
#ifdef CONFIG_DHD_USE_STATIC_BUF
	#define WLAN_STATIC_SCAN_BUF0        5
	#define WLAN_STATIC_SCAN_BUF1        6
	#define WLAN_STATIC_DHD_INFO_BUF     7
	#define WLAN_STATIC_WLFC_INFO_BUF    8
	#define WLAN_STATIC_DHD_IF_FLOW_LKUP 9
	#define WLAN_STATIC_FW_BUF           11
	#define WLAN_SCAN_BUF_SIZE           (64 * 1024)
	#define WLAN_DHD_INFO_BUF_SIZE       (24 * 1024)
	#define WLAN_WLFC_INFO_BUF_SIZE      (24 * 1024)
	#define WLAN_DHD_IF_FLOW_LKUP_SIZE   (20 * 1024)
	/* WLAN_FW_BUF_SIZE must be >= DHD FW_IMAGE_MAX + DHD_SDALIGN */
	#define WLAN_FW_BUF_SIZE             (384 * 1024 + PAGE_SIZE)

	#define PREALLOC_WLAN_SEC_NUM 4
	#define PREALLOC_WLAN_BUF_NUM 160
	#define PREALLOC_WLAN_SECTION_HEADER 24

	#ifdef CONFIG_BCMDHD_PCIE
		#define WLAN_SECTION_SIZE_0	(PREALLOC_WLAN_BUF_NUM * 128)
		#define WLAN_SECTION_SIZE_1	0
		#define WLAN_SECTION_SIZE_2	0
		#define WLAN_SECTION_SIZE_3	(PREALLOC_WLAN_BUF_NUM * 1024)
	#else
		#define WLAN_SECTION_SIZE_0	(PREALLOC_WLAN_BUF_NUM * 128)
		#define WLAN_SECTION_SIZE_1	(PREALLOC_WLAN_BUF_NUM * 128)
		#define WLAN_SECTION_SIZE_2	(PREALLOC_WLAN_BUF_NUM * 512)
		#define WLAN_SECTION_SIZE_3	(PREALLOC_WLAN_BUF_NUM * 1024)
	#endif /* CONFIG_BCMDHD_PCIE */

	#define DHD_SKB_HDRSIZE 336
	#define DHD_SKB_1PAGE_BUFSIZE	((PAGE_SIZE*1)-DHD_SKB_HDRSIZE)
	#define DHD_SKB_2PAGE_BUFSIZE	((PAGE_SIZE*2)-DHD_SKB_HDRSIZE)
	#define DHD_SKB_4PAGE_BUFSIZE	((PAGE_SIZE*4)-DHD_SKB_HDRSIZE)

	#define DHD_SKB_1PAGE_BUF_NUM	8
	#define DHD_SKB_2PAGE_BUF_NUM	8
	#define DHD_SKB_4PAGE_BUF_NUM	1

	#define WLAN_SKB_1_2PAGE_BUF_NUM ((DHD_SKB_1PAGE_BUF_NUM) + \
				(DHD_SKB_2PAGE_BUF_NUM))

	#define WLAN_SKB_BUF_NUM ((WLAN_SKB_1_2PAGE_BUF_NUM) + \
				(DHD_SKB_4PAGE_BUF_NUM))

static struct sk_buff *wlan_static_skb[WLAN_SKB_BUF_NUM];

struct wlan_mem_prealloc {
	void *mem_ptr;
	unsigned long size;
};

static struct wlan_mem_prealloc wlan_mem_array[PREALLOC_WLAN_SEC_NUM] = {
	{NULL, (WLAN_SECTION_SIZE_0 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_1 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_2 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_3 + PREALLOC_WLAN_SECTION_HEADER)}
};

void *wlan_static_scan_buf0 = NULL;
void *wlan_static_scan_buf1 = NULL;
void *wlan_static_dhd_info_buf = NULL;
void *wlan_static_dhd_wlfc_info = NULL;
void *wlan_static_if_flow_lkup = NULL;
void *wlan_static_fw_buf = NULL;


void *brcm_wlan_mem_prealloc(int section, unsigned long size)
{
	if (section == PREALLOC_WLAN_SEC_NUM)
		return wlan_static_skb;

	if (section == WLAN_STATIC_SCAN_BUF0)
		return wlan_static_scan_buf0;

	if (section == WLAN_STATIC_SCAN_BUF1)
		return wlan_static_scan_buf1;

	if (section == WLAN_STATIC_DHD_INFO_BUF) {
		if (size > WLAN_DHD_INFO_BUF_SIZE) {
			pr_err("request DHD_INFO size(%lu) is bigger than static size(%d).\n",
				size, WLAN_DHD_INFO_BUF_SIZE);
			return NULL;
		}
		return wlan_static_dhd_info_buf;
	}
	if (section == WLAN_STATIC_WLFC_INFO_BUF) {
		if (size > WLAN_WLFC_INFO_BUF_SIZE) {
			pr_err("request WLFC_INFO size(%lu) is bigger than static size(%d).\n",
				size, WLAN_WLFC_INFO_BUF_SIZE);
			return NULL;
		}
		return wlan_static_dhd_wlfc_info;
	}
	if (section == WLAN_STATIC_DHD_IF_FLOW_LKUP)  {
		if (size > WLAN_DHD_IF_FLOW_LKUP_SIZE) {
			pr_err("request DHD_IF_FLOW_LKUP size(%lu) is bigger than static size(%d).\n",
				size, WLAN_DHD_IF_FLOW_LKUP_SIZE);
			return NULL;
		}

		return wlan_static_if_flow_lkup;
	}
	if (section == WLAN_STATIC_FW_BUF) {
		if (size > WLAN_FW_BUF_SIZE) {
			pr_err("request FW_BUF size(%lu) is bigger than static size(%ld).\n",
				size, WLAN_FW_BUF_SIZE);
			return NULL;
		}

		return wlan_static_fw_buf;
	}
	if ((section < 0) || (section > PREALLOC_WLAN_SEC_NUM))
		return NULL;

	if (wlan_mem_array[section].size < size)
		return NULL;

	return wlan_mem_array[section].mem_ptr;
}
EXPORT_SYMBOL(brcm_wlan_mem_prealloc);

static int brcm_init_wlan_mem(void)
{
	int i, j;

	for (i = 0; i < DHD_SKB_1PAGE_BUF_NUM; i++) {
		wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_1PAGE_BUFSIZE);
		if (!wlan_static_skb[i])
			goto err_skb_alloc;
	}

	for (i = DHD_SKB_1PAGE_BUF_NUM; i < WLAN_SKB_1_2PAGE_BUF_NUM; i++) {
		wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_2PAGE_BUFSIZE);
		if (!wlan_static_skb[i])
			goto err_skb_alloc;
	}

	wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_4PAGE_BUFSIZE);
	if (!wlan_static_skb[i])
		goto err_skb_alloc;

	for (i = 0; i < PREALLOC_WLAN_SEC_NUM; i++) {
		if (wlan_mem_array[i].size > 0) {
			wlan_mem_array[i].mem_ptr =
				kmalloc(wlan_mem_array[i].size, GFP_KERNEL);
			if (!wlan_mem_array[i].mem_ptr)
				goto err_mem_alloc;
		}
	}

	wlan_static_scan_buf0 = kmalloc(WLAN_SCAN_BUF_SIZE, GFP_KERNEL);
	if (!wlan_static_scan_buf0) {
		pr_err("Failed to alloc wlan_static_scan_buf0\n");
		goto err_mem_alloc;
	}

	wlan_static_scan_buf1 = kmalloc(WLAN_SCAN_BUF_SIZE, GFP_KERNEL);
	if (!wlan_static_scan_buf1) {
		pr_err("Failed to alloc wlan_static_scan_buf1\n");
		goto err_mem_alloc;
	}

	wlan_static_dhd_info_buf = kmalloc(WLAN_DHD_INFO_BUF_SIZE, GFP_KERNEL);
	if (!wlan_static_dhd_info_buf) {
		pr_err("Failed to alloc wlan_static_dhd_info_buf\n");
		goto err_mem_alloc;
	}

	wlan_static_dhd_wlfc_info = kmalloc(WLAN_WLFC_INFO_BUF_SIZE, GFP_KERNEL);
	if (!wlan_static_dhd_wlfc_info) {
		pr_err("Failed to alloc wlan_static_dhd_wlfc_info\n");
		goto err_mem_alloc;
	}

#ifdef CONFIG_BCMDHD_PCIE
	wlan_static_if_flow_lkup = kmalloc(WLAN_DHD_IF_FLOW_LKUP_SIZE, GFP_KERNEL);
	if (!wlan_static_if_flow_lkup) {
		pr_err("Failed to alloc wlan_static_if_flow_lkup\n");
		goto err_mem_alloc;
	}
#endif /* CONFIG_BCMDHD_PCIE */

	wlan_static_fw_buf = kmalloc(WLAN_FW_BUF_SIZE, GFP_KERNEL);
	if (!wlan_static_fw_buf) {
		pr_err("Failed to alloc wlan_static_fw_buf\n");
		goto err_mem_alloc;
	}

	return 0;

err_mem_alloc:
#ifdef CONFIG_BCMDHD_PCIE
	if (wlan_static_if_flow_lkup)
		kfree(wlan_static_if_flow_lkup);
#endif /* CONFIG_BCMDHD_PCIE */

	if (wlan_static_dhd_info_buf)
		kfree(wlan_static_dhd_info_buf);

	if (wlan_static_dhd_wlfc_info)
		kfree(wlan_static_dhd_wlfc_info);

	if (wlan_static_scan_buf1)
		kfree(wlan_static_scan_buf1);

	if (wlan_static_scan_buf0)
		kfree(wlan_static_scan_buf0);

	pr_err("Failed to mem_alloc for WLAN\n");
	for (j = 0 ; j < i ; j++)
		kfree(wlan_mem_array[j].mem_ptr);

	i = WLAN_SKB_BUF_NUM;

err_skb_alloc:
	pr_err("Failed to skb_alloc for WLAN\n");
	for (j = 0 ; j < i ; j++)
		dev_kfree_skb(wlan_static_skb[j]);

	return -ENOMEM;
}
#endif /* CONFIG_DHD_USE_STATIC_BUF */


static struct resource brcm_wifi_resources[] = {
	[0] = {
		.name	= "bcmdhd_wlan_irq",
		.start	= 0, /* default */
		.end	= 0, /* default */
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL,
	},
};

#ifdef CONFIG_IDME
static int hexval(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0;
}

#define IDME_MACADDR        "/idme/mac_addr"
#endif

/*
 * bcm_wlan_get_mac_addr() - return the WiFi MAC address.
 *
 * This function is called by the WiFi driver during initialization.
 * The returned address is used to create the WiFi interface.
 * The MAC address is returned in a 6-byte buffer pointed to by buf.
 * Non-zero return code is treated as error and the content of the
 * buffer is ignored.
 */

static int bcm_wlan_get_mac_addr(unsigned char *buf)
{
#ifdef CONFIG_LAB126
	extern char lab126_mac_address[IFHWADDRLEN * 2 + 1];
	unsigned char mac_addr[IFHWADDRLEN], tmp[3];
	int idx, err;

	if (strlen(lab126_mac_address) != IFHWADDRLEN * 2) {
		pr_err("%s: invalid mac address string len=%d\n",
		       __func__, strlen(lab126_mac_address));
		return -EINVAL;
	}

	for (tmp[2] = 0, idx = 0; idx < IFHWADDRLEN; idx++) {
		tmp[0] = lab126_mac_address[idx * 2];
		tmp[1] = lab126_mac_address[idx * 2 + 1];
		err = kstrtou8(tmp, 16, &mac_addr[idx]);
		if (err) {
			pr_err("%s: invalid mac address string: '%s', err=%d\n",
			       __func__, lab126_mac_address, err);
			return err;
		}
	}

	pr_debug("%s: mac_addr=%pM\n", __func__, mac_addr);
#else	/* !CONFIG_LAB126 */
	unsigned char mac_addr[IFHWADDRLEN] = { 0, 0x90, 0x4c, 0, 0, 0 };
#ifndef CONFIG_IDME
	uint rand_mac;

//	srandom32((uint) jiffies);
//	rand_mac = random32();
	rand_mac=123456;
	mac_addr[3] = (unsigned char)rand_mac;
	mac_addr[4] = (unsigned char)(rand_mac >> 8);
	mac_addr[5] = (unsigned char)(rand_mac >> 16);
#else
	struct device_node *ap;
	int len = 0;
	int idx;
	const char *idme_mac;

	/* Get IDME mac address */
	ap = of_find_node_by_path(IDME_MACADDR);
	if (!ap) {
		pr_err("%s: Unable to get of node: %s\n", __func__, IDME_MACADDR);
		return -EINVAL;
	}

	idme_mac = of_get_property(ap, "value", &len);
	if (!idme_mac || (len < IFHWADDRLEN*2)) {
		pr_err("%s:%s invalid length %d\n", __func__, IDME_MACADDR, len);
		return -EINVAL;
	}

	/* Convert IDME mac address */
	for (idx = 0; idx < IFHWADDRLEN; idx++) {
		mac_addr[idx]  = hexval(idme_mac[idx*2])<<4;
		mac_addr[idx] += hexval(idme_mac[idx*2+1]);
	}
#endif
#endif	/* CONFIG_LAB126 */
	if (!buf)
		return -EFAULT;

	memcpy(buf, mac_addr, IFHWADDRLEN);

	return 0;
}

static int brcm_wlan_set_carddetect(int val)
{
	pr_info("%s: Not implemented\n", __func__);
	return 0;
}

static int brcm_wlan_reset(int onoff)
{
	pr_info("%s: Not implemented\n", __func__);
	return 0;
}

/* Must be called in non-atomic context */
int brcm_wlan_power(int on)
{
	pr_info("%s: setting gpio #[%d] %s\n",
		__func__, g_gpio_wl_reg_on, on ? "on" : "off");
#ifdef CONFIG_LAB126
	/* brcm_init_gpio_power() has configured it as output */
	gpio_set_value(g_gpio_wl_reg_on, on);
	/* BRCM data sheet "4343W-DS105-R" (1/12/2015)
	 * Section 22: Power-up Sequence and Timing:
	 * Wait at least 150 ms after VDDC and VDDIO are available
	 * before initiating SDIO accesses.
	 */
	msleep(on ? 150 : 100);
#else
	gpio_direction_output(g_gpio_wl_reg_on, on);
	mdelay(100);
#endif
	return 0;
}

#ifdef CONFIG_LAB126
extern struct sdhci_host *mmc1_host;

int wifi_card_enable(void)
{
	if(!mmc1_host) {
		printk(KERN_ERR "%s: device not available!\n", __func__);
		return -1;
	} else {
		brcm_wlan_power(1);
		tasklet_schedule(&mmc1_host->card_tasklet);
	}
	return 0;
}
EXPORT_SYMBOL(wifi_card_enable);

int wifi_card_disable(void)
{
	int timeout = 20;

	brcm_wlan_power(0);

	if(!mmc1_host) {
		printk(KERN_ERR "%s: device not available!\n", __func__);
		return -1;
	} else {
		tasklet_schedule(&mmc1_host->card_tasklet);
		msleep(10);
		while(mmc1_host->pwr != 0 && timeout) {
			msleep(100);
			timeout--;
		}
		if (!timeout && mmc1_host->pwr) {
			printk(KERN_ERR "%s: wifi module clean unload failed.\n", __func__);
			return -1;
		}
	}
	return 0;
}
EXPORT_SYMBOL(wifi_card_disable);
#endif

static struct wifi_platform_data brcm_wlan_control = {
	.set_power		= brcm_wlan_power,
	.set_reset		= brcm_wlan_reset,
	.set_carddetect		= brcm_wlan_set_carddetect,
	.get_mac_addr		= bcm_wlan_get_mac_addr,
#ifdef CONFIG_DHD_USE_STATIC_BUF
	.mem_prealloc		= brcm_wlan_mem_prealloc,
#else
	.mem_prealloc		= NULL,
#endif
	.get_country_code	= brcm_wlan_get_country_code,
};

static struct platform_device brcm_wifi_device = {
	.name		= "bcmdhd_wlan",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(brcm_wifi_resources),
	.resource	= brcm_wifi_resources,
	.dev		= {
		.platform_data = &brcm_wlan_control,
	},
};

int __init brcm_init_gpio_power(struct device_node *pnode)
{
	int ret = 0;

	if (!pnode) {
		pr_err("%s: invalid pnode\n", __func__);
		return -EINVAL;
	}

	/* Get the REG_ON GPIO from device tree */
	ret = of_get_named_gpio(pnode, "wl_reg_on", 0);
	if (ret < 0) {
		pr_err("%s: get wl_reg_on GPIO failed err=%d\n", __func__, ret);
		return ret;
	}

	/* Update the reg on gpio */
	g_gpio_wl_reg_on = ret;

	ret = gpio_request(g_gpio_wl_reg_on, "WL_REG_ON");
	if (ret) {
		pr_err("%s: Failed to request gpio %d for WL_REG_ON\n",
				__func__, g_gpio_wl_reg_on);
		return ret;
	}

#ifdef CONFIG_LAB126
	/* Configure wifi GPIO output and default to off */
	ret = gpio_direction_output(g_gpio_wl_reg_on, 0);
#else
	/* Turn on the wifi module */
	ret = gpio_direction_output(g_gpio_wl_reg_on, 1);
#endif
	if (ret) {
		pr_err("%s: WL_REG_ON failed to pull up\n", __func__);
		return ret;
	}

#ifndef CONFIG_LAB126
	msleep(100); /* required per Benzy */

	gpio_set_value(g_gpio_wl_reg_on, 1);
#endif

	return 0;
}

int __init brcm_init_gpio_hostwake_irq(struct device_node *pnode)
{
	int irq = 0;
	int ret = 0;

	if (!pnode) {
		pr_err("%s: invalid pnode\n", __func__);
		return -EINVAL;
	}

	ret = of_get_named_gpio(pnode, "wl_host_wake", 0);
	if (ret < 0) {
		pr_err("%s: get wl_host_wake GPIO failed err=%d\n", __func__, ret);
		return ret;
	}

	g_gpio_wl_host_wake = ret;

	ret = gpio_request(g_gpio_wl_host_wake, "bcmdhd_wlan_irq");
	if (ret) {
		pr_err("%s: failed to request gpio %d for brcm wifi err=%d\n", __func__, g_gpio_wl_host_wake, ret);
		return ret;
	}

	ret = gpio_direction_input(g_gpio_wl_host_wake);
	if (ret) {
		pr_err("%s: configuration failure err=%d\n", __func__, ret);
		gpio_free(g_gpio_wl_host_wake);
		return ret;
	}

	/* Out Of Band interrupt */
	irq = gpio_to_irq(g_gpio_wl_host_wake);
	if (irq < 0) {
		pr_err("%s: irq failure\n", __func__);
		gpio_free(g_gpio_wl_host_wake);
		return -EINVAL;
	}

	/* Update the resource irq */
	brcm_wifi_resources[0].start = irq;
	brcm_wifi_resources[0].end = irq;

	return ret;
}

static int __init brcm_wlan_init(void)
{
	int rc;
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "bcm,bcm43430");
	if (!node) {
		pr_err("%s: failed to find bcm,bcm43430 node\n", __func__);
		return -EINVAL;
	}

	/* Update the of node in the wifi device */
	brcm_wifi_device.dev.of_node = node;
	/* Turn on the chip */
	rc = brcm_init_gpio_power(node);
	if (rc)
		pr_err("%s: power init failed\n", __func__);

	/* Configure the wake on lan interrupt */
	rc = brcm_init_gpio_hostwake_irq(node);
	if (rc) {
		if (g_gpio_wl_reg_on != -1)
			gpio_free(g_gpio_wl_reg_on);
		pr_err("%s: hostwake init failed\n", __func__);
		return rc;
	}

#ifdef CONFIG_DHD_USE_STATIC_BUF
	brcm_init_wlan_mem();
#endif

	rc = platform_device_register(&brcm_wifi_device);
	if (rc) {
		if (g_gpio_wl_reg_on != -1)
			gpio_free(g_gpio_wl_reg_on);
		if (g_gpio_wl_host_wake != -1)
			gpio_free(g_gpio_wl_host_wake);
		pr_err("%s: wlan init failed err=%d\n", __func__, rc);
	}

	return rc;
}
late_initcall(brcm_wlan_init);
