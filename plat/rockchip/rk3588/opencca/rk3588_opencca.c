#include <common/debug.h>
#include <common/runtime_svc.h>
#include <drivers/scmi-msg.h>

#include <plat_sip_calls.h>
#include <rockchip_sip_svc.h>
#include <services/rmm_core_manifest.h>
#include <rk3588_def.h>
#include <lib/fconf/fconf.h>

#include <common/debug.h>
#include <lib/gpt_rme/gpt_rme.h>
#include <plat/common/platform.h>

#if ENABLE_RME
/*
 * Get a pointer to the RMM-EL3 Shared buffer and return it
 * through the pointer passed as parameter.
 *
 * This function returns the size of the shared buffer.
 */
size_t plat_rmmd_get_el3_rmm_shared_mem(uintptr_t *shared)
{
	*shared = (uintptr_t)RMM_SHARED_BASE;

	return (size_t)RMM_SHARED_SIZE;
}

int plat_rmmd_load_manifest(struct rmm_manifest *manifest)
{
	uint64_t checksum, num_banks, num_consoles;
	struct ns_dram_bank *bank_ptr;
	struct console_info *console_ptr;

	assert(manifest != NULL);

	/* Get number of DRAM banks */
	num_banks = RMM_NS_DRAM_BANKS;
	assert(num_banks <= ARM_DRAM_NUM_BANKS);

	/* Set number of consoles */
	num_consoles = 1;

	manifest->version = RMMD_MANIFEST_VERSION;
	manifest->padding = 0U; /* RES0 */
	manifest->plat_data = (uintptr_t)NULL;
	manifest->plat_dram.num_banks = num_banks;
	manifest->plat_console.num_consoles = num_consoles;

	/*
	 * Boot Manifest structure illustration, with two dram banks and
	 * a single console.
	 *
	 * +----------------------------------------+
	 * | offset |     field      |    comment   |
	 * +--------+----------------+--------------+
	 * |   0    |    version     |  0x00000003  |
	 * +--------+----------------+--------------+
	 * |   4    |    padding     |  0x00000000  |
	 * +--------+----------------+--------------+
	 * |   8    |   plat_data    |     NULL     |
	 * +--------+----------------+--------------+
	 * |   16   |   num_banks    |              |
	 * +--------+----------------+              |
	 * |   24   |     banks      |   plat_dram  |
	 * +--------+----------------+              |
	 * |   32   |    checksum    |              |
	 * +--------+----------------+--------------+
	 * |   40   |  num_consoles  |              |
	 * +--------+----------------+              |
	 * |   48   |    consoles    | plat_console |
	 * +--------+----------------+              |
	 * |   56   |    checksum    |              |
	 * +--------+----------------+--------------+
	 * |   64   |     base 0     |              |
	 * +--------+----------------+    bank[0]   |
	 * |   72   |     size 0     |              |
	 * +--------+----------------+--------------+
	 * |   80   |     base 1     |              |
	 * +--------+----------------+    bank[1]   |
	 * |   88   |     size 1     |              |
	 * +--------+----------------+--------------+
	 * |   96   |     base       |              |
	 * +--------+----------------+              |
	 * |   104  |   map_pages    |              |
	 * +--------+----------------+              |
	 * |   112  |     name       |              |
	 * +--------+----------------+  consoles[0] |
	 * |   120  |   clk_in_hz    |              |
	 * +--------+----------------+              |
	 * |   128  |   baud_rate    |              |
	 * +--------+----------------+              |
	 * |   136  |     flags      |              |
	 * +--------+----------------+--------------+
	 */

	bank_ptr = (struct ns_dram_bank *)
			(((uintptr_t)manifest) + sizeof(*manifest));
	console_ptr = (struct console_info *)
			((uintptr_t)bank_ptr + (num_banks * sizeof(*bank_ptr)));

	manifest->plat_dram.banks = bank_ptr;
	manifest->plat_console.consoles = console_ptr;

	/* Ensure the manifest is not larger than the shared buffer */
	assert((sizeof(struct rmm_manifest) +
		(sizeof(struct console_info) * manifest->plat_console.num_consoles) +
		(sizeof(struct ns_dram_bank) * manifest->plat_dram.num_banks)) <= ARM_EL3_RMM_SHARED_SIZE);

	/* Calculate checksum of plat_dram structure */
	checksum = num_banks + (uint64_t)bank_ptr;

	bank_ptr[0].base = RMM_NS_RAM0_BASE;
	bank_ptr[0].size = RMM_NS_RAM0_SIZE;
	checksum += bank_ptr[0].base + bank_ptr[0].size;

	#if 0
	// XXX: For now we only pass one RAM base
	bank_ptr[1].base = RMM_NS_RAM1_BASE;
	bank_ptr[1].size = RMM_NS_RAM1_SIZE;
	checksum += bank_ptr[1].base + bank_ptr[1].size;
	#endif

	INFO("NS_RAM0_BASE: 0x%lx, NS_RAM0_SIZE: 0x%lx\n", bank_ptr[0].base, bank_ptr[0].size);

	/* Checksum must be 0 */
	manifest->plat_dram.checksum = ~checksum + 1UL;

	/* Calculate the checksum of the plat_consoles structure */
	checksum = num_consoles + (uint64_t)console_ptr;

	/* Zero out the console info struct */
	memset((void *)console_ptr, '\0', sizeof(struct console_info) * num_consoles);

	console_ptr[0].map_pages = 1;
	console_ptr[0].base = RMM_UART_BASE;
	console_ptr[0].clk_in_hz = RMM_UART_CLK_IN_HZ;
	console_ptr[0].baud_rate = RMM_CONSOLE_BAUDRATE;

	strlcpy(console_ptr[0].name, RMM_CONSOLE_NAME, RMM_CONSOLE_MAX_NAME_LEN-1UL);

	/* Update checksum */
	checksum += console_ptr[0].base + console_ptr[0].map_pages +
		console_ptr[0].clk_in_hz + console_ptr[0].baud_rate;

	/* Checksum must be 0 */
	manifest->plat_console.checksum = ~checksum + 1UL;

	return 0;
}

/*
 * The GPT library might modify the gpt regions structure to optimize
 * the layout, so the array cannot be constant.
 */
static pas_region_t pas_regions[] = {
	ARM_PAS_EL3_DRAM,
	ARM_PAS_REALM_SHARED_REGION,
	ARM_PAS_REALM,
	ARM_PAS_GPT_L0,
	ARM_PAS_GPT_L1,
	ARM_PAS_DEV,
	ARM_PAS_NS_RAM0,
	ARM_PAS_NS_RAM1,
};

typedef struct gpt_info {
	pas_region_t *pas_region_base;
	unsigned int pas_region_count;
	uintptr_t l0_base;
	uintptr_t l1_base;
	size_t l0_size;
	size_t l1_size;
	gpccr_pps_e pps;
	gpccr_pgs_e pgs;
} gpt_info_t;


static const gpt_info_t gpt_info = {
	.pas_region_base  = pas_regions,
	.pas_region_count = (unsigned int)ARRAY_SIZE(pas_regions),
	.l0_base = (uintptr_t)ARM_L0_GPT_BASE,
	.l1_base = (uintptr_t)ARM_L1_GPT_BASE,
	.l0_size = (size_t)ARM_L0_GPT_SIZE,
	.l1_size = (size_t)ARM_L1_GPT_SIZE,
	.pps = PLAT_OPENCCA_PPS,
	.pgs = PLAT_OPENCCA_PGS,
};


void rk_gpt_setup(void)
{
	VERBOSE("ARM_L1_GPT_BASE: %p\n", (void *)ARM_L1_GPT_BASE);

	/* Initialize entire protected space to GPT_GPI_ANY. */
	if (gpt_init_l0_tables(gpt_info.pps, gpt_info.l0_base,
		gpt_info.l0_size) < 0) {
		ERROR("gpt_init_l0_tables() failed!\n");
		panic();
	}

	/* Carve out defined PAS ranges. */
	if (gpt_init_pas_l1_tables(gpt_info.pgs,
				   gpt_info.l1_base,
				   gpt_info.l1_size,
				   gpt_info.pas_region_base,
				   gpt_info.pas_region_count) < 0) {
		ERROR("gpt_init_pas_l1_tables() failed!\n");
		panic();
	}

	INFO("Enabling Granule Protection Checks\n");
	if (gpt_enable() < 0) {
		ERROR("gpt_enable() failed!\n");
		panic();
	}

		if (gpt_runtime_init() < 0) {
		ERROR("gpt_runtime_init() failed!\n");
		panic();
	}

}

void bl31_plat_runtime_setup(void)
{
	rk_gpt_setup();

	// XXX: We are not using ROM lib on this board
}

#endif	/* ENABLE_RME */
