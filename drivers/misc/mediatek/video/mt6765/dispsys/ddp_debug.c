/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#define LOG_TAG "DEBUG"

#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
/* #include "mt-plat/aee.h" */
#include "disp_assert_layer.h"
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/time.h>

#include "disp_drv_ddp.h"

#include "ddp_debug.h"
#include "ddp_reg.h"
#include "ddp_drv.h"
#include "ddp_wdma.h"
#include "ddp_wdma_ex.h"
#include "ddp_hal.h"
#include "ddp_path.h"
#include "ddp_color.h"
#include "ddp_aal.h"
#include "ddp_pwm.h"
#include <ddp_od.h>
#include "ddp_dither.h"
#include "ddp_info.h"
#include "ddp_dsi.h"
#include "ddp_rdma.h"
#include "ddp_rdma_ex.h"
#include "ddp_manager.h"
#include "ddp_log.h"
#include "ddp_met.h"
#include "display_recorder.h"
#include "disp_session.h"
#include "disp_lowpower.h"
#include "disp_drv_log.h"
#include "disp_recovery.h"
#include "disp_cust.h"

static struct dentry *debugfs;
static struct dentry *debugDir;


static struct dentry *debugfs_dump;

static const long int DEFAULT_LOG_FPS_WND_SIZE = 30;
static int debug_init;


unsigned char pq_debug_flag;
unsigned char aal_debug_flag;

static unsigned int dbg_log_level;
static unsigned int irq_log_level;
static unsigned int dump_to_buffer;

static int dbg_force_roi;
static int dbg_partial_x;
static int dbg_partial_y;
static int dbg_partial_w;
static int dbg_partial_h;
static int dbg_partial_statis;

unsigned int gUltraEnable = 1;
static char STR_HELP[] =
	"USAGE:\n"
	"       echo [ACTION]>/d/dispsys\n"
	"ACTION:\n"
	"       regr:addr\n              :regr:0xf400c000\n"
	"       regw:addr,value          :regw:0xf400c000,0x1\n"
	"       dbg_log:0|1|2            :0 off, 1 dbg, 2 all\n"
	"       irq_log:0|1              :0 off, !0 on\n"
	"       met_on:[0|1],[0|1],[0|1] :fist[0|1]on|off,other [0|1]direct|decouple\n"
	"       backlight:level\n"
	"       dump_aal:arg\n"
	"       mmp\n"
	"       dump_reg:moduleID\n dump_path:mutexID\n  dpfd_ut1:channel\n";


/* ------------------------------------------------------------------------- */
/* Command Processor */
/* ------------------------------------------------------------------------- */
static int low_power_cust_mode = LP_CUST_DISABLE;
static unsigned int vfp_backup;

char para_lcd_reg[16] = {0};
int lcd_reg_cmd = 0;
int lcd_reg_size = 0;

int get_lp_cust_mode(void)
{
	return low_power_cust_mode;
}

void backup_vfp_for_lp_cust(unsigned int vfp)
{
	vfp_backup = vfp;
}

unsigned int get_backup_vfp(void)
{
	return vfp_backup;
}

void _ddic_test_read(void)
{
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int ret_dlen = 0;

	struct dsi_cmd_desc cmd_tab[3];

	memset(&cmd_tab, 0, 3 * sizeof(struct dsi_cmd_desc));

	/*read display power mode*/
	cmd_tab[0].dtype = 0x0A;
	cmd_tab[0].payload = vmalloc(4 * sizeof(unsigned char));
	memset(cmd_tab[0].payload, 0, 4);
	cmd_tab[0].dlen = 4;


	/*read ID2 Value*/
	cmd_tab[1].dtype = 0xDB;
	cmd_tab[1].payload = vmalloc(4 * sizeof(unsigned char));
	memset(cmd_tab[1].payload, 0, 4);
	cmd_tab[1].dlen = 4;

	/*read display id*/
	cmd_tab[2].dtype = 0x04;
	cmd_tab[2].payload = vmalloc(4 * sizeof(unsigned char));
	memset(cmd_tab[2].payload, 0, 4);
	cmd_tab[2].dlen = 4;


	do_lcm_vdo_lp_read(cmd_tab, 3);

	for (i = 0; i < 3; i++) {
		ret_dlen = cmd_tab[i].dlen;
		DISPMSG("read lcm addr:0x%x--dlen:%d\n",
		cmd_tab[i].dtype, ret_dlen);
		for (j = 0; j < ret_dlen; j++) {
			DISPMSG("read lcm addr:0x%x--byte:%d,val:0x%x\n",
			cmd_tab[i].dtype, j, *(cmd_tab[i].payload + j));
		}
	}

	for (i = 0; i < 3; i++)
		vfree(cmd_tab[i].payload);
}

void _ddic_test_read_v1(void)
{
	unsigned int j = 0;
	unsigned int ret_dlen = 0;
	int ret;
	struct dsi_cmd_desc *cmd_tab = vmalloc(sizeof(struct dsi_cmd_desc));

	DISPMSG("%s start +\n", __func__);

	/*read display power mode, normal value = 0x9c*/
	cmd_tab->dtype = 0x0A;
	cmd_tab->payload = vmalloc(4 * sizeof(unsigned char));
	memset(cmd_tab->payload, 0, 4);
	cmd_tab->dlen = 4;
	cmd_tab->cmd = 0x06;

	#if 0
	/*read ID2 Value*/
	cmd_tab->dtype = 0xDB;
	cmd_tab->payload = vmalloc(4 * sizeof(unsigned char));
	memset(cmd_tab->payload, 0, 4);
	cmd_tab->dlen = 12;
	cmd_tab->cmd = 0x06;

	/*read display id*/
	cmd_tab->dtype = 0x04;
	cmd_tab->payload = vmalloc(4 * sizeof(unsigned char));
	memset(cmd_tab->payload, 0, 4);
	cmd_tab->dlen = 4;
	cmd_tab->cmd = 0x06;
	#endif

	ret = do_lcm_vdo_lp_read_v1(cmd_tab);
	if (ret == -1) {
		DISPERR("do_lcm_vdo_lp_read_v1 error\n");
		goto  done;
	}

	ret_dlen = cmd_tab->dlen;
	DISPMSG("read lcm addr:0x%x--dlen:%d\n",
		cmd_tab->dtype, ret_dlen);
	for (j = 0; j < ret_dlen; j++) {
		DISPMSG("read lcm addr:0x%x--byte:%d,val:0x%x\n",
			cmd_tab->dtype, j, *(cmd_tab->payload + j));
	}

done:
	vfree(cmd_tab->payload);
	DISPMSG("%s end -\n", __func__);
}
void _ddic_test_write(void)
{
	unsigned int i;

	struct dsi_cmd_desc cmd_tab[5];

	for (i = 0; i < 5; i++) {
		cmd_tab[i].payload = vmalloc(5*sizeof(char));
		cmd_tab[i].vc = 0;
		cmd_tab[i].link_state = 1;
	}
	cmd_tab[0].dtype = 0xB6;
	cmd_tab[1].dtype = 0xC8;
	cmd_tab[2].dtype = 0xF0;
	cmd_tab[3].dtype = 0xB0;
	cmd_tab[4].dtype = 0xB1;

	cmd_tab[0].dlen = 1;
	cmd_tab[1].dlen = 1;
	cmd_tab[2].dlen = 5;
	cmd_tab[3].dlen = 2;
	cmd_tab[4].dlen = 2;

	*(cmd_tab[0].payload) = 0x01;
	*(cmd_tab[1].payload) = 0x83;
	(cmd_tab[2].payload)[0] = 0x55;
	(cmd_tab[2].payload)[1] = 0xAA;
	(cmd_tab[2].payload)[2] = 0x52;
	(cmd_tab[2].payload)[3] = 0x08;
	(cmd_tab[2].payload)[4] = 0x01;
	(cmd_tab[3].payload)[0] = 0x0F;
	(cmd_tab[3].payload)[1] = 0x0F;
	(cmd_tab[4].payload)[0] = 0x0F;
	(cmd_tab[4].payload)[1] = 0x0F;

	do_lcm_vdo_lp_write(cmd_tab, 5);

	for (i = 0; i < 5; i++)
		vfree(cmd_tab[i].payload);

}

void _ddic_test_read_write(void)
{
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int ret_dlen = 0;
	struct dsi_cmd_desc read_tab[3];
	struct dsi_cmd_desc write_table1[3];

	memset(&read_tab, 0, 3 * sizeof(struct dsi_cmd_desc));
	read_tab[0].dtype = 0x52;
	read_tab[1].dtype = 0x54;
	read_tab[2].dtype = 0x5F;

	for (i = 0; i < 3; i++) {
		write_table1[i].payload = vmalloc(sizeof(unsigned char));
		write_table1[i].vc = 0;
		write_table1[i].dlen = 1;
		write_table1[i].link_state = 1;

		read_tab[i].payload = vmalloc(4 * sizeof(unsigned char));
		memset(read_tab[i].payload, 0, 4);
		read_tab[i].dlen = 4;
	}

	write_table1[0].dtype = 0x51;
	write_table1[1].dtype = 0x53;
	write_table1[2].dtype = 0x5E;

	*(write_table1[0].payload) = 0xFE;
	*(write_table1[1].payload) = 0xff;
	*(write_table1[2].payload) = 0x45;

	do_lcm_vdo_lp_write(write_table1, 3);
	do_lcm_vdo_lp_read(read_tab, 3);

	for (i = 0; i < 3; i++) {
		ret_dlen = read_tab[i].dlen;
		DDPMSG("read lcm addr:0x%x--dlen:%d\n",
		read_tab[i].dtype, ret_dlen);
		for (j = 0; j < ret_dlen; j++) {
			DDPMSG("read lcm addr:0x%x--byte:%d,val:0x%x\n",
			read_tab[i].dtype, j, *(read_tab[i].payload + j));
		}
	}

	for (i = 0; i < 3; i++) {
		vfree(read_tab[i].payload);
		vfree(write_table1[i].payload);
	}

}

static char dbg_buf[2048];

static void process_dbg_opt(const char *opt)
{
	char *buf = dbg_buf + strlen(dbg_buf);
	/* static disp_session_config config; */
	int ret;
	int buf_size_left = ARRAY_SIZE(dbg_buf) - strlen(dbg_buf) - 10;

#ifdef CONFIG_MTK_ENG_BUILD
	if (strncmp(opt, "get_reg", 7) == 0) {
		unsigned long pa;
		unsigned int *va;

		ret = sscanf(opt, "get_reg:0x%lx\n", &pa);
		if (ret != 1) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}
		va = ioremap(pa, 4);
		DDPMSG("get_reg: 0x%lx = 0x%08X\n", pa, DISP_REG_GET(va));
		snprintf(buf, buf_size_left, "get_reg: 0x%lx = 0x%08X\n",
			pa, DISP_REG_GET(va));
		iounmap(va);
		return;
	}

	if (strncmp(opt, "set_reg", 7) == 0) {
		unsigned long pa;
		unsigned int *va, val;

		ret = sscanf(opt, "set_reg:0x%lx,0x%x\n", &pa, &val);
		if (ret != 2) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}
		va = ioremap(pa, 4);
		DISP_CPU_REG_SET(va, val);
		DDPMSG("set_reg: 0x%lx = 0x%08X(0x%x)\n",
			pa, DISP_REG_GET(va), val);
		snprintf(buf, buf_size_left, "set_reg: 0x%lx = 0x%08X(0x%x)\n",
			pa, DISP_REG_GET(va), val);
		iounmap(va);
		return;
	}
#endif

	if (strncmp(opt, "regr:", 5) == 0) {
		char *p = (char *)opt + 5;
		unsigned long addr;

		ret = kstrtoul(p, 16, &addr);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		if (is_reg_addr_valid(1, addr)) {
			unsigned int regVal = DISP_REG_GET(addr);

			DDPMSG("regr: 0x%lx = 0x%08X\n", addr, regVal);
			sprintf(buf, "regr: 0x%lx = 0x%08X\n", addr, regVal);
		} else {
			sprintf(buf, "regr, invalid address 0x%lx\n", addr);
			goto Error;
		}
	} else if (strncmp(opt, "lfr_update", 3) == 0) {
		DSI_LFR_UPDATE(DISP_MODULE_DSI0, NULL);
	} else if (strncmp(opt, "regw:", 5) == 0) {
		unsigned long addr;
		unsigned int val;

		ret = sscanf(opt, "regw:0x%lx,0x%x\n", &addr, &val);
		if (ret != 2) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		if (is_reg_addr_valid(1, addr)) {
			unsigned int regVal;

			DISP_CPU_REG_SET(addr, val);
			regVal = DISP_REG_GET(addr);
			DDPMSG("regw: 0x%lx, 0x%08X = 0x%08X\n",
				addr, val, regVal);
			sprintf(buf, "regw: 0x%lx, 0x%08X = 0x%08X\n",
				addr, val, regVal);
		} else {
			sprintf(buf, "regw, invalid address 0x%lx\n", addr);
			goto Error;
		}
	} else if (strncmp(opt, "dbg_log:", 8) == 0) {
		char *p = (char *)opt + 8;
		unsigned int enable;

		ret = kstrtouint(p, 0, &enable);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		if (enable)
			dbg_log_level = 1;
		else
			dbg_log_level = 0;

		sprintf(buf, "dbg_log: %d\n", dbg_log_level);
	} else if (strncmp(opt, "vfp:", 4) == 0) {
		char *p = (char *)opt + 4;
		unsigned int vfp = 0;

		ret = kstrtouint(p, 0, &vfp);
		if (ret) {
			pr_debug("error to parse cmd %s\n", opt);
			return;
		}

		if (vfp > 0 && vfp <= 2000)
			backup_vfp_for_lp_cust(vfp);
	} else if (strncmp(opt, "irq_log:", 8) == 0) {
		char *p = (char *)opt + 8;
		unsigned int enable;

		ret = kstrtouint(p, 0, &enable);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}
		if (enable)
			irq_log_level = 1;
		else
			irq_log_level = 0;

		sprintf(buf, "irq_log: %d\n", irq_log_level);
	} else if (strncmp(opt, "met_on:", 7) == 0) {
		int met_on, rdma0_mode, rdma1_mode;

		ret = sscanf(opt, "met_on:%d,%d,%d\n",
			&met_on, &rdma0_mode, &rdma1_mode);
		if (ret != 3) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}
		ddp_init_met_tag(met_on, rdma0_mode, rdma1_mode);
		DDPMSG("process_dbg_opt, met_on=%d,rdma0_mode %d, rdma1 %d\n",
			met_on, rdma0_mode, rdma1_mode);
		sprintf(buf, "met_on:%d,rdma0_mode:%d,rdma1_mode:%d\n",
			met_on, rdma0_mode,	rdma1_mode);
	} else if (strncmp(opt, "backlight:", 10) == 0) {
		char *p = (char *)opt + 10;
		unsigned int level;

		ret = kstrtouint(p, 0, &level);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		if (level) {
			disp_bls_set_backlight(level);
			sprintf(buf, "backlight: %d\n", level);
		} else {
			goto Error;
		}
	} else if (strncmp(opt, "ddic_test:", 10) == 0) {
		char *p = (char *)opt + 10;
		unsigned int test_type;

		ret = kstrtouint(p, 0, &test_type);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		if (test_type == 0) {
			_ddic_test_read();
		} else if (test_type == 1) {
			_ddic_test_write();
		} else if (test_type == 2) {
			_ddic_test_read_write();
		} else if (test_type == 3) {
			_ddic_test_read_v1();
		}

	} else if (strncmp(opt, "partial:", 8) == 0) {
		ret = sscanf(opt, "partial:%d,%d,%d,%d,%d\n", &dbg_force_roi,
			&dbg_partial_x, &dbg_partial_y, &dbg_partial_w,
			&dbg_partial_h);
		if (ret != 5) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}
		DDPMSG("process_dbg_opt, partial force=%d (%d,%d,%d,%d)\n",
			dbg_force_roi, dbg_partial_x, dbg_partial_y,
			dbg_partial_w, dbg_partial_h);
	} else if (strncmp(opt, "partial_s:", 10) == 0) {
		ret = sscanf(opt, "partial_s:%d\n", &dbg_partial_statis);
		if (ret != 5) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}
		DDPMSG("process_dbg_opt, partial_s:%d\n", dbg_partial_statis);
	} else if (strncmp(opt, "pwm0:", 5) == 0 ||
			strncmp(opt, "pwm1:", 5) == 0) {
		char *p = (char *)opt + 5;
		unsigned int level;

		ret = kstrtouint(p, 0, &level);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		if (level) {
			enum disp_pwm_id_t pwm_id = DISP_PWM0;

			if (opt[3] == '1')
				pwm_id = DISP_PWM1;

			disp_pwm_set_backlight(pwm_id, level);
			sprintf(buf, "PWM 0x%x : %d\n", pwm_id, level);
		} else {
			goto Error;
		}
	} else if (strncmp(opt, "rdma_color:", 11) == 0) {
		if (strncmp(opt + 11, "on", 2) == 0) {
			/* char *p = (char *)opt + 14; */
			unsigned int red, green, blue;

			struct rdma_color_matrix matrix = { 0 };
			struct rdma_color_pre pre = { 0 };
			struct rdma_color_post post = { 255, 0, 0 };

			ret = sscanf(opt, "rdma_color:on,%d,%d,%d\n",
				&red, &green, &blue);
			if (ret != 3) {
				snprintf(buf, 50, "error to parse cmd %s\n",
					opt);
				pr_debug("error to parse cmd %s\n", opt);
				return;
			}

			post.ADD0 = red;
			post.ADD1 = green;
			post.ADD2 = blue;
			rdma_set_color_matrix(DISP_MODULE_RDMA0, &matrix, &pre,
				&post);
			rdma_enable_color_transform(DISP_MODULE_RDMA0);
		} else if (strncmp(opt + 11, "off", 3) == 0) {
			rdma_disable_color_transform(DISP_MODULE_RDMA0);
		}
	} else if (strncmp(opt, "aal_dbg:", 8) == 0) {
		char *p = (char *)opt + 8;

		ret = kstrtouint(p, 0, &aal_dbg_en);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		sprintf(buf, "aal_dbg_en = 0x%x\n", aal_dbg_en);
	} else if (strncmp(opt, "color_dbg:", 10) == 0) {
		char *p = (char *)opt + 10;
		unsigned int debug_level;

		ret = kstrtouint(p, 0, &debug_level);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		disp_color_dbg_log_level(debug_level);

		sprintf(buf, "color_dbg_en = 0x%x\n", debug_level);
	} else if (strncmp(opt, "corr_dbg:", 9) == 0) {
		char *p = (char *)opt + 9;

		ret = kstrtouint(p, 0, &corr_dbg_en);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		sprintf(buf, "corr_dbg_en = 0x%x\n", corr_dbg_en);
	} else if (strncmp(opt, "aal_test:", 9) == 0) {
		aal_test(opt + 9, buf);
	} else if (strncmp(opt, "pwm_test:", 9) == 0) {
		disp_pwm_test(opt + 9, buf);
	} else if (strncmp(opt, "dither_test:", 12) == 0) {
		/*dither_test(opt + 12, buf);*/
	} else if (strncmp(opt, "ccorr_test:", 11) == 0) {
		/*ccorr_test(opt + 11, buf);*/
	} else if (strncmp(opt, "od_test:", 8) == 0) {
		/* od_test(opt + 8, buf); */
	} else if (strncmp(opt, "dump_reg:", 9) == 0) {
		char *p = (char *)opt + 9;
		unsigned int module;

		ret = kstrtouint(p, 0, &module);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		DDPMSG("process_dbg_opt, module=%d\n", module);
		if (module < DISP_MODULE_NUM) {
			ddp_dump_reg(module);
			sprintf(buf, "dump_reg: %d\n", module);
		} else {
			DDPMSG("process_dbg_opt2, module=%d\n", module);
			goto Error;
		}
	} else if (strncmp(opt, "dump_path:", 10) == 0) {
		char *p = (char *)opt + 10;
		unsigned int mutex_idx;

		ret = kstrtouint(p, 0, &mutex_idx);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		DDPMSG("process_dbg_opt, path mutex=%d\n", mutex_idx);
		dpmgr_debug_path_status(mutex_idx);
		sprintf(buf, "dump_path: %d\n", mutex_idx);

	} else if (strncmp(opt, "get_module_addr", 15) == 0) {
		unsigned int i = 0;
		char *buf_temp = buf;

		for (i = 0; i < DISP_MODULE_NUM; i++) {
			if (!is_ddp_module_has_reg_info(i))
				continue;
			DDPDUMP("i=%d, %s, va=0x%lx, pa=0x%lx, irq(%d)\n",
				i, ddp_get_module_name(i), ddp_get_module_va(i),
				ddp_get_module_pa(i), ddp_get_module_irq(i));
			sprintf(buf_temp,
				"i=%d, %s, va=0x%lx, pa=0x%lx, irq(%d)\n",
				i, ddp_get_module_name(i), ddp_get_module_va(i),
				ddp_get_module_pa(i), ddp_get_module_irq(i));
			buf_temp += strlen(buf_temp);
		}

	} else if (strncmp(opt, "debug:", 6) == 0) {
		char *p = (char *)opt + 6;
		unsigned int enable;

		ret = kstrtouint(p, 0, &enable);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		if (enable == 1) {
			DDPMSG("[DDP] debug=1, trigger AEE\n");
		} else if (enable == 2) {
			ddp_mem_test();
		} else if (enable == 3) {
			ddp_lcd_test();
		} else if (enable == 4) {
			/* DDPAEE("test 4"); */
		} else if (enable == 12) {
			if (gUltraEnable == 0)
				gUltraEnable = 1;
			else
				gUltraEnable = 0;
			sprintf(buf, "gUltraEnable: %d\n", gUltraEnable);
		}
	} else if (strncmp(opt, "mmp", 3) == 0) {
#ifdef SUPPORT_MMPROFILE	 /* FIXME: remove when MMP ready */
		init_ddp_mmp_events();
#else
		;
#endif
	} else if (strncmp(opt, "low_power_mode:", 15) == 0) {
		char *p = (char *)opt + 15;
		unsigned int mode;

		ret = kstrtouint(p, 0, &mode);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		low_power_cust_mode = mode;

	} else if (strncmp(opt, "set_dsi_cmd:", 12) == 0) {
		int cmd;
		int para_cnt, i;
		char para[15] = {0};
		static char fmt[256] = {0};
		static const char temp[] = "set_dsi_cmd:0x%x";

		memset(fmt, 0, sizeof(fmt));
		strncpy((char *)fmt, (char *)temp, sizeof(temp));

		for (i = 0; i < ARRAY_SIZE(para); i++)
			strncat(fmt, ",0x%hhx", sizeof(fmt) - strlen(fmt) - 1);

		strncat(fmt, "\n", sizeof(fmt) - strlen(fmt) - 1);

		ret = sscanf(opt, fmt,
			&cmd, &para[0], &para[1], &para[2], &para[3], &para[4],
			&para[5], &para[6], &para[7], &para[8], &para[9],
			&para[10], &para[11], &para[12], &para[13], &para[14]);

		if (ret < 1 || ret > ARRAY_SIZE(para) + 1) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		para_cnt = ret - 1;

		DSI_set_cmdq_V2(DISP_MODULE_DSI0, NULL, cmd, para_cnt, para, 1);

		DISPMSG("set_dsi_cmd cmd=0x%x\n", cmd);
		for (i = 0; i < para_cnt; i++)
			DISPMSG("para[%d] = 0x%x\n", i, para[i]);

	} else if (strncmp(opt, "dsi_read:", 9) == 0) {
		int cmd;
		int size, i, tmp = 0;
		char para[15] = {0};

		ret = sscanf(opt, "dsi_read:0x%x,%d\n",	&cmd, &size);

		if (ret != 2 || size > ARRAY_SIZE(para)) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		DSI_dcs_read_lcm_reg_v2(DISP_MODULE_DSI0, NULL, cmd,
			para, size);

		tmp += snprintf(buf, buf_size_left, "dsi_read cmd=0x%x:", cmd);

		for (i = 0; i < size; i++)
			tmp += snprintf(buf + tmp, buf_size_left - tmp,
				"para[%d]=0x%x,", i, para[i]);
		DISPMSG("%s\n", buf);
	} else if (strncmp(opt, "set_customer_cmd:", 17) == 0) {
		int cmd;
		int hs;
		int para_cnt, i;
		char para[15] = {0};
		struct LCM_setting_table_V3 test;
		static char fmt[256] = "set_customer_cmd:0x%x, %d";

		for (i = 0; i < ARRAY_SIZE(para); i++)
			strncat(fmt, ",0x%hhx", sizeof(fmt) - strlen(fmt) - 1);

		strncat(fmt, "\n", sizeof(fmt) - strlen(fmt) - 1);

		ret = sscanf(opt, fmt, &cmd,
			&hs, &para[0], &para[1], &para[2], &para[3], &para[4],
			&para[5], &para[6], &para[7], &para[8], &para[9],
			&para[10], &para[11], &para[12], &para[13], &para[14]);

		if (ret < 1 || ret > ARRAY_SIZE(para) + 1) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		para_cnt = ret - 2;
		test.id = REGFLAG_ESCAPE_ID;
		test.cmd = cmd;
		test.count = para_cnt;
		for (i = 0; i < 15; i++)
			test.para_list[i] = para[i];
		pr_debug("set_dsi_cmd cmd=0x%x\n", cmd);
		for (i = 0; i < para_cnt; i++)
			pr_debug("para[%d] = 0x%x\n", i, para[i]);
		set_lcm(&test, 1, hs);

	} else if (strncmp(opt, "read_customer_cmd:", 18) == 0) {
		int cmd;
		int size, i;
		int sendhs;

		memset(para_lcd_reg,0,16);

		ret = sscanf(opt, "read_customer_cmd:0x%x, %d, %d\n",
						&cmd, &size, &sendhs);

		if (ret != 3 || size > ARRAY_SIZE(para_lcd_reg)) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		lcd_reg_cmd = cmd;
		lcd_reg_size = size;
		pr_debug(" read_lcm: 0x%x, size= %d %d\n", cmd, size, sendhs);
		read_lcm(cmd, para_lcd_reg, size, sendhs);

		for (i = 0; i < size; i++)
			pr_debug("para[%d] = 0x%x\n", i, para_lcd_reg[i]);
	} else {
		dbg_buf[0] = '\0';
		goto Error;
	}

	return;

Error:
	DDPERR("parse command error!\n%s\n\n%s", opt, STR_HELP);
}

static ssize_t debug_lcd_reg_dump(struct file *file, char __user *buf,
	size_t size, loff_t *ppos)
	
{
	int i;
	ssize_t n = 0, count;
	char *str1 = NULL;

	str1 = kmalloc(128, GFP_KERNEL);
	    if (!str1) {
		    pr_err("%s: sh could not allocate a buffer \n");
		        return -1;
	    }

	   memset(str1,0,128);

	  n += snprintf(str1 + n, 128-n, "REG 0x%02x: ", lcd_reg_cmd); 
	
	for (i = 0; i < lcd_reg_size; i++)
	{
		pr_debug("sh lcd reg para[%d] = 0x%x\n", i, para_lcd_reg[i]);
		n += snprintf(str1 + n, 128-n, "%02x ", para_lcd_reg[i]);
	}

		n += snprintf(str1 + n, 128- n, "\n");
	
	count =  simple_read_from_buffer(buf, size, ppos, str1,
			strlen(str1));

	kfree(str1);
	return count;
}


static const struct file_operations debug_fops_lcd_reg_dump = {
	.read = debug_lcd_reg_dump,
};


static void process_dbg_cmd(char *cmd)
{
	char *tok;

	DDPDBG("cmd: %s\n", cmd);
	memset(dbg_buf, 0, sizeof(dbg_buf));
	while ((tok = strsep(&cmd, " ")) != NULL)
		process_dbg_opt(tok);
}


/* ------------------------------------------------------------------------- */
/* Debug FileSystem Routines */
/* ------------------------------------------------------------------------- */

static int debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}


static char cmd_buf[512];

static ssize_t debug_read(struct file *file, char __user *ubuf, size_t count,
	loff_t *ppos)
{
	if (strlen(dbg_buf))
		return simple_read_from_buffer(ubuf, count, ppos, dbg_buf,
			strlen(dbg_buf));
	else
		return simple_read_from_buffer(ubuf, count, ppos, STR_HELP,
			strlen(STR_HELP));

}


static ssize_t debug_write(struct file *file, const char __user *ubuf,
	size_t count, loff_t *ppos)
{
	const int debug_bufmax = sizeof(cmd_buf) - 1;
	size_t ret;

	ret = count;

	if (count > debug_bufmax)
		count = debug_bufmax;

	if (copy_from_user(&cmd_buf, ubuf, count))
		return -EFAULT;

	cmd_buf[count] = 0;

	process_dbg_cmd(cmd_buf);

	return ret;
}


static const struct file_operations debug_fops = {
	.read = debug_read,
	.write = debug_write,
	.open = debug_open,
};

static ssize_t lp_cust_read(struct file *file, char __user *ubuf, size_t count,
	loff_t *ppos)
{
	char *mode0 = "low power mode(1)\n";
	char *mode1 = "just make mode(2)\n";
	char *mode2 = "performance mode(3)\n";
	char *mode4 = "unknown mode(n)\n";

	switch (low_power_cust_mode) {
	case LOW_POWER_MODE:
		return simple_read_from_buffer(ubuf, count, ppos, mode0,
			strlen(mode0));
	case JUST_MAKE_MODE:
		return simple_read_from_buffer(ubuf, count, ppos, mode1,
			strlen(mode1));
	case PERFORMANC_MODE:
		return simple_read_from_buffer(ubuf, count, ppos, mode2,
			strlen(mode2));
	default:
		return simple_read_from_buffer(ubuf, count, ppos, mode4,
			strlen(mode4));


	}

}

static const struct file_operations low_power_cust_fops = {
	.read = lp_cust_read,
};

static ssize_t debug_dump_read(struct file *file, char __user *buf,
	size_t size, loff_t *ppos)
{
	char *str = "idlemgr disable mtcmos now, all the regs may 0x00000000\n";

	dprec_logger_dump_reset();
	dump_to_buffer = 1;
	/* dump all */
	dpmgr_debug_path_status(-1);
	dump_to_buffer = 0;
	if (is_mipi_enterulps())
		return simple_read_from_buffer(buf, size, ppos, str,
			strlen(str));
	return simple_read_from_buffer(buf, size, ppos,
		dprec_logger_get_dump_addr(), dprec_logger_get_dump_len());
}


static const struct file_operations debug_fops_dump = {
	.read = debug_dump_read,
};

void ddp_debug_init(void)
{
	struct dentry *d;

	if (debug_init)
		return;

	debug_init = 1;
	debugfs = debugfs_create_file("dispsys",
		S_IFREG | 0444, NULL, (void *)0, &debug_fops);

	debugfs = debugfs_create_file("lcdreg",
			S_IFREG | 0444, NULL, (void *)0, &debug_fops_lcd_reg_dump);


	debugDir = debugfs_create_dir("disp", NULL);
	if (!debugDir)
		return;

	debugfs_dump = debugfs_create_file("dump",
		S_IFREG | 0444, debugDir, NULL, &debug_fops_dump);
	d = debugfs_create_file("lowpowermode", S_IFREG | 0444,
		debugDir, NULL, &low_power_cust_fops);
}

unsigned int ddp_debug_analysis_to_buffer(void)
{
	return dump_to_buffer;
}

unsigned int ddp_debug_dbg_log_level(void)
{
	return dbg_log_level;
}

unsigned int ddp_debug_irq_log_level(void)
{
	return irq_log_level;
}

int ddp_debug_force_roi(void)
{
	return dbg_force_roi;
}

int ddp_debug_partial_statistic(void)
{
	return dbg_partial_statis;
}

int ddp_debug_force_roi_x(void)
{
	return dbg_partial_x;
}

int ddp_debug_force_roi_y(void)
{
	return dbg_partial_y;
}

int ddp_debug_force_roi_w(void)
{
	return dbg_partial_w;
}

int ddp_debug_force_roi_h(void)
{
	return dbg_partial_h;
}

void ddp_debug_exit(void)
{
	debugfs_remove(debugfs);
	debugfs_remove(debugfs_dump);
	debug_init = 0;
}

int ddp_mem_test(void)
{
	return -1;
}

int ddp_lcd_test(void)
{
	return -1;
}
