/*
 * Copyright (c) 2021-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/bl_common.h>
#include <common/debug.h>
#include <drivers/arm/pl011.h>
#include <drivers/console.h>
#include <services/rmm_core_manifest.h>
#include <services/rmmd_svc.h>
#include <services/trp/platform_trp.h>
#include <trp_helpers.h>

#include <platform_def.h>

#include <drivers/console.h>
#include <drivers/ti/uart/uart_16550.h>


/*******************************************************************************
 * Received from boot manifest and populated here
 ******************************************************************************/
extern uint32_t trp_boot_manifest_version;

/*******************************************************************************
 * Initialize the UART
 ******************************************************************************/
static console_t trp_runtime_console;

static int rk_trp_process_manifest(struct rmm_manifest *manifest)
{
	/* padding field on the manifest must be RES0 */
	assert(manifest->padding == 0U);

	/* Verify the Boot Manifest Version. Only the Major is considered */
	if (RMMD_MANIFEST_VERSION_MAJOR !=
		RMMD_GET_MANIFEST_VERSION_MAJOR(manifest->version)) {
		return E_RMM_BOOT_MANIFEST_VERSION_NOT_SUPPORTED;
	}

	trp_boot_manifest_version = manifest->version;
	flush_dcache_range((uintptr_t)manifest, sizeof(struct rmm_manifest));

	return 0;
}

void rk_trp_early_platform_setup(struct rmm_manifest *manifest)
{
	int rc;

	rc = rk_trp_process_manifest(manifest);
	if (rc != 0) {
		trp_boot_abort(rc);
	}
	
	console_16550_register(RMM_UART_BASE,
					RMM_UART_CLK_IN_HZ,
					RMM_CONSOLE_BAUDRATE,
					&trp_runtime_console);
}


void trp_early_platform_setup(struct rmm_manifest *manifest)
{
	rk_trp_early_platform_setup(manifest);
}


void __dead2 rockchip_soc_soft_reset(void)
{
	while (1)
		;
}