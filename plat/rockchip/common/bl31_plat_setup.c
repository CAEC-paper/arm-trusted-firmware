/*
 * Copyright (c) 2016-2023, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <platform_def.h>

#include <common/bl_common.h>
#include <common/debug.h>
#include <common/desc_image_load.h>
#include <drivers/console.h>
#include <drivers/generic_delay_timer.h>
#include <drivers/ti/uart/uart_16550.h>
#include <lib/mmio.h>
#include <plat_private.h>
#include <plat/common/platform.h>

static entry_point_info_t bl32_ep_info;
static entry_point_info_t bl33_ep_info;
static entry_point_info_t rmm_ep_info;


static void bl31_rmm_params_parse_helper(u_register_t param,
			      entry_point_info_t *bl32_ep_info_out,
			      entry_point_info_t *bl33_ep_info_out,
				  entry_point_info_t *rmm_ep_info_out)
{
	bl_params_node_t *node;
	bl_params_t *v2 = (void *)(uintptr_t)param;

#if !ERROR_DEPRECATED
	if (v2->h.version == PARAM_VERSION_1) {
		struct { /* Deprecated version 1 parameter structure. */
			param_header_t h;
			image_info_t *bl31_image_info;
			entry_point_info_t *bl32_ep_info;
			image_info_t *bl32_image_info;
			entry_point_info_t *bl33_ep_info;
			image_info_t *bl33_image_info;
		} *v1 = (void *)(uintptr_t)param;
		assert(v1->h.type == PARAM_BL31);
		if (bl32_ep_info_out != NULL)
			*bl32_ep_info_out = *v1->bl32_ep_info;
		if (bl33_ep_info_out != NULL)
			*bl33_ep_info_out = *v1->bl33_ep_info;
		return;
	}
#endif /* !ERROR_DEPRECATED */

	assert(v2->h.version == PARAM_VERSION_2);
	assert(v2->h.type == PARAM_BL_PARAMS);
	for (node = v2->head; node != NULL; node = node->next_params_info) {
		if (node->image_id == BL32_IMAGE_ID)
			if (bl32_ep_info_out != NULL)
				*bl32_ep_info_out = *node->ep_info;
		if (node->image_id == BL33_IMAGE_ID)
			if (bl33_ep_info_out != NULL)
				*bl33_ep_info_out = *node->ep_info;
		if (node->image_id == RMM_IMAGE_ID)
			if (rmm_ep_info_out != NULL)
				*rmm_ep_info_out = *node->ep_info;
	}
}

/*******************************************************************************
 * Return a pointer to the 'entry_point_info' structure of the next image for
 * the security state specified. BL33 corresponds to the non-secure image type
 * while BL32 corresponds to the secure image type. A NULL pointer is returned
 * if the image does not exist.
 ******************************************************************************/
entry_point_info_t *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	entry_point_info_t *next_image_info;
	
	if (type == NON_SECURE) {
		next_image_info = &bl33_ep_info;
	} else if (type == REALM) {
		next_image_info = &rmm_ep_info;
	} else {
		next_image_info = &bl32_ep_info;
	}
	assert(next_image_info->h.type == PARAM_EP);

	/* None of the images on this platform can have 0x0 as the entrypoint */
	if (next_image_info->pc)
		return next_image_info;
	else
		return NULL;
}

#pragma weak params_early_setup
void params_early_setup(u_register_t plat_param_from_bl2)
{
}

/*******************************************************************************
 * Perform any BL3-1 early platform setup. Here is an opportunity to copy
 * parameters passed by the calling EL (S-EL1 in BL2 & EL3 in BL1) before they
 * are lost (potentially). This needs to be done before the MMU is initialized
 * so that the memory layout can be used while creating page tables.
 * BL2 has flushed this information to memory, so we are guaranteed to pick up
 * good data.
 ******************************************************************************/
void bl31_early_platform_setup2(u_register_t arg0, u_register_t arg1,
				u_register_t arg2, u_register_t arg3)
{
	static console_t console;

	params_early_setup(arg1);

	if (rockchip_get_uart_base() != 0)
		console_16550_register(rockchip_get_uart_base(),
				       rockchip_get_uart_clock(),
				       rockchip_get_uart_baudrate(), &console);

	VERBOSE("bl31_setup\n");

	bl31_rmm_params_parse_helper(arg0, &bl32_ep_info, &bl33_ep_info, &rmm_ep_info);
}

/*******************************************************************************
 * Perform any BL3-1 platform setup code
 ******************************************************************************/
void bl31_platform_setup(void)
{
	generic_delay_timer_init();
	plat_rockchip_soc_init();

	/* Initialize the gic cpu and distributor interfaces */
	plat_rockchip_gic_driver_init();
	plat_rockchip_gic_init();
	plat_rockchip_pmu_init();
}

/*******************************************************************************
 * Perform the very early platform specific architectural setup here. At the
 * moment this is only initializes the mmu in a quick and dirty way.
 ******************************************************************************/
void bl31_plat_arch_setup(void)
{
	plat_cci_init();
	plat_cci_enable();

	VERBOSE("BL31_START=%lx\n", BL31_START);
	VERBOSE("BL31_END=%lx\n", BL31_END);
	VERBOSE("BL_CODE_BASE=%lx\n", BL_CODE_BASE);
	VERBOSE("BL_CODE_END=%lx\n", BL_CODE_END);


#if USE_COHERENT_MEM
	plat_configure_mmu_el3(BL_CODE_BASE,
			       BL_COHERENT_RAM_END - BL_CODE_BASE,
			       BL_CODE_BASE,
			       BL_CODE_END,
			       BL_COHERENT_RAM_BASE,
			       BL_COHERENT_RAM_END);
#else
	plat_configure_mmu_el3(BL31_START,
			       BL31_END - BL31_START,
			       BL_CODE_BASE,
			       BL_CODE_END,
			       0,
			       0);
#endif
}
