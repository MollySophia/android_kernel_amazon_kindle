/*
 * Copyright 2012-2014 Amazon Technologies, Inc. All Rights Reserved.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

/* Lab126 functions for mxc_epdc
 */

#include <linux/spi/spi.h>
#include <linux/proc_fs.h>
#include <linux/zlib.h>
#include <linux/vmalloc.h>
#include <linux/einkwf.h>
#include <linux/fs.h>
//#include <mach/boardid.h>

static int override_panel_settings = 0;
bool wfm_using_builtin = false; 
bool builtin_firmware = false; //whoever loading this module can use this param to load the default waveform

#define DISPLAY_UP_RIGHT   0

#ifdef DEVELOPMENT_MODE
/*
 * The override_panel_settings flag allows the user to ignore the eink panel
 * settings and instead use the ones defined in panel_get_default_info(). This
 * is needed to load the EPDC module when connected to a blank panel (though
 * it would also work with no panel at all). Make sure the default settings
 * are correct before enabling this flag or you will risk burning up the panel.
 */
module_param(override_panel_settings, int, 0);
MODULE_PARM_DESC(override_panel_settings, "Do not read settings from the panel.");
#endif // DEVELOPMENT_MODE

module_param(builtin_firmware, bool, 0);
MODULE_PARM_DESC(builtin_firmware, "request default waveform instead");

/************
 * Utility  *
 ************/

#define GUNZIP_HEAD_CRC       2
#define GUNZIP_EXTRA_FIELD    4
#define GUNZIP_ORIG_NAME      8
#define GUNZIP_COMMENT        0x10
#define GUNZIP_RESERVED       0xe0
#define GUNZIP_DEFLATED       8

unsigned char sum8(unsigned char *buf, int len);
unsigned sum32(unsigned char *buf, int len);
static unsigned long crc32(unsigned char *buf, int len);
static unsigned long update_crc(unsigned long crc, unsigned char *buf, int len);
static void make_crc_table(void);
static int gunzip(unsigned char *dst, size_t dst_len, size_t *write_len, const unsigned char *src, size_t src_len);


/************
 *   eInk   *
 ************/

#define EINK_ADDR_CHECKSUM1         0x001F  // 1 byte  (checksum of bytes 0x00-0x1E)
#define EINK_ADDR_CHECKSUM2         0x002F  // 1 byte  (checksum of bytes 0x20-0x2E)
#define EINK_WAVEFORM_FILESIZE      262144  // 256K..
#define EINK_WAVEFORM_TYPE_WJ       0x15
#define EINK_WAVEFORM_TYPE_WR       0x2B
#define EINK_WAVEFORM_TYPE_AA       0x3C
#define EINK_WAVEFORM_TYPE_AC       0x4B
#define EINK_WAVEFORM_TYPE_BD       0x4C
#define EINK_WAVEFORM_TYPE_AE       0x50
#define EINK_CHECKSUM(c1, c2)       (((c2) << 16) | (c1))


/************
 * Waveform *
 ************/

#define WF_UPD_MODES_00             0       // Test waveform
#define WF_UPD_MODES_07             7       // V220 210 dpi85Hz modes
#define WF_UPD_MODES_18             18
#define WF_UPD_MODES_19             19
#define WF_UPD_MODES_24             24
#define WF_UPD_MODES_25             25

#define WF_PROC_PARENT              "eink"
#define WF_PROC_PANEL_PARENT        "eink/panel"
#define WF_PROC_PANEL_WFM_PARENT    "eink/panel/waveform"
#define WF_PROC_WFM_PARENT          "eink/waveform"

#define WAVEFORM_VERSION_STRING_MAX 64
#define CHECKSUM_STRING_MAX         64

#define WAVEFORM_AA_VCOM_SHIFT      250000

/* Steps for voltage control in uV */
#define EPDC_VC_VPOS_STEP 12500
#define EPDC_VC_VNEG_STEP -12500
#define EPDC_VC_VDDH_STEP 12500
#define EPDC_VC_VEE_STEP  -12500
#define EPDC_VC_MAX 0x7FFF

#define EPDC_VC_VCOM_OFFSET_POS_STEP 3125
#define EPDC_VC_VCOM_OFFSET_NEG_STEP -3125
#define EPDC_VC_VCOM_OFFSET_POS_MIN 0x0000
#define EPDC_VC_VCOM_OFFSET_POS_MAX 0x0FFF
#define EPDC_VC_VCOM_OFFSET_NEG_MIN 0x8000
#define EPDC_VC_VCOM_OFFSET_NEG_MAX 0x8FFF

struct update_mode {
	unsigned char mode;
	char *name;
};

struct update_modes {
	struct update_mode init;
	struct update_mode du;
	struct update_mode gc16;
	struct update_mode gcf;
	struct update_mode gl16;
	struct update_mode glf;
	struct update_mode a2;
	struct update_mode du4;
	struct update_mode gl4;
	struct update_mode glr;
	struct update_mode glrd;
	struct update_mode gldk;
	struct update_mode glr4;
};

struct panel_addrs {
	off_t cmd_sec_addr;
	off_t waveform_addr;
	off_t pnl_info_addr;
	off_t test_sec_addr;
	size_t cmd_sec_len;
	size_t waveform_len;
	size_t pnl_info_len;
	size_t test_sec_len;
	size_t flash_end;
};

extern struct mxc_epdc_fb_data *g_fb_data;

char * wfm_name_for_mode(struct mxc_epdc_fb_data *fb_data, int mode);
char proc_msg_buf[512];

static ssize_t proc_wfm_data_read(struct file *file, char __user *buf,size_t count, loff_t *off);
static int proc_wfm_version_open(struct inode *inode, struct file *file);
static ssize_t proc_wfm_version_show(struct seq_file *m, void *v);
//static ssize_t proc_wfm_human_version_read(struct file *file, char __user *buf,size_t count, loff_t *off);
static int proc_wfm_embedded_checksum_open(struct inode *inode,
					   struct file *file);
static ssize_t proc_wfm_embedded_checksum_show(struct seq_file *m, void *v);
static int proc_wfm_computed_checksum_open(struct inode *inode,
					   struct file *file);
static ssize_t proc_wfm_computed_checksum_show(struct seq_file *m, void *v);
static int proc_wfm_info_open(struct inode *inode, struct file *file);
static ssize_t proc_wfm_info_show(struct seq_file *m, void *v);
static int proc_wfm_source_open(struct inode *inode, struct file *file);
static ssize_t proc_wfm_source_show(struct seq_file *m, void *v);
char *eink_get_wfm_human_version(struct waveform_data_header *wv_header, u8 *wf_buffer, size_t wf_buffer_len, char *str, size_t str_len);


/***********
 *  Panel  *
 ***********/

/*
 * Panel flash
 */

/* All panels must locate the waveform at the default offset. Otherwise, we
 * have no way of knowing what kind of panel is attached. Additionally, all
 * panels must be greater than or equal in size to the default flash size.
 */
#define DEFAULT_WFM_ADDR      0x00886
#define DEFAULT_FLASH_SIZE    0x40000

#define WFM_HDR_SIZE          (0x30)

#define PNL_BASE_PART_NUMBER  0x00
#define PNL_SIZE_PART_NUMBER  16

#define PNL_BASE_VCOM         0x10
#define PNL_SIZE_VCOM         5
#define PNL_SIZE_VCOM_STR     (PNL_SIZE_VCOM + 1)

#define PNL_BASE_WAVEFORM     0x20
#define PNL_SIZE_WAVEFORM     23

#define PNL_BASE_FPL          0x40
#define PNL_SIZE_FPL          3

#define PNL_BASE_BCD          0x50
#define PNL_SIZE_BCD          33
#define PNL_SIZE_BCD_STR      (PNL_SIZE_BCD + 1)
#define PNL_BCD_PREFIX_LEN    3

#define PNL_BASE_RESOLUTION   0x80
#define PNL_SIZE_RESOLUTION   16
#define PNL_SIZE_RESOLUTION_STR (PNL_SIZE_RESOLUTION + 1)

#define PNL_BASE_DIMENSIONS   0x90
#define PNL_SIZE_DIMENSIONS   32
#define PNL_SIZE_DIMENSIONS_STR (PNL_SIZE_DIMENSIONS + 1)

#define PNL_BASE_VDD          0xB0
#define PNL_SIZE_VDD          16
#define PNL_SIZE_VDD_STR      (PNL_SIZE_VDD + 1)

#define PNL_BASE_VERSION      0x300
#define PNL_SIZE_VERSION      16
#define PNL_SIZE_VERSION_STR  (PNL_SIZE_VERSION + 1)

#define PNL_CHAR_UNKNOWN      '!'

/*
 * SPI Flash API
 */

#define SFM_WRSR              0x01
#define SFM_PP                0x02
#define SFM_READ              0x03
#define SFM_WRDI              0x04
#define SFM_RDSR              0x05
#define SFM_WREN              0x06
#define SFM_FAST_READ         0x0B
#define SFM_SE                0x20
#define SFM_BE                0xD8
#define SFM_RES               0xAB
#define SFM_ID                0x9F
#define SFM_WIP_MASK          BIT(0)
#define SFM_BP0_MASK          BIT(2)
#define SFM_BP1_MASK          BIT(3)

#define PNL_PAGE_SIZE         (256)
#define PNL_SECTOR_SIZE       (1024 * 4)
#define PNL_BLOCK_SIZE        (1024 * 64)
#define PNL_SIZE              (1024 * 128)

#define PANEL_ID_UNKNOWN      "????_???_??_???"
#define PNL_SIZE_ID_STR       32

#define MXC_SPI_MAX_CHARS     28
#define SFM_READ_CMD_LEN      4
#define SFM_WRITE_CMD_LEN     4

struct panel_info {
	struct panel_addrs *addrs;
	int  vcom_uV;
	long computed_checksum;
	long embedded_checksum;
	int  version_major;
	int  version_minor;
	struct eink_waveform_info_t *waveform_info;
	char human_version[WAVEFORM_VERSION_STRING_MAX];
	char version[WAVEFORM_VERSION_STRING_MAX];
	char bcd[PNL_SIZE_BCD_STR];
	char id[PNL_SIZE_ID_STR];
	char panel_info_version[PNL_SIZE_VERSION_STR];
	char resolution[PNL_SIZE_RESOLUTION_STR];
	char dimensions[PNL_SIZE_DIMENSIONS_STR];
	char vdd[PNL_SIZE_VDD_STR];
};

static struct panel_info *panel_info_cache = NULL;

static struct update_modes *panel_get_upd_modes(struct mxc_epdc_fb_data *fb_data);
static struct imx_epdc_fb_mode * panel_choose_fbmode(struct mxc_epdc_fb_data *fb_data);

extern void epdc_iomux_config_lve(void);

/***********
 *   MXC   *
 ***********/

#define mV_to_uV(mV)        ((mV) * 1000)
#define uV_to_mV(uV)        ((uV) / 1000)
#define V_to_uV(V)          (mV_to_uV((V) * 1000))
#define uV_to_V(uV)         (uV_to_mV(uV) / 1000)

/******************************************************************************
 ******************************************************************************
 **                                                                          **
 **                            Utility Functions                             **
 **                                                                          **
 ******************************************************************************
 ******************************************************************************/

/*
 * CRC-32 algorithm from:
 *  <http://glacier.lbl.gov/cgi-bin/viewcvs.cgi/dor-test/crc32.c?rev=HEAD>
 */

/* Table of CRCs of all 8-bit messages. */
static unsigned long crc_table[256];

/* Flag: has the table been computed? Initially false. */
static int crc_table_computed = 0;

/* Make the table for a fast CRC. */
static void make_crc_table(void)
{
	unsigned long c;
	int n, k;

	for (n = 0; n < 256; n++) {
		c = (unsigned long) n;
		for (k = 0; k < 8; k++) {
			if (c & 1)
				c = 0xedb88320L ^ (c >> 1);
			else
				c = c >> 1;
		}
		crc_table[n] = c;
	}
	crc_table_computed = 1;
}

/*
 * Update a running crc with the bytes buf[0..len-1] and return
 * the updated crc. The crc should be initialized to zero. Pre- and
 * post-conditioning (one's complement) is performed within this
 * function so it shouldn't be done by the caller. Usage example:
 *
 *   unsigned long crc = 0L;
 *
 *   while (read_buffer(buffer, length) != EOF) {
 *     crc = update_crc(crc, buffer, length);
 *   }
 *   if (crc != original_crc) error();
 */
static unsigned long update_crc(unsigned long crc, unsigned char *buf, int len)
{
	unsigned long c = crc ^ 0xffffffffL;
	int n;

	if (!crc_table_computed)
		make_crc_table();
	for (n = 0; n < len; n++)
		c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);

	return c ^ 0xffffffffL;
}

/* Return the CRC of the bytes buf[0..len-1]. */
static unsigned long crc32(unsigned char *buf, int len)
{
	return update_crc(0L, buf, len);
}

/* Return the sum of the bytes buf[0..len-1]. */
unsigned sum32(unsigned char *buf, int len)
{
	unsigned c = 0;
	int n;

	for (n = 0; n < len; n++)
		c += buf[n];

	return c;
}

/* Return the sum of the bytes buf[0..len-1]. */
unsigned char sum8(unsigned char *buf, int len)
{
	unsigned char c = 0;
	int n;

	for (n = 0; n < len; n++)
		c += buf[n];

	return c;
}


/*
** This is a hack: because procfs doesn't support large write operations,
** this function gets called multiple times (in 4KB chunks). Each time a
** large write is requested, fcount is reset to some "random" value. By
** setting it to the MAGIC_TOKEN, we are able to figure out which
** invocations belong together and keep track of the offset.
*/

#define MAGIC_TOKEN 0x42975623

static int gunzip(unsigned char *dst, size_t dst_len, size_t *write_len, const unsigned char *src, size_t src_len)
{
	z_stream stream;
	int i;
	int flags;
	int ret = 0;
	static void *z_inflate_workspace = NULL;

	z_inflate_workspace = kzalloc(zlib_inflate_workspacesize(), GFP_ATOMIC);
	if (z_inflate_workspace == NULL) {
		printk(KERN_ERR "%s: error: gunzip failed to allocate workspace\n", __func__);
		return -ENOMEM;
	}

	/* skip header */
	i = 10;
	flags = src[3];
	if (src[2] != GUNZIP_DEFLATED || (flags & GUNZIP_RESERVED) != 0) {
		printk(KERN_ERR "%s: error: Bad gzipped data\n", __func__);
		ret = -EINVAL;
		goto cleanup;
	}

	if ((flags & GUNZIP_EXTRA_FIELD) != 0)
		i = 12 + src[10] + (src[11] << 8);

	if ((flags & GUNZIP_ORIG_NAME) != 0)
		while (src[i++] != 0);

	if ((flags & GUNZIP_COMMENT) != 0)
		while (src[i++] != 0);

	if ((flags & GUNZIP_HEAD_CRC) != 0)
		i += 2;

	if (i >= src_len) {
		printk(KERN_ERR "%s: error: gunzip out of data in header\n", __func__);
		ret = -EINVAL;
		goto cleanup;
	}

	stream.workspace = z_inflate_workspace;
	ret = zlib_inflateInit2(&stream, -MAX_WBITS);
	if (ret != Z_OK) {
		printk(KERN_ERR "%s: error: zlib_inflateInit2() failed (%d)\n", __func__, ret);
		goto cleanup;
	}
	stream.next_in = src + i;
	stream.avail_in = src_len - i;
	stream.next_out = dst;
	stream.avail_out = dst_len;
	ret = zlib_inflate(&stream, Z_FINISH);
	if (ret != Z_OK && ret != Z_STREAM_END) {
		printk(KERN_ERR "%s: error: zlib_inflate() failed (%d)\n", __func__, ret);
		goto cleanup;
	}
	*write_len = dst_len - stream.avail_out;
	zlib_inflateEnd(&stream);

cleanup:
	kfree(z_inflate_workspace);
	return ret;
}




/******************************************************************************
 ******************************************************************************
 **                                                                          **
 **                              eInk Functions                              **
 **                                                                          **
 ******************************************************************************
 ******************************************************************************/

unsigned long eink_get_computed_waveform_checksum(u8 *wf_buffer)
{
	unsigned long checksum = 0;

	if (wf_buffer) {
		struct waveform_data_header *header = (struct waveform_data_header *)wf_buffer;
		unsigned long filesize = header->file_length;

		if (filesize) {
			unsigned long saved_embedded_checksum;

			// Save the buffer's embedded checksum and then set it zero.
			//
			saved_embedded_checksum = header->checksum;
			header->checksum = 0;

			// Compute the checkum over the entire buffer, including
			// the zeroed-out embedded checksum area, and then restore
			// the embedded checksum.
			//
			checksum = crc32((unsigned char *)wf_buffer, filesize);
			header->checksum = saved_embedded_checksum;
		} else {
			unsigned char checksum1, checksum2;
			int start, length;

			// Checksum bytes 0..(EINK_ADDR_CHECKSUM1 - 1).
			//
			start     = 0;
			length    = EINK_ADDR_CHECKSUM1;
			checksum1 = sum8((unsigned char *)wf_buffer + start, length);

			// Checksum bytes (EINK_ADDR_CHECKSUM1 + 1)..(EINK_ADDR_CHECKSUM2 - 1).
			//
			start     = EINK_ADDR_CHECKSUM1 + 1;
			length    = EINK_ADDR_CHECKSUM2 - start;
			checksum2 = sum8((unsigned char *)wf_buffer + start, length);

			checksum  = EINK_CHECKSUM(checksum1, checksum2);
		}
	}

	return checksum;
}

void eink_get_waveform_info(u8 *wf_buffer, struct eink_waveform_info_t *info)
{
	struct waveform_data_header *header = (struct waveform_data_header *)wf_buffer;

	if (info) {
		info->waveform.version         = header->wf_version;
		info->waveform.subversion      = header->wf_subversion;
		info->waveform.type            = header->wf_type;
		info->waveform.run_type        = header->run_type;
		info->fpl.platform             = header->fpl_platform;
		info->fpl.size                 = header->panel_size;
		info->fpl.adhesive_run_number  = header->fpl_lot;
		info->waveform.mode_version    = header->mode_version;
		info->waveform.mfg_code        = header->amepd_part_number;
		info->waveform.bit_depth       = ((header->luts & 0xC) == 0x4) ? 5 : 4;
		info->waveform.vcom_shift      = header->vcom_shifted;

		if (info->waveform.type == EINK_WAVEFORM_TYPE_WJ) {
			info->waveform.tuning_bias = header->wf_revision;
		} else if (info->waveform.type == EINK_WAVEFORM_TYPE_WR) {
			info->waveform.revision    = header->wf_revision;
		} else {
			info->waveform.revision    = header->wf_revision;
			info->waveform.awv         = header->advanced_wfm_flags;
		}

		info->waveform.fpl_rate        = header->frame_rate;

		info->fpl.lot                  = header->fpl_lot;

		info->checksum                 = header->checksum;
		info->filesize                 = header->file_length;
		info->waveform.serial_number   = header->serial_number;

		/* XWIA is only 3 bytes */
		info->waveform.xwia            = header->xwia;

		if (0 == info->filesize) {
			info->checksum = EINK_CHECKSUM(header->cs1, header->cs2);
			info->waveform.parse_wf_hex  = false;
		} else {
			info->waveform.parse_wf_hex  = false;
		}

		pr_debug(   "\n"
		            " Waveform version:  0x%02X\n"
		            "       subversion:  0x%02X\n"
		            "             type:  0x%02X (v%02d)\n"
		            "         run type:  0x%02X\n"
		            "     mode version:  0x%02X\n"
		            "      tuning bias:  0x%02X\n"
		            "       frame rate:  0x%02X\n"
		            "       vcom shift:  0x%02X\n"
		            "        bit depth:  0x%02X\n"
		            "\n"
		            "     FPL platform:  0x%02X\n"
		            "              lot:  0x%04X\n"
		            "             size:  0x%02X\n"
		            " adhesive run no.:  0x%02X\n"
		            "\n"
		            "        File size:  0x%08lX\n"
		            "         Mfg code:  0x%02X\n"
		            "       Serial no.:  0x%08lX\n"
		            "         Checksum:  0x%08lX\n",

		            info->waveform.version,
		            info->waveform.subversion,
		            info->waveform.type,
		            info->waveform.revision,
		            info->waveform.run_type,
		            info->waveform.mode_version,
		            info->waveform.tuning_bias,
		            info->waveform.fpl_rate,
		            info->waveform.vcom_shift,
		            info->waveform.bit_depth,

		            info->fpl.platform,
		            info->fpl.lot,
		            info->fpl.size,
		            info->fpl.adhesive_run_number,

		            info->filesize,
		            info->waveform.mfg_code,
		            info->waveform.serial_number,
		            info->checksum);
    }
}

char *eink_get_wfm_version(u8 *wf_buffer, char *version_string, size_t version_string_len)
{
	struct eink_waveform_info_t info;

	eink_get_waveform_info(wf_buffer, &info);

	// Build up a waveform version string in the following way:
	//
	//      <FPL PLATFORM>_<RUN TYPE>_<FPL LOT NUMBER>_<FPL SIZE>_
	//      <WF TYPE><WF VERSION><WF SUBVERSION>_
	//      (<WAVEFORM REV>|<TUNING BIAS>)_<MFG CODE>_<S/N>_<FRAME RATE>_MODEVERSION
	snprintf(version_string,
	        version_string_len,
	        "%02x_%02x_%04x_%02x_%02x%02x%02x_%02x_%02x_%08x_%02x_%02x",
	        info.fpl.platform,
	        info.waveform.run_type,
	        info.fpl.lot,
	        info.fpl.size,
	        info.waveform.type,
	        info.waveform.version,
	        info.waveform.subversion,
	        (info.waveform.type == EINK_WAVEFORM_TYPE_WR) ? info.waveform.revision : info.waveform.tuning_bias,
	        info.waveform.mfg_code,
	        (unsigned int) info.waveform.serial_number,
	        info.waveform.fpl_rate,
	        info.waveform.mode_version);

	return version_string;
}




/******************************************************************************
 ******************************************************************************
 **                                                                          **
 **                            Waveform Functions                            **
 **                                                                          **
 ******************************************************************************
 ******************************************************************************/

// Test waveform only
struct update_modes panel_mode_00 = {
		.init = { .mode = 0, .name = "init" },
		.du   = { .mode = 1, .name = "du" },
		.gc16 = { .mode = 2, .name = "gc16_fast" },
		.gcf  = { .mode = 2, .name = "gc16_fast" },
		.gl16 = { .mode = 3, .name = "gl16_fast" },
		.glf  = { .mode = 3, .name = "gl16_fast" },
		.a2   = { .mode = 6, .name = "a2" },
		.du4  = { .mode = 7, .name = "gl4" },
		.gl4  = { .mode = 7, .name = "gl4" },
		.glr  = { .mode = 4, .name = "reagl" },
		.glrd = { .mode = 5, .name = "reagld" },
		.gldk = { .mode = 3, .name = "gldk" },
		.glr4 = { .mode = 8, .name = "reagl4" },

};

struct update_modes panel_mode_07 = {
		.init = { .mode = 0, .name = "init" },
		.du   = { .mode = 1, .name = "du" },
		.gc16 = { .mode = 2, .name = "gc16" },
		.gcf  = { .mode = 3, .name = "gc16_fast" },
		.gl16 = { .mode = 5, .name = "gl16" },
		.glf  = { .mode = 6, .name = "gl16_fast" },
		.a2   = { .mode = 4, .name = "a2" },
		.du4  = { .mode = 2, .name = "gc16" },
		.gl4  = { .mode = 6, .name = "gl16_fast" },
		.glr  = { .mode = 6, .name = "gl16_fast" },
		.glrd = { .mode = 3, .name = "gc16_fast" },
		.gldk = { .mode = 3, .name = "gc16_fast" },
		.glr4 = { .mode = 6, .name = "gl16_fast" },
};

struct update_modes panel_mode_18 = {
		.init = { .mode = 0, .name = "init" },
		.du   = { .mode = 1, .name = "du" },
		.gc16 = { .mode = 2, .name = "gc16" },
		.gcf  = { .mode = 3, .name = "gc16_fast" },
		.gl16 = { .mode = 5, .name = "gl16" },
		.glf  = { .mode = 6, .name = "gl16_fast" },
		.a2   = { .mode = 4, .name = "a2" },
		.du4  = { .mode = 7, .name = "du4" },
		.gl4  = { .mode = 7, .name = "du4" },
		.glr  = { .mode = 6, .name = "gl16_fast" },
		.glrd = { .mode = 3, .name = "gc16_fast" },
		.gldk = { .mode = 3, .name = "gc16_fast" },
		.glr4 = { .mode = 6, .name = "gl16_fast" },
};

struct update_modes panel_mode_19 = {
		.init = { .mode = 0, .name = "init" },
		.du   = { .mode = 1, .name = "du" },
		.gc16 = { .mode = 2, .name = "gc16_fast" },
		.gcf  = { .mode = 2, .name = "gc16_fast" },
		.gl16 = { .mode = 3, .name = "gl16_fast" },
		.glf  = { .mode = 3, .name = "gl16_fast" },
		.a2   = { .mode = 4, .name = "a2" },
		.du4  = { .mode = 2, .name = "gc16_fast" },
		.gl4  = { .mode = 3, .name = "gl16_fast" },
		.glr  = { .mode = 3, .name = "gl16_fast" },
		.glrd = { .mode = 2, .name = "gc16_fast" },
		.gldk = { .mode = 2, .name = "gc16_fast" },
		.glr4 = { .mode = 3, .name = "gl16_fast" },
};

struct update_modes panel_mode_24 = {
		.init = { .mode = 0, .name = "init" },
		.du   = { .mode = 1, .name = "du" },
		.gc16 = { .mode = 2, .name = "gc16_fast" },
		.gcf  = { .mode = 2, .name = "gc16_fast" },
		.gl16 = { .mode = 3, .name = "gl16_fast" },
		.glf  = { .mode = 3, .name = "gl16_fast" },
		.a2   = { .mode = 6, .name = "a2" },
		.du4  = { .mode = 2, .name = "gc16_fast" },
		.gl4  = { .mode = 3, .name = "gl16_fast" },
		.glr  = { .mode = 4, .name = "reagl" },
		.glrd = { .mode = 5, .name = "reagld" },
		.gldk = { .mode = 2, .name = "gc16_fast" },
		.glr4 = { .mode = 4, .name = "reagl" },
};

struct update_modes panel_mode_25 = {
		.init = { .mode = 0, .name = "init" },
		.du   = { .mode = 1, .name = "du" },
		.gc16 = { .mode = 2, .name = "gc16_fast" },
		.gcf  = { .mode = 2, .name = "gc16_fast" },
		.gl16 = { .mode = 3, .name = "gl16_fast" },
		.glf  = { .mode = 3, .name = "gl16_fast" },
		.a2   = { .mode = 6, .name = "a2" },
		.du4  = { .mode = 7, .name = "du4" },
		.gl4  = { .mode = 7, .name = "du4" },
		.glr  = { .mode = 4, .name = "reagl" },
		.glrd = { .mode = 5, .name = "reagld" },
		.gldk = { .mode = 2, .name = "gc16_fast" },
		.glr4 = { .mode = 4, .name = "reagl" },
};

struct panel_addrs waveform_addrs_WJ = {
	.cmd_sec_addr  = 0x00000,
	.waveform_addr = 0x00886,
	.pnl_info_addr = 0x30000,
	.test_sec_addr = 0x3E000,
	.cmd_sec_len   = 0x00886 - 0x00000,
	.waveform_len  = 0x30000 - 0x00886,
	.pnl_info_len  = 0x3E000 - 0x30000,
	.test_sec_len  = 0x40000 - 0x3E000,
	.flash_end     = 0x40000,
};


struct panel_addrs waveform_addrs_WR = {
	.cmd_sec_addr  = 0x00000,
	.waveform_addr = 0x00886,
	.pnl_info_addr = 0x30000,
	.test_sec_addr = 0x3E000,
	.cmd_sec_len   = 0x00886 - 0x00000,
	.waveform_len  = 0x30000 - 0x00886,
	.pnl_info_len  = 0x3E000 - 0x30000,
	.test_sec_len  = 0x40000 - 0x3E000,
	.flash_end     = 0x40000,
};

struct panel_addrs waveform_addrs_AA_AC_AE_BD = {
	.cmd_sec_addr  = 0x00000,
	.waveform_addr = 0x00886,
	.pnl_info_addr = 0x70000,
	.test_sec_addr = 0x7E000,
	.cmd_sec_len   = 0x00886 - 0x00000,
	.waveform_len  = 0x70000 - 0x00886,
	.pnl_info_len  = 0x7E000 - 0x70000,
	.test_sec_len  = 0x80000 - 0x7E000,
	.flash_end     = 0x80000,
};

static const struct file_operations proc_wfm_data_fops = {
 .owner = THIS_MODULE,
 .read  = proc_wfm_data_read,
 .write = NULL,
};

static const struct file_operations proc_wfm_version_fops = {
	.owner = THIS_MODULE,
	.open  = proc_wfm_version_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations proc_wfm_embedded_checksum_fops = {
	.owner = THIS_MODULE,
	.open  = proc_wfm_embedded_checksum_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations proc_wfm_computed_checksum_fops = {
	.owner = THIS_MODULE,
	.open  = proc_wfm_computed_checksum_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations proc_wfm_info_fops = {
	.owner = THIS_MODULE,
	.open  = proc_wfm_info_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations proc_wfm_source_fops = {
	.owner = THIS_MODULE,
	.open = proc_wfm_source_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};


struct wf_proc_dir_entry {
	const char *name;
	mode_t mode;
//	read_proc_t *read_proc;
//	write_proc_t *write_proc;
	const struct file_operations *fops;
	struct proc_dir_entry *proc_entry;
};

static struct proc_dir_entry *proc_wf_parent            = NULL;


static struct proc_dir_entry *proc_wf_waveform_parent   = NULL;

static struct proc_dir_entry *create_wf_proc_entry(const char *name,
                                                   mode_t mode,
                                                   struct proc_dir_entry *parent,
						const struct file_operations* fops)
{
	struct proc_dir_entry *wf_proc_entry = proc_create(name, mode, parent,fops);
	return wf_proc_entry;
}

static inline void remove_wf_proc_entry(const char *name,
                                        struct proc_dir_entry *entry,
                                        struct proc_dir_entry *parent)
{
	if (entry) {
		remove_proc_entry(name, parent);
		entry = NULL;
	}
}

char * wfm_name_for_mode(struct mxc_epdc_fb_data *fb_data, int mode)
{
	struct update_modes *wf_upd_modes = panel_get_upd_modes(fb_data);
	if (mode == wf_upd_modes->init.mode) return wf_upd_modes->init.name;
	if (mode == wf_upd_modes->du.mode)   return wf_upd_modes->du.name;
	if (mode == wf_upd_modes->gc16.mode) return wf_upd_modes->gc16.name;
	if (mode == wf_upd_modes->gcf.mode)  return wf_upd_modes->gcf.name;
	if (mode == wf_upd_modes->gl16.mode) return wf_upd_modes->gl16.name;
	if (mode == wf_upd_modes->glf.mode)  return wf_upd_modes->glf.name;
	if (mode == wf_upd_modes->a2.mode)   return wf_upd_modes->a2.name;
	if (mode == wf_upd_modes->du4.mode)  return wf_upd_modes->du4.name;
	if (mode == wf_upd_modes->glr.mode)  return wf_upd_modes->glr.name;
	if (mode == wf_upd_modes->glrd.mode) return wf_upd_modes->glrd.name;
	if (mode == wf_upd_modes->gldk.mode) return wf_upd_modes->gldk.name;
	if (mode == wf_upd_modes->gl4.mode)  return wf_upd_modes->gl4.name;
	if (mode == wf_upd_modes->glr4.mode) return wf_upd_modes->glr4.name;
	if (mode == WAVEFORM_MODE_AUTO)      return "auto";
	return NULL;
}

static ssize_t proc_wfm_data_read(struct file *file, char __user *buf,size_t count, loff_t *off)
{
	ssize_t length = 0;
	size_t waveform_size = g_fb_data->waveform_buffer_size + g_fb_data->wv_header_size;


	if ((*off < waveform_size) && count) {
		if (*off < g_fb_data->wv_header_size) {
			length = min((ssize_t)(g_fb_data->wv_header_size - *off), (ssize_t)count);
			length=(copy_to_user(buf, (u8 *)g_fb_data->wv_header + *off, length))?-EFAULT:length;
		} else {
			length = min((ssize_t)(waveform_size - *off), (ssize_t)count);
			length= (copy_to_user(buf, (u8 *)g_fb_data->waveform_buffer_virt + (*off - g_fb_data->wv_header_size), length))?-EFAULT:length;
		}
		*off+=length;
	} else {
		*off=0;
	}

	pr_debug("%s: off=%ld flsz=%ld count=%d \n", __func__, (long int)*off, (long int)length, (int)count);

	return length;
}


static int proc_wfm_version_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_wfm_version_show, NULL);
}

static ssize_t proc_wfm_version_show(struct seq_file *m, void *v)
{
	char version_str[WAVEFORM_VERSION_STRING_MAX] = "";

	seq_printf(m, "%s\n",
		   eink_get_wfm_version((u8 *)g_fb_data->wv_header,
					version_str,
					WAVEFORM_VERSION_STRING_MAX));
	return 0;
}

static int proc_wfm_human_version_read(struct seq_file *file, void *data)
{
	char version_str[WAVEFORM_VERSION_STRING_MAX] = "";
	return seq_printf(file, "%s\n",
		eink_get_wfm_human_version(g_fb_data->wv_header, (u8 *)g_fb_data->waveform_buffer_virt, sizeof(struct waveform_data_header),
			version_str, WAVEFORM_VERSION_STRING_MAX));
}

static int proc_wfm_human_version_open(struct inode *inode, struct file *file) {
        return single_open(file, &proc_wfm_human_version_read, NULL);
}

static const struct file_operations proc_wfm_human_version_fops = {
// .owner = THIS_MODULE,
// .read  = proc_wfm_human_version_read,
// .write = NULL,
     .owner   = THIS_MODULE,
     .open    = proc_wfm_human_version_open,
     .read    = seq_read,
     .release  = single_release,

};

static struct wf_proc_dir_entry wfm_proc_entries[] = {
	// TODO ALEX Write
	{ "data", S_IRUGO, &proc_wfm_data_fops,  NULL },
	{ "version", S_IRUGO, &proc_wfm_version_fops,  NULL },
	{ "human_version", S_IRUGO, &proc_wfm_human_version_fops,  NULL },
	{ "embedded_checksum", S_IRUGO, &proc_wfm_embedded_checksum_fops,  NULL },
	// TODO ALEX computed checksum is not correct
	{ "computed_checksum", S_IRUGO, &proc_wfm_computed_checksum_fops,  NULL },
	{ "info", S_IRUGO, &proc_wfm_info_fops,  NULL },
	{ "source", S_IRUGO, &proc_wfm_source_fops,  NULL },
};

static int proc_wfm_embedded_checksum_open(struct inode *inode,
					   struct file *file)
{
	return single_open(file, proc_wfm_embedded_checksum_show, NULL);
}

static ssize_t proc_wfm_embedded_checksum_show(struct seq_file *m, void *v)
{
	/* TODO */
	return 0;
}

static int proc_wfm_computed_checksum_open(struct inode *inode,
					   struct file *file)
{
	return single_open(file, proc_wfm_computed_checksum_show, NULL);
}

static ssize_t proc_wfm_computed_checksum_show(struct seq_file *m, void *v)
{
	/* TODO ALEX, Note: The computed checksum is not accurate */
	return 0;
}

static int proc_wfm_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_wfm_info_show, NULL);
}

static ssize_t proc_wfm_info_show(struct seq_file *m, void *v)
{
	struct eink_waveform_info_t info;

	eink_get_waveform_info((u8 *)g_fb_data->wv_header, &info);
	seq_printf(m, " Waveform version:  0x%02X\n"
		   "       subversion:  0x%02X\n"
		   "             type:  0x%02X (v%02d)\n"
		   "         run type:  0x%02X\n"
		   "     mode version:  0x%02X\n"
		   "      tuning bias:  0x%02X\n"
		   "       frame rate:  0x%02X\n"
		   "       vcom shift:  0x%02X\n"
		   "        bit depth:  0x%02X\n"
		   "\n"
		   "     FPL platform:  0x%02X\n"
		   "              lot:  0x%04X\n"
		   "             size:  0x%02X\n"
		   " adhesive run no.:  0x%02X\n"
		   "\n"
		   "        File size:  0x%08lX\n"
		   "         Mfg code:  0x%02X\n"
		   "       Serial no.:  0x%08lX\n"
		   "         Checksum:  0x%08lX\n",
		   info.waveform.version,
		   info.waveform.subversion,
		   info.waveform.type,
		   info.waveform.revision,
		   info.waveform.run_type,
		   info.waveform.mode_version,
		   info.waveform.tuning_bias,
		   info.waveform.fpl_rate,
		   info.waveform.vcom_shift,
		   info.waveform.bit_depth,
		   info.fpl.platform,
		   info.fpl.lot,
		   info.fpl.size,
		   info.fpl.adhesive_run_number,
		   info.filesize,
		   info.waveform.mfg_code,
		   info.waveform.serial_number,
		   info.checksum);
	return 0;
}

static int proc_wfm_source_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", (wfm_using_builtin ? "built-in" : "stored"));
	return 0;
}

static int proc_wfm_source_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_wfm_source_show, NULL);
}

/*
 * Decode panel data
 */

enum panel_data_characters {
	zero = 0x0, one, two, three, four, five, six, seven, eight, nine,
	underline = 0x0a, dot = 0x0b, negative = 0x0c,
	_a = 0xcb, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n,
	           _o, _p, _q, _r, _s, _t, _u, _v, _w, _x, _y, _z,

	_A = 0xe5, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N,
	           _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y, _Z
};

struct fbmode_override {
	char *barcode_prefix;
	int vmode_index;
	int vddh;
	int lve;
};

static struct update_modes *panel_get_upd_modes(struct mxc_epdc_fb_data *fb_data)
{
	unsigned char wf_upd_mode_version;
	struct update_modes *wf_upd_modes;

	struct eink_waveform_info_t info;
	eink_get_waveform_info((u8 *)fb_data->wv_header, &info);

	switch (info.waveform.mode_version) {
	case WF_UPD_MODES_00:
		wf_upd_mode_version = WF_UPD_MODES_00;
		wf_upd_modes = &panel_mode_00;
		break;
	case WF_UPD_MODES_07:
		wf_upd_mode_version = WF_UPD_MODES_07;
		wf_upd_modes = &panel_mode_07;
		break;
	case WF_UPD_MODES_18:
		wf_upd_mode_version = WF_UPD_MODES_18;
		wf_upd_modes = &panel_mode_18;
		break;
	case WF_UPD_MODES_19:
		wf_upd_mode_version = WF_UPD_MODES_19;
		wf_upd_modes = &panel_mode_19;
	case WF_UPD_MODES_24:
		wf_upd_mode_version = WF_UPD_MODES_24;
		wf_upd_modes = &panel_mode_24;
		break;
	case WF_UPD_MODES_25:
		wf_upd_mode_version = WF_UPD_MODES_25;
		wf_upd_modes = &panel_mode_25;
		break;
	default:
		wf_upd_mode_version = WF_UPD_MODES_07;
		wf_upd_modes = &panel_mode_07;
		printk(KERN_ERR "%s: Unknown waveform mode. Using MODE_07!", __func__);
	}

	return wf_upd_modes;
}

static struct imx_epdc_fb_mode * panel_choose_fbmode(struct mxc_epdc_fb_data *fb_data)
{
	return &fb_data->pdata->epdc_mode[0];
}

char *eink_get_wfm_human_version(struct waveform_data_header *wv_header, u8 *wf_buffer, size_t wf_buffer_len, char *str, size_t str_len)
{
	int wfm_mode_count;
	int len;

	u64 longOffset;  // Address of the first waveform
	u64 xwiOffset;
	u8 *waveform_xwi_buffer = NULL;
	u8 waveform_xwi_len = 0;
	wfm_mode_count = wv_header->mc + 1;

	longOffset = __le64_to_cpu(((uint64_t *)wf_buffer)[0]);
	if (longOffset <= (sizeof(u64) * wfm_mode_count))
		goto error;

	if (wv_header->advanced_wfm_flags > 3)
		goto error;

	xwiOffset = __le64_to_cpu(((uint64_t *)wf_buffer)[wfm_mode_count + wv_header->advanced_wfm_flags]);
	waveform_xwi_len = wf_buffer[xwiOffset];
	waveform_xwi_buffer = wf_buffer + xwiOffset + 1;

	len = ((str_len - 1) < waveform_xwi_len) ? str_len - 1 : waveform_xwi_len;
	memmove(str, waveform_xwi_buffer, len);
	str[len] = '\0';
	return str;

error:
	snprintf(str, str_len, "?????");
	return str;
}

/******
 * proc entries
 */


/******************************************************************************
 ******************************************************************************
 **                                                                          **
 **                              MXC Functions                               **
 **                                                                          **
 ******************************************************************************
 ******************************************************************************/


int mxc_epdc_do_panel_init(struct mxc_epdc_fb_data *fb_data)
{
	struct fb_var_screeninfo tmpvar;

	// Say that we want to switch into Y8 mode:  This is where the INIT waveform
	// update occurs.
	//
	tmpvar = fb_data->info.var;
	tmpvar.bits_per_pixel = 8;
	tmpvar.grayscale = GRAYSCALE_8BIT;
	tmpvar.activate = FB_ACTIVATE_FORCE | FB_ACTIVATE_NOW ;

	// Say that we want to switch to portrait mode.
	//
	if(DISPLAY_UP_RIGHT) {
		tmpvar.rotate = FB_ROTATE_UR;
	} else {
		tmpvar.rotate = FB_ROTATE_CCW;
	}
	tmpvar.activate = FB_ACTIVATE_FORCE | FB_ACTIVATE_NOW ;
	fb_set_var(&(fb_data->info), &tmpvar);
	return 0;
}

/* previously this is read from panel flash and send to the driver. 
 * As the removal of panel flash this is hard coded and control by 
 * display team
 * */
static void set_waveform_modes(struct mxc_epdc_fb_data *fb_data)
{
	struct mxcfb_waveform_modes waveform_modes;
	
	waveform_modes.mode_init      = 0;
	waveform_modes.mode_du        = 1;
	waveform_modes.mode_gc4       = 2;
	waveform_modes.mode_gc8       = 2;
	waveform_modes.mode_gc16      = 2;
	waveform_modes.mode_gc16_fast = 2;
	waveform_modes.mode_gc32      = 2;
	waveform_modes.mode_gl16      = 3;
	waveform_modes.mode_gl16_fast = 3;
	waveform_modes.mode_a2        = 6;
	waveform_modes.mode_du4       = 7;
	waveform_modes.mode_reagl     = 4;
	waveform_modes.mode_reagld    = 5;
	waveform_modes.mode_gl4       = 0;
	waveform_modes.mode_gl16_inv  = 0;

	mxc_epdc_fb_set_waveform_modes(&waveform_modes, (struct fb_info *) fb_data);
}

int mxc_epdc_waveform_init(struct mxc_epdc_fb_data *fb_data)
{
	int i;
	int sz;

	set_waveform_modes(fb_data);

	proc_wf_parent = proc_mkdir(WF_PROC_PARENT,NULL);
	if (proc_wf_parent) {

		proc_wf_waveform_parent = proc_mkdir(WF_PROC_WFM_PARENT,NULL); 
		if (proc_wf_waveform_parent) {
			sz = ARRAY_SIZE(wfm_proc_entries);

			for (i = 0; i < sz; i++) {
				wfm_proc_entries[i].proc_entry = create_wf_proc_entry(
					wfm_proc_entries[i].name,
					wfm_proc_entries[i].mode,
					proc_wf_waveform_parent,
					wfm_proc_entries[i].fops);
			}
		}
	}

	return 0;
}

void mxc_epdc_waveform_done(struct mxc_epdc_fb_data *fb_data)
{
	int i, sz;

	if (panel_info_cache) {
		kfree(panel_info_cache->waveform_info);
		kfree(panel_info_cache);
		panel_info_cache = NULL;
	}

	if (proc_wf_parent) {
		if (proc_wf_waveform_parent) {
			sz = ARRAY_SIZE(wfm_proc_entries);

			for (i = 0; i < sz; i++) {
				remove_wf_proc_entry(wfm_proc_entries[i].name,
				                     wfm_proc_entries[i].proc_entry,
				                     proc_wf_waveform_parent);
			}

			remove_wf_proc_entry(WF_PROC_WFM_PARENT, proc_wf_waveform_parent, NULL);
		}

		remove_wf_proc_entry(WF_PROC_PARENT, proc_wf_parent, NULL);
	}
}
