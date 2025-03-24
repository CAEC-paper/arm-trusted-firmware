#
# Copyright (c) 2021, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

RMM_SOURCES		+=	plat/rockchip/rk3588/trp/rk_trp_setup.c	\
				plat/common/aarch64/platform_mp_stack.S \
				plat/rockchip/common/aarch64/plat_helpers.S \
				drivers/ti/uart/aarch64/16550_console.S	

INCLUDES		+=	-Iinclude/services/trp 
				  


