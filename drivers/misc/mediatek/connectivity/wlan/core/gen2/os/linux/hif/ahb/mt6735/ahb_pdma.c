/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

#define MODULE_AHB_DMA

#include <linux/version.h>	/* constant of kernel version */

#include <linux/kernel.h>	/* bitops.h */

#include <linux/timer.h>	/* struct timer_list */
#include <linux/jiffies.h>	/* jiffies */
#include <linux/delay.h>	/* udelay and mdelay macro */

#if CONFIG_ANDROID
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
#include <linux/wakelock.h>
#endif
#endif

#include <linux/irq.h>		/* IRQT_FALLING */

#include <linux/netdevice.h>	/* struct net_device, struct net_device_stats */
#include <linux/etherdevice.h>	/* for eth_type_trans() function */
#include <linux/wireless.h>	/* struct iw_statistics */
#include <linux/if_arp.h>
#include <linux/inetdevice.h>	/* struct in_device */

#include <linux/ip.h>		/* struct iphdr */

#include <linux/string.h>	/* for memcpy()/memset() function */
#include <linux/stddef.h>	/* for offsetof() macro */

#include <linux/proc_fs.h>	/* The proc filesystem constants/structures */

#include <linux/rtnetlink.h>	/* for rtnl_lock() and rtnl_unlock() */
#include <linux/kthread.h>	/* kthread_should_stop(), kthread_run() */
#include <linux/uaccess.h>	/* for copy_from_user() */
#include <linux/fs.h>		/* for firmware download */
#include <linux/vmalloc.h>

#include <linux/kfifo.h>	/* for kfifo interface */
#include <linux/cdev.h>		/* for cdev interface */

#include <linux/firmware.h>	/* for firmware download */

#include <linux/random.h>

#include <linux/io.h>		/* readw and writew */

#include <linux/module.h>
#include <linux/errno.h>

#if defined(CONFIG_MTK_CLKMGR)
#include <mach/mt_clkmgr.h>
#else
#include <linux/clk.h>
#endif /* defined(CONFIG_MTK_CLKMGR) */

#include "hif.h"
#include "hif_pdma.h"
#include "gl_os.h"

#include <mach/emi_mpu.h>

/* #if (CONF_MTK_AHB_DMA == 1) */

/* #define PDMA_DEBUG_SUP */

#ifdef PDMA_DEBUG_SUP
#define PDMA_DBG	pr_info
#else
#define PDMA_DBG(_fmt, ...)
#endif /* PDMA_DEBUG_SUP */

static UINT_32 gDmaReg[AP_DMA_HIF_0_LENGTH];

#if !defined(CONFIG_MTK_CLKMGR)
struct clk *g_clk_wifi_pdma;
#endif

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static VOID HifPdmaConfig(IN void *HifInfoSrc, IN void *Conf);

static VOID HifPdmaStart(IN void *HifInfoSrc);

static VOID HifPdmaStop(IN void *HifInfoSrc);

static MTK_WCN_BOOL HifPdmaPollStart(IN void *HifInfoSrc);

static MTK_WCN_BOOL HifPdmaPollIntr(IN void *HifInfoSrc);

static VOID HifPdmaAckIntr(IN void *HifInfoSrc);

static VOID HifPdmaClockCtrl(IN UINT_32 FlgIsEnabled);

static VOID HifPdmaRegDump(IN void *HifInfoSrc);

static VOID HifPdmaReset(IN void *HifInfoSrc);

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
GL_HIF_DMA_OPS_T HifPdmaOps = {
	.DmaConfig = HifPdmaConfig,
	.DmaStart = HifPdmaStart,
	.DmaStop = HifPdmaStop,
	.DmaPollStart = HifPdmaPollStart,
	.DmaPollIntr = HifPdmaPollIntr,
	.DmaAckIntr = HifPdmaAckIntr,
	.DmaClockCtrl = HifPdmaClockCtrl,
	.DmaRegDump = HifPdmaRegDump,
	.DmaReset = HifPdmaReset
};

/*******************************************************************************
*                        P U B L I C   F U N C T I O N S
********************************************************************************
*/

#if defined(CONFIG_ARCH_MT6735) || defined(CONFIG_ARCH_MT6753)
int HifAhbSetMpuProtect(bool enable)
{
	emi_mpu_set_region_protection(
		gConEmiPhyBase, gConEmiPhyBase + 512 * 1024 - 1, 12,
		SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, FORBIDDEN,
				     FORBIDDEN, FORBIDDEN,
				     NO_PROTECTION, FORBIDDEN,
				     enable ? FORBIDDEN : NO_PROTECTION));
	return 0;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
* \brief Config PDMA TX/RX.
*
* \param[in] prGlueInfo         Pointer to the GLUE_INFO_T structure.
* \param[in] Conf               Pointer to the settings.
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
VOID HifPdmaInit(GL_HIF_INFO_T *HifInfo)
{
	/* IO remap PDMA register memory */
	HifInfo->DmaRegBaseAddr = ioremap(AP_DMA_HIF_BASE, AP_DMA_HIF_0_LENGTH);

	/* assign PDMA operators */
	HifInfo->DmaOps = &HifPdmaOps;

	/* enable PDMA mode */
	HifInfo->fgDmaEnable = TRUE;

	/* WIFI using TOP 512KB */
	DBGLOG(HAL, TRACE, "[wlan] MPU region 12, 0x%08x - 0x%08x\n", (UINT_32) gConEmiPhyBase,
	       (UINT_32) (gConEmiPhyBase + 512 * 1024));
#if defined(CONFIG_ARCH_MT6735) || defined(CONFIG_ARCH_MT6753)
	/* for MT6735M, we share region with wmt due to not enough region to use */
	HifAhbSetMpuProtect(true);
#endif

#if !defined(CONFIG_MTK_CLKMGR)
	g_clk_wifi_pdma = HifInfo->clk_wifi_dma;
#endif

	PDMA_DBG("PDMA> HifPdmaInit ok!\n");
}

/*******************************************************************************
*                       P R I V A T E   F U N C T I O N S
********************************************************************************
*/

/*----------------------------------------------------------------------------*/
/*!
* \brief Config PDMA TX/RX.
*
* \param[in] HifInfo            Pointer to the GL_HIF_INFO_T structure.
* \param[in] Param              Pointer to the settings.
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
static VOID HifPdmaConfig(IN void *HifInfoSrc, IN void *Param)
{
	GL_HIF_INFO_T *HifInfo = (GL_HIF_INFO_T *) HifInfoSrc;
	MTK_WCN_HIF_DMA_CONF *Conf = (MTK_WCN_HIF_DMA_CONF *) Param;
	UINT_32 RegVal;

	/* Assign fixed value */
	Conf->Burst = HIF_PDMA_BURST_4_4;	/* vs. HIF_BURST_4DW */
	Conf->Fix_en = FALSE;

	/* AP_P_DMA_G_DMA_2_CON */
	PDMA_DBG("PDMA> Conf->Dir = %d\n", Conf->Dir);

	/* AP_DMA_HIF_0_CON */
	RegVal = HIF_DMAR_READL(HifInfo, AP_DMA_HIF_0_CON);
	RegVal &= ~(ADH_CR_BURST_LEN | ADH_CR_FIX_EN | ADH_CR_DIR);
	RegVal |= (((Conf->Burst << ADH_CR_BURST_LEN_OFFSET) & ADH_CR_BURST_LEN) |
		   (Conf->Fix_en << ADH_CR_FIX_EN_OFFSET) | (Conf->Dir));
	HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_CON, RegVal);
	PDMA_DBG("PDMA> AP_DMA_HIF_0_CON = 0x%08x\n", RegVal);

	/* AP_DMA_HIF_0_SRC_ADDR */
	HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_SRC_ADDR, Conf->Src);
	PDMA_DBG("PDMA> AP_DMA_HIF_0_SRC_ADDR = 0x%08lx\n", Conf->Src);

	/* AP_DMA_HIF_0_DST_ADDR */
	HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_DST_ADDR, Conf->Dst);
	PDMA_DBG("PDMA> AP_DMA_HIF_0_DST_ADDR = 0x%08lx\n", Conf->Dst);

	/* AP_DMA_HIF_0_LEN */
	HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_LEN, (Conf->Count & ADH_CR_LEN));
	PDMA_DBG("PDMA> AP_DMA_HIF_0_LEN = %u\n", (UINT_32)(Conf->Count & ADH_CR_LEN));

}				/* End of HifPdmaConfig */

/*----------------------------------------------------------------------------*/
/*!
* \brief Start PDMA TX/RX.
*
* \param[in] HifInfo            Pointer to the GL_HIF_INFO_T structure.
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
static VOID HifPdmaStart(IN void *HifInfoSrc)
{
	GL_HIF_INFO_T *HifInfo = (GL_HIF_INFO_T *) HifInfoSrc;
	UINT_32 RegVal, RegIdx, RegId;

	/* Enable interrupt */
	RegVal = HIF_DMAR_READL(HifInfo, AP_DMA_HIF_0_INT_EN);
	HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_INT_EN, (RegVal | ADH_CR_INTEN_FLAG_0));

	/* keep old register settings */
	RegIdx = 0;
	for (RegId = 0; RegId < AP_DMA_HIF_0_LENGTH; RegId += 4)
		gDmaReg[RegIdx++] = HIF_DMAR_READL(HifInfo, RegId);

	/* Start DMA */
	RegVal = HIF_DMAR_READL(HifInfo, AP_DMA_HIF_0_EN);
	HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_EN, (RegVal | ADH_CR_EN));

	PDMA_DBG("PDMA> HifPdmaStart...\n");

}				/* End of HifPdmaStart */

/*----------------------------------------------------------------------------*/
/*!
* \brief Stop PDMA TX/RX.
*
* \param[in] HifInfo            Pointer to the GL_HIF_INFO_T structure.
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
static VOID HifPdmaStop(IN void *HifInfoSrc)
{
	GL_HIF_INFO_T *HifInfo = (GL_HIF_INFO_T *) HifInfoSrc;
	UINT_32 RegVal;
/* UINT32 pollcnt; */

	/* Disable interrupt */
	RegVal = HIF_DMAR_READL(HifInfo, AP_DMA_HIF_0_INT_EN);
	HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_INT_EN, (RegVal & ~(ADH_CR_INTEN_FLAG_0)));

#if 0				/* DE says we donot need to do it */
	/* Stop DMA */
	RegVal = HIF_DMAR_READL(HifInfo, AP_DMA_HIF_0_STOP);
	HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_STOP, (RegVal | ADH_CR_STOP));

	/* Polling START bit turn to 0 */
	pollcnt = 0;
	do {
		RegVal = HIF_DMAR_READL(HifInfo, AP_DMA_HIF_0_EN);
		if (pollcnt++ > 100000)
			; /* TODO: warm reset PDMA */
	} while (RegVal & ADH_CR_EN);
#endif

}				/* End of HifPdmaStop */

/*----------------------------------------------------------------------------*/
/*!
* \brief Enable PDMA TX/RX.
*
* \param[in] HifInfo            Pointer to the GL_HIF_INFO_T structure.
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
static MTK_WCN_BOOL HifPdmaPollStart(IN void *HifInfoSrc)
{
	GL_HIF_INFO_T *HifInfo = (GL_HIF_INFO_T *) HifInfoSrc;
	UINT_32 RegVal;

	RegVal = HIF_DMAR_READL(HifInfo, AP_DMA_HIF_0_EN);
	return ((RegVal & ADH_CR_EN) != 0) ? TRUE : FALSE;

}				/* End of HifPdmaPollStart */

/*----------------------------------------------------------------------------*/
/*!
* \brief Poll PDMA TX/RX done.
*
* \param[in] HifInfo            Pointer to the GL_HIF_INFO_T structure.
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
static MTK_WCN_BOOL HifPdmaPollIntr(IN void *HifInfoSrc)
{
	GL_HIF_INFO_T *HifInfo = (GL_HIF_INFO_T *) HifInfoSrc;
	UINT_32 RegVal;

	RegVal = HIF_DMAR_READL(HifInfo, AP_DMA_HIF_0_INT_FLAG);
	return ((RegVal & ADH_CR_FLAG_0) != 0) ? TRUE : FALSE;

}				/* End of HifPdmaPollIntr */

/*----------------------------------------------------------------------------*/
/*!
* \brief Acknowledge PDMA TX/RX done.
*
* \param[in] HifInfo            Pointer to the GL_HIF_INFO_T structure.
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
static VOID HifPdmaAckIntr(IN void *HifInfoSrc)
{
	GL_HIF_INFO_T *HifInfo = (GL_HIF_INFO_T *) HifInfoSrc;
	UINT_32 RegVal;

	/* Write 0 to clear interrupt */
	RegVal = HIF_DMAR_READL(HifInfo, AP_DMA_HIF_0_INT_FLAG);
	HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_INT_FLAG, (RegVal & ~ADH_CR_FLAG_0));

}				/* End of HifPdmaAckIntr */

/*----------------------------------------------------------------------------*/
/*!
* \brief Acknowledge PDMA TX/RX done.
*
* \param[in] FlgIsEnabled       TRUE: enable; FALSE: disable
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
static VOID HifPdmaClockCtrl(IN UINT_32 FlgIsEnabled)
{
#if !defined(CONFIG_MTK_CLKMGR)
	int ret = 0;
#endif

#if defined(CONFIG_MTK_CLKMGR)
	if (FlgIsEnabled == TRUE)
		enable_clock(MT_CG_PERI_APDMA, "WLAN");
	else
		disable_clock(MT_CG_PERI_APDMA, "WLAN");
#else
	if (FlgIsEnabled == TRUE) {
		ret = clk_prepare_enable(g_clk_wifi_pdma);
		if (ret)
			DBGLOG(INIT, TRACE, "[CCF]clk_prepare_enable ret= %d\n", ret);
	} else {
		clk_disable_unprepare(g_clk_wifi_pdma);
	}
#endif
}

/*----------------------------------------------------------------------------*/
/*!
* \Dump PDMA related registers when there is abnormal, such as DMA timeout.
*
* \param[in] HifInfo            Pointer to the GL_HIF_INFO_T structure.
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
static VOID HifPdmaRegDump(IN void *HifInfoSrc)
{
	GL_HIF_INFO_T *HifInfo = (GL_HIF_INFO_T *) HifInfoSrc;
	UINT_32 RegId, RegVal, RegIdx;
	UINT_32 RegNum = 0;
	INT_8 *pucIoAddr;
	UINT_32 regVal1, regVal2, regVal3;

	DBGLOG(INIT, INFO, "PDMA> Register content before start 0x%x=\n\t", AP_DMA_HIF_BASE);
	for (RegId = 0, RegIdx = 0; RegId < AP_DMA_HIF_0_LENGTH; RegId += 4) {
		RegVal = gDmaReg[RegIdx++];
		DBGLOG(INIT, INFO, "0x%08x ", RegVal);

		if (RegNum++ >= 3) {
			DBGLOG(INIT, INFO, "\n");
			DBGLOG(INIT, INFO, "PDMA> Register content 0x%x=\n\t", AP_DMA_HIF_BASE + RegId + 4);
			RegNum = 0;
		}
	}

	DBGLOG(INIT, INFO, "PDMA> Register content 0x%x=\n\t", AP_DMA_HIF_BASE);
	for (RegId = 0; RegId < AP_DMA_HIF_0_LENGTH; RegId += 4) {
		RegVal = HIF_DMAR_READL(HifInfo, RegId);
		DBGLOG(INIT, INFO, "0x%08x ", RegVal);

		if (RegNum++ >= 3) {
			DBGLOG(INIT, INFO, "\n");
			DBGLOG(INIT, INFO, "PDMA> Register content 0x%x=\n\t", AP_DMA_HIF_BASE + RegId + 4);
			RegNum = 0;
		}
	}

	/* dump CONFIG */
	pucIoAddr = ioremap(0x18070000, 0x500);
	if (pucIoAddr) {
		UINT_32 u4CrId;
		UINT_32 u4ClockStatus = 0;

		for (u4CrId = 0; u4CrId <= 0x08; u4CrId += 4) {
			u4ClockStatus = *(volatile unsigned int *)(pucIoAddr+u4CrId);
			DBGLOG(INIT, INFO, "<hif> addr 0x%x: 0x%x\n", (UINT_32)(0x18070000 + u4CrId), u4ClockStatus);
		}
		for (u4CrId = 0x6c; u4CrId <= 0xB0; u4CrId += 4) {
			u4ClockStatus = *(volatile unsigned int *)(pucIoAddr+u4CrId);
			DBGLOG(INIT, INFO, "<hif> addr 0x%x: 0x%x\n", (UINT_32)(0x18070000 + u4CrId), u4ClockStatus);
		}
		for (u4CrId = 0x100; u4CrId <= 0x128; u4CrId += 4) {
			u4ClockStatus = *(volatile unsigned int *)(pucIoAddr+u4CrId);
			DBGLOG(INIT, INFO, "<hif> addr 0x%x: 0x%x\n", (UINT_32)(0x18070000 + u4CrId), u4ClockStatus);
		}
		for (u4CrId = 0x160; u4CrId <= 0x160; u4CrId += 4) {
			u4ClockStatus = *(volatile unsigned int *)(pucIoAddr+u4CrId);
			DBGLOG(INIT, INFO, "<hif> addr 0x%x: 0x%x\n", (UINT_32)(0x18070000 + u4CrId), u4ClockStatus);
		}
		for (u4CrId = 0x220; u4CrId <= 0x224; u4CrId += 4) {
			u4ClockStatus = *(volatile unsigned int *)(pucIoAddr+u4CrId);
			DBGLOG(INIT, INFO, "<hif> addr 0x%x: 0x%x\n", (UINT_32)(0x18070000 + u4CrId), u4ClockStatus);
		}
		for (u4CrId = 0x320; u4CrId <= 0x320; u4CrId += 4) {
			u4ClockStatus = *(volatile unsigned int *)(pucIoAddr+u4CrId);
			DBGLOG(INIT, INFO, "<hif> addr 0x%x: 0x%x\n", (UINT_32)(0x18070000 + u4CrId), u4ClockStatus);
		}
		for (u4CrId = 0x334; u4CrId <= 0x334; u4CrId += 4) {
			u4ClockStatus = *(volatile unsigned int *)(pucIoAddr + u4CrId);
			DBGLOG(INIT, INFO, "<hif> addr 0x%x: 0x%x\n", (UINT_32)(0x18070000 + u4CrId), u4ClockStatus);
		}
		for (u4CrId = 0x3c0; u4CrId <= 0x3c0; u4CrId += 4) {
			u4ClockStatus = *(volatile unsigned int *)(pucIoAddr + u4CrId);
			DBGLOG(INIT, INFO, "<hif> addr 0x%x: 0x%x\n", (UINT_32)(0x18070000 + u4CrId), u4ClockStatus);
		}
		for (u4CrId = 0x3d0; u4CrId <= 0x40c; u4CrId += 4) {
			u4ClockStatus = *(volatile unsigned int *)(pucIoAddr + u4CrId);
			DBGLOG(INIT, INFO, "<hif> addr 0x%x: 0x%x\n", (UINT_32)(0x18070000 + u4CrId), u4ClockStatus);
		}
		iounmap(pucIoAddr);
	}

	pucIoAddr = ioremap(0x180b0000, 0x100);
	if (pucIoAddr) {
		UINT_32 u4CrId;
		UINT_32 u4ClockStatus = 0;

		for (u4CrId = 0; u4CrId <= 0x04; u4CrId += 4) {
			u4ClockStatus = *(volatile unsigned int *)(pucIoAddr + u4CrId);
			DBGLOG(INIT, INFO, "<hif> addr 0x%x: 0x%x\n", (UINT_32)(0x180b0000 + u4CrId), u4ClockStatus);
		}
		for (u4CrId = 0x0c; u4CrId <= 0x0c; u4CrId += 4) {
			u4ClockStatus = *(volatile unsigned int *)(pucIoAddr + u4CrId);
			DBGLOG(INIT, INFO, "<hif> addr 0x%x: 0x%x\n", (UINT_32)(0x180b0000 + u4CrId), u4ClockStatus);
		}
		for (u4CrId = 0x28; u4CrId <= 0x88; u4CrId += 4) {
			u4ClockStatus = *(volatile unsigned int *)(pucIoAddr + u4CrId);
			DBGLOG(INIT, INFO, "<hif> addr 0x%x: 0x%x\n", (UINT_32)(0x180b0000 + u4CrId), u4ClockStatus);
		}
		iounmap(pucIoAddr);
	}

	pucIoAddr = ioremap(0x10002000, 0x100);
	if (pucIoAddr) {
		UINT_32 u4ClockStatus = 0;

		u4ClockStatus = *(volatile unsigned int *)(pucIoAddr + 0x18);
		iounmap(pucIoAddr);
		DBGLOG(INIT, INFO, "<dma> clock status: 0x%x\n", u4ClockStatus);
	}

	pucIoAddr = ioremap(0x11000008, 0x4);
	if (pucIoAddr) { /* DMA global register status, to observe the channel status of all channels */
		UINT_32 chnlStatus = 0;
		UINT_8 i = 1;
		UINT_8 *pucChnlStatus = NULL;

		chnlStatus = *(volatile unsigned int *)pucIoAddr;
		iounmap(pucIoAddr);

		DBGLOG(INIT, INFO, "global channel status: %x\n", chnlStatus);
		for (; i < 19; i++) {
			if ((chnlStatus & 1<<i) == 0)
				continue;
			pucChnlStatus = (unsigned char *)ioremap((AP_DMA_HIF_BASE + i * 0x80), 0x54);
			DBGLOG(INIT, INFO, "channel %d dir,0x18: %u\n", i,
				*(volatile unsigned int *)(pucChnlStatus + 0x18));
			DBGLOG(INIT, INFO, "channel %d debug 0x50: %u\n", i,
				*(volatile unsigned int *)(pucChnlStatus + 0x50));
			iounmap(pucChnlStatus);
		}
	}
	regVal1 = HIF_DMAR_READL(HifInfo, 0x18);
	regVal2 = HIF_DMAR_READL(HifInfo, 0x50);
	regVal3 = HIF_DMAR_READL(HifInfo, 0x54);
	DBGLOG(INIT, INFO, "HIF dir=%u, debug 0x50=%u, debug 0x54=%u\n", regVal1, regVal2, regVal3);

	/* DBGLOG(INIT, INFO, "\nPDMA> clock status = 0x%x\n\n", *(volatile unsigned int *)0xF0003018); */
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Reset DMA.
*
* \param[in] HifInfo            Pointer to the GL_HIF_INFO_T structure.
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
static VOID HifPdmaReset(IN void *HifInfoSrc)
{
	GL_HIF_INFO_T *HifInfo = (GL_HIF_INFO_T *) HifInfoSrc;
	UINT_32 LoopCnt;

	/* do warm reset: DMA will wait for current traction finished */
	DBGLOG(INIT, TRACE, "DMA> do warm reset...\n");

	/* normally, we need to sure that bit0 of AP_P_DMA_G_DMA_2_EN is 1 here */

	HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_RST, 0x01);

	for (LoopCnt = 0; LoopCnt < 10000; LoopCnt++) {
		if (!HifPdmaPollStart(HifInfo))
			break;	/* reset ok */
	}

	if (HifPdmaPollStart(HifInfo)) {
		/* do hard reset because warm reset fails */
		DBGLOG(INIT, INFO, "DMA> do hard reset...\n");
		HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_RST, 0x02);
		mdelay(1);
		HIF_DMAR_WRITEL(HifInfo, AP_DMA_HIF_0_RST, 0x00);
	}
}

/* #endif */ /* CONF_MTK_AHB_DMA */

/* End of ahb_pdma.c */
