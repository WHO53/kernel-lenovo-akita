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

/*
 * DW9763_FHD1AF voice coil motor driver
 *
 *
 */

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>

#include "lens_info.h"

#define AF_DRVNAME "DW9763_FHD1AF_DRV"
#define AF_I2C_SLAVE_ADDR 0x18

#define AF_DEBUG
#ifdef AF_DEBUG
#define LOG_INF(format, args...)                                               \
    pr_debug(AF_DRVNAME " [%s] " format, __func__, ##args)
#else
#define LOG_INF(format, args...)
#endif

static struct i2c_client *g_pstAF_I2Cclient;
static int *g_pAF_Opened;
static spinlock_t *g_pAF_SpinLock;

static unsigned long g_u4AF_INF;
static unsigned long g_u4AF_MACRO = 1023;
static unsigned long g_u4CurrPosition;

static int i2c_read(u8 a_u2Addr, u8 *a_puBuff)
{
    int i4RetValue = 0;
    char puReadCmd[1] = {(char)(a_u2Addr)};

    g_pstAF_I2Cclient->addr = AF_I2C_SLAVE_ADDR;

    g_pstAF_I2Cclient->addr = g_pstAF_I2Cclient->addr >> 1;

    i4RetValue = i2c_master_send(g_pstAF_I2Cclient, puReadCmd, 1);
    if (i4RetValue < 0) {
        LOG_INF(" I2C write failed!!\n");
        return -1;
    }

    i4RetValue = i2c_master_recv(g_pstAF_I2Cclient, (char *)a_puBuff, 1);
    if (i4RetValue < 0) {
        LOG_INF(" I2C read failed!!\n");
        return -1;
    }

    return 0;
}

static u8 read_data(u8 addr)
{
    u8 get_byte = 0xFF;

    i2c_read(addr, &get_byte);

    return get_byte;
}

#if 0
static int DW9763_FHD1AF_ReadReg(unsigned short *a_pu2Result)
{
    *a_pu2Result = (read_data(0x02) << 8) + (read_data(0x03) & 0xff);

    return 0;
}
#endif

static int DW9763_FHD1AF_WriteReg(u16 a_u2Data)
{
    int i4RetValue = 0;
    char afValueH = (char)(a_u2Data >> 8);
    char afValueL = (char)(a_u2Data & 0xff);
    char puSendCmd[3] = {0x03, afValueH,afValueL};
    //char puSendCmd1[2] = {0x03, afValueH};
    //char puSendCmd2[2] = {0x04, afValueL};
    g_pstAF_I2Cclient->addr = AF_I2C_SLAVE_ADDR;

    g_pstAF_I2Cclient->addr = g_pstAF_I2Cclient->addr >> 1;

    i4RetValue = i2c_master_send(g_pstAF_I2Cclient, puSendCmd, 3);
    if (i4RetValue < 0) {
        LOG_INF("I2C send 0x03:0x%xfailed!!\n",a_u2Data);
        return -1;
    }

    return 0;
}

static inline int getAFInfo(__user struct stAF_MotorInfo *pstMotorInfo)
{
    struct stAF_MotorInfo stMotorInfo;

    stMotorInfo.u4MacroPosition = g_u4AF_MACRO;
    stMotorInfo.u4InfPosition = g_u4AF_INF;
    stMotorInfo.u4CurrentPosition = g_u4CurrPosition;
    stMotorInfo.bIsSupportSR = 1;

    stMotorInfo.bIsMotorMoving = 1;

    if (*g_pAF_Opened >= 1)
        stMotorInfo.bIsMotorOpen = 1;
    else
        stMotorInfo.bIsMotorOpen = 0;

    if (copy_to_user(pstMotorInfo, &stMotorInfo,
                sizeof(struct stAF_MotorInfo)))
        LOG_INF("copy to user failed when getting motor information\n");

    return 0;
}

/* initAF include driver initialization and standby mode */
static int initAF(void)
{
    LOG_INF("+\n");

    if (*g_pAF_Opened == 1) {

        u8 data = 0x0;
        int i4RetValue = 0;
        char puSendCmd1[2] = {0x02, 0x03}; 
        char puSendCmd2[2] = {0x02, 0x02};
        char puSendCmd3[2] = {0x06, 0x60};
        char puSendCmd4[2] = {0x07, 0x00};
        //char puSendCmd5[2] = {0x03, 0x00};
        //char puSendCmd5[2] = {0x04, 0x00};
        g_pstAF_I2Cclient->addr = AF_I2C_SLAVE_ADDR;
        g_pstAF_I2Cclient->addr = g_pstAF_I2Cclient->addr >> 1;

        i4RetValue = i2c_master_send(g_pstAF_I2Cclient, puSendCmd1, 2);

        if (i4RetValue < 0) {
            LOG_INF("I2C send puSendCmd1 failed!!\n");
            return -1;
        }

        i4RetValue = i2c_master_send(g_pstAF_I2Cclient, puSendCmd2, 2);

        if (i4RetValue < 0) {
            LOG_INF("I2C send puSendCmd2 failed!!\n");
            return -1;
        }

        i4RetValue = i2c_master_send(g_pstAF_I2Cclient, puSendCmd3, 2);

        if (i4RetValue < 0) {
            LOG_INF("I2C send puSendCmd3 failed!!\n");
            return -1;
        }

        i4RetValue = i2c_master_send(g_pstAF_I2Cclient, puSendCmd4, 2);

        if (i4RetValue < 0) {
            LOG_INF("I2C send puSendCmd4 failed!!\n");
            return -1;
        }

        LOG_INF("driver init success!!\n");
        data = read_data(0x00);
        LOG_INF("Addr:0x00 Data:0x%x (%d)\n", data, i4RetValue);
        spin_lock(g_pAF_SpinLock);
        *g_pAF_Opened = 2;
        spin_unlock(g_pAF_SpinLock);
    }

    LOG_INF("-\n");

    return 0;
}

/* moveAF only use to control moving the motor */
static inline int moveAF(unsigned long a_u4Position)
{
    int ret = 0;

    //LOG_INF(" a_u4Position %lu\n",a_u4Position);
    if (DW9763_FHD1AF_WriteReg((unsigned short)a_u4Position) == 0) {
        g_u4CurrPosition = a_u4Position;
        ret = 0;
    } else {
        LOG_INF("set I2C failed when moving the motor\n");
        ret = -1;
    }

    return ret;
}

static inline int setAFInf(unsigned long a_u4Position)
{
    spin_lock(g_pAF_SpinLock);
    g_u4AF_INF = a_u4Position;
    spin_unlock(g_pAF_SpinLock);
    return 0;
}

static inline int setAFMacro(unsigned long a_u4Position)
{
    spin_lock(g_pAF_SpinLock);
    g_u4AF_MACRO = a_u4Position;
    spin_unlock(g_pAF_SpinLock);
    return 0;
}

/* ////////////////////////////////////////////////////////////// */
long DW9763_FHD1AF_Ioctl(struct file *a_pstFile, unsigned int a_u4Command,
        unsigned long a_u4Param)
{
    long i4RetValue = 0;

    switch (a_u4Command) {
        case AFIOC_G_MOTORINFO:
            i4RetValue =
                getAFInfo((__user struct stAF_MotorInfo *)(a_u4Param));
            break;

        case AFIOC_T_MOVETO:
            i4RetValue = moveAF(a_u4Param);
            break;

        case AFIOC_T_SETINFPOS:
            i4RetValue = setAFInf(a_u4Param);
            break;

        case AFIOC_T_SETMACROPOS:
            i4RetValue = setAFMacro(a_u4Param);
            break;

        default:
            LOG_INF("No CMD 0x%x \n",a_u4Command);
            LOG_INF("AFIOC_G_MOTORINFO:%x \n",AFIOC_G_MOTORINFO);
            LOG_INF("AFIOC_T_MOVETO:%x \n",AFIOC_T_MOVETO);
            LOG_INF("AFIOC_T_SETINFPOS:%x \n",AFIOC_T_SETINFPOS);
            LOG_INF("AFIOC_T_SETMACROPOS:%x \n",AFIOC_T_SETMACROPOS);
            LOG_INF("AFIOC_G_MOTORCALPOS:%x \n",AFIOC_G_MOTORCALPOS);
            LOG_INF("AFIOC_S_SETPARA:%x \n",AFIOC_S_SETPARA);
            LOG_INF("AFIOC_S_SETDRVNAME:%x \n",AFIOC_S_SETDRVNAME);
            LOG_INF("AFIOC_S_SETPOWERDOWN:%x \n",AFIOC_S_SETPOWERDOWN);
            LOG_INF("AFIOC_G_MOTOROISINFO:%x \n",AFIOC_G_MOTOROISINFO);
            LOG_INF("AFIOC_S_SETPOWERCTRL:%x \n",AFIOC_S_SETPOWERCTRL);
            LOG_INF("AFIOC_S_SETLENSTEST:%x \n",AFIOC_S_SETLENSTEST);
            LOG_INF("AFIOC_G_OISPOSINFO:%x \n",AFIOC_G_OISPOSINFO);
            LOG_INF("AFIOC_S_SETDRVINIT:%x \n",AFIOC_S_SETDRVINIT);
            LOG_INF("AFIOC_G_GETDRVNAME:%x \n",AFIOC_G_GETDRVNAME);
            i4RetValue = -EPERM;
            break;
    }

    return i4RetValue;
}

/* Main jobs: */
/* 1.Deallocate anything that "open" allocated in private_data. */
/* 2.Shut down the device on last close. */
/* 3.Only called once on last time. */
/* Q1 : Try release multiple times. */
int DW9763_FHD1AF_Release(struct inode *a_pstInode, struct file *a_pstFile)
{
    LOG_INF("Start\n");

    if (*g_pAF_Opened == 2) {
        int i4RetValue = 0;
        char puSendCmd[2] = {0x02, 0x01};

        LOG_INF("apply\n");

        g_pstAF_I2Cclient->addr = AF_I2C_SLAVE_ADDR;
        g_pstAF_I2Cclient->addr = g_pstAF_I2Cclient->addr >> 1;
        i4RetValue = i2c_master_send(g_pstAF_I2Cclient, puSendCmd, 2);
    }

    if (*g_pAF_Opened) {
        LOG_INF("Free\n");

        spin_lock(g_pAF_SpinLock);
        *g_pAF_Opened = 0;
        spin_unlock(g_pAF_SpinLock);
    }

    LOG_INF("End\n");

    return 0;
}
int DW9763_FHD1AF_PowerDown(struct i2c_client *pstAF_I2Cclient,
        int *pAF_Opened)
{
    g_pstAF_I2Cclient = pstAF_I2Cclient;
    g_pAF_Opened = pAF_Opened;

    LOG_INF("+\n");
    if (*g_pAF_Opened == 0) {
        int i4RetValue = 0;
        u8 data = 0x0;
        char puSendCmd[2] = {0x00, 0x01};

        g_pstAF_I2Cclient->addr = AF_I2C_SLAVE_ADDR;
        g_pstAF_I2Cclient->addr = g_pstAF_I2Cclient->addr >> 1;
        i4RetValue = i2c_master_send(g_pstAF_I2Cclient, puSendCmd, 2);

        data = read_data(0x00);
        LOG_INF("Addr:0x00 Data:0x%x\n", data);

        LOG_INF("apply - %d\n", i4RetValue);

        if (i4RetValue < 0)
            return -1;
    }
    LOG_INF("-\n");

    return 0;
}

int DW9763_FHD1AF_SetI2Cclient(struct i2c_client *pstAF_I2Cclient,
        spinlock_t *pAF_SpinLock, int *pAF_Opened)
{
    g_pstAF_I2Cclient = pstAF_I2Cclient;
    g_pAF_SpinLock = pAF_SpinLock;
    g_pAF_Opened = pAF_Opened;

    initAF();

    return 1;
}

int DW9763_FHD1AF_GetFileName(unsigned char *pFileName)
{
#if SUPPORT_GETTING_LENS_FOLDER_NAME
    char FilePath[256];
    char *FileString;

    sprintf(FilePath, "%s", __FILE__);
    FileString = strrchr(FilePath, '/');
    *FileString = '\0';
    FileString = (strrchr(FilePath, '/') + 1);
    strncpy(pFileName, FileString, AF_MOTOR_NAME);
    LOG_INF("FileName : %s\n", pFileName);
#else
    pFileName[0] = '\0';
#endif
    return 1;
}
