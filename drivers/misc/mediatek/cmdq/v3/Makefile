#
# Copyright (C) 2015 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

# common driver code for CMDQ

ifeq ($(CONFIG_MACH_MT6761),y)
MTK_PLATFORM := mt6765
else
MTK_PLATFORM := $(CONFIG_MTK_PLATFORM)
endif

ifeq ($(CONFIG_MACH_MT6779),y)
ccflags-y += -DPMQOS_VERSION2
ccflags-y += -DMDP_MMPATH
ccflags-y += -DCMDQ_MDP_ENABLE_SPR
endif

ccflags-y += -I$(srctree)/drivers/misc/mediatek/smi/ \
             -I$(srctree)/drivers/misc/mediatek/mmp/ \
             -I$(srctree)/drivers/misc/mediatek/mdp/ \
             -I$(srctree)/drivers/iommu/ \
             -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM) \
             -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/include

# mdp platform code
MDP_PLATFORM_DIR=$(srctree)/drivers/misc/mediatek/cmdq/v3/$(MTK_PLATFORM)

platformdir_exists=$(shell if [ -d ${MDP_PLATFORM_DIR} ]; then echo "y"; else echo "n"; fi;)
ifeq ($(platformdir_exists), y)
ccflags-y += -I$(MDP_PLATFORM_DIR)/
else
ccflags-y += -DCMDQ_COMMON_ENG_SUPPORT
endif

# Enable AEE
ccflags-y += -DCMDQ_AEE_READY

ifeq ($(CONFIG_MTK_ENG_BUILD),y)
ccflags-y += -DCMDQ_TEST_PROC
ccflags-y += -DCMDQ_PROFILE_LOCK
ccflags-y += -DCMDQ_DEBUG_LOOP_IRQ
endif

# secure path
ifeq ($(or $(strip $(CONFIG_TRUSTONIC_TEE_SUPPORT)), $(strip $(CONFIG_MICROTRUST_TEE_SUPPORT))),y)
ifeq ($(or $(strip $(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT)), $(strip $(CONFIG_MTK_CAM_SECURITY_SUPPORT))),y)

ifeq ($(CONFIG_TRUSTONIC_TEE_SUPPORT),y)
include $(srctree)/drivers/misc/mediatek/gud/Makefile.include
endif

ifeq ($(CONFIG_MICROTRUST_TEE_SUPPORT),y)
include $(srctree)/drivers/misc/mediatek/teei/Makefile.include
endif	# end of CONFIG_MICROTRUST_TEE_SUPPORT

ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/inc/cmdq_sec_dr \
             -I$(srctree)/drivers/misc/mediatek/cmdq/v3/inc/cmdq_sec_tl

ccflags-y += -DCMDQ_SECURE_PATH_SUPPORT

ifeq ($(strip $(CONFIG_MTK_TEE_GP_SUPPORT)),y)
ccflags-y += -DCMDQ_GP_SUPPORT
endif

ccflags-y += -DCMDQ_EVENT_SVP_BACKUP
endif	# end of CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT or CONFIG_MTK_CAM_SECURITY_SUPPORT
endif	# end of CONFIG_TRUSTONIC_TEE_SUPPORT or CONFIG_MICROTRUST_TEE_SUPPORT

# platform dependent definition
ccflags-y += -DCMDQ_SPECIAL_ESD_PRIORITY
ccflags-y += -D_CMDQ_DISABLE_MARKER_
ccflags-y += -DCMDQ_PROFILE_MMP

ifneq ($(strip $(CONFIG_MTK_GMO_RAM_OPTIMIZE)),y)
ccflags-y += -DCMDQ_LARGE_MAX_FIRSTERROR_BUFFER
endif

# driver module
obj-y += cmdq_record.o
obj-y += cmdq_virtual.o
obj-y += cmdq_device.o
obj-y += cmdq_driver.o
obj-y += cmdq_mmp.o
obj-y += cmdq_mdp_common.o
#obj-y += cmdq_prof.o
#obj-y += cmdq_test.o
obj-y += cmdq_event_common.o
obj-y += cmdq_subsys_common.o
obj-y += cmdq_helper_ext.o

ifeq ($(platformdir_exists), y)
obj-y += $(subst ",,$(MTK_PLATFORM))/
endif

ifeq ($(or $(strip $(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT)), $(strip $(CONFIG_MTK_CAM_SECURITY_SUPPORT))),y)

obj-y += cmdq_sec.o
ifeq ($(strip $(CONFIG_MTK_TEE_GP_SUPPORT)),y)
obj-y += cmdq_sec_gp.o
else ifeq ($(CONFIG_TRUSTONIC_TEE_SUPPORT),y)
obj-y += cmdq_sec_trustonic.o
endif

endif	# end of CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT or CONFIG_MTK_CAM_SECURITY_SUPPORT

