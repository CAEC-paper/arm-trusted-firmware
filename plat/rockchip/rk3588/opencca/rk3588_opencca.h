#ifndef __RK3588_OPENCCA_H__
#define __RK3588_OPENCCA_H__

/*
 *
 * The rk3588 can be configured with 4, 8, 16, 32 GB RAM.
 * To enforce the same memory layout across several configurations, we 
 * place RME related memory at the beginning of the first DRAM bank and below 4 GB.
 * U-Boot reserves the first 32 MB for TFA. We place RME related memory after that.
 * 
 *   - L1 GPT DRAM: Reserved for L1 GPT if RME is enabled
 *   - TF-A <-> RMM SHARED: Area shared for communication between TF-A and RMM
 *   - REALM DRAM: Reserved for Realm world if RME is enabled 
 * 
 *                    DRAM Bank 1
 *               +------------------+ -> TFA Reserve (32 MB)
 *               |   TFA BL31       |
 *               |   (0 - 32 MB)    |
 *               +------------------+ -> RMM Reserve (256 MB)
 *               |   TFA-<-> RMM    | 
 *               |   SHARED (4KB)   | 
 *               +------------------+ 
 *               |   REALM (RMM)    |
 *               |   (32 MB)        | 
 *               +------------------+ 
 *               |   L0 + L1 GPT    |
 *               |                  |
 *               +------------------+
 *               |                  |
 *               |   ......         |  
 *               +------------------+ <- u-boot bl33 relocates before here
 *     DRAM1 End |     0xf0000000.  | <- Devices
 *               |  to 0x100000000  |
 *               | mmio before 4GB  |
 *               +------------------+
 */

/*******************************************************************************
 * DRAM bank specific defines.
 ******************************************************************************/
#define ARM_DRAM_NUM_BANKS (2)

/* ARM_DRAM2_SIZE_MAX: compile-time upper bound (worst-case 16 GB config).
 * rk3588_detect_dram2_size() reads PMU1GRF OS regs at runtime and returns
 * the true size; RMM_MAX_GRANULES must be sized for this maximum. */
#define ARM_DRAM1_BASE			UL(0x0)
#define ARM_DRAM1_SIZE			UL(0xf0000000)
#define ARM_DRAM1_END			(ARM_DRAM1_BASE +		\
					 ARM_DRAM1_SIZE - 1U)

#define ARM_DRAM2_BASE			ULL(0x100000000)
#define ARM_DRAM2_SIZE_MAX		ULL(0x300000000)  /* 16-GB board: 12 GB bank2 */
/* ARM_DRAM2_SIZE equals the max; the NS_RAM1 PAS entry is patched at runtime. */
#define ARM_DRAM2_SIZE			ARM_DRAM2_SIZE_MAX
#define ARM_DRAM2_END			(ARM_DRAM2_BASE +		\
					 ARM_DRAM2_SIZE_MAX - 1U)

/* devices on dram0 bank*/
#define ARM_DEVICES0_BASE UL(0xf0000000)
#define ARM_DEVICES0_SIZE UL(0x10000000)

/* U-Boot reserves 32 MB for TFA */
#define ARM_DRAM_BL31_RESERVE_BASE UL(0x0)
#define ARM_DRAM_BL31_RESERVE_SIZE UL(0x200000)

/* U-Boot reserves additional 256 MB for RME emulation */
#define ARM_DRAM_RME_RESERVE_BASE (ARM_DRAM_BL31_RESERVE_BASE \
								 + ARM_DRAM_BL31_RESERVE_SIZE)

#define ARM_DRAM_RME_RESERVE_SIZE UL(0x10000000)

/* Ns memory */
/* Both DRAM banks are exposed to RMM so realm memory can span above 4 GB */
#define RMM_NS_DRAM_BANKS (2)

#define RMM_NS_RAM0_BASE (ARM_DRAM_RME_RESERVE_SIZE + ARM_DRAM_BL31_RESERVE_SIZE)
#define RMM_NS_RAM0_SIZE (ARM_DEVICES0_BASE - RMM_NS_RAM0_BASE)

#define RMM_NS_RAM1_BASE ARM_DRAM2_BASE
/* RMM_NS_RAM1_SIZE is the compile-time max; rk3588_opencca.c patches the
 * ARM_PAS_NS_RAM1 entry and the boot manifest with the detected runtime size. */
#define RMM_NS_RAM1_SIZE ARM_DRAM2_SIZE_MAX

#ifndef __ASSEMBLER__
/* Detect bank-2 (above 4 GB) size from PMU1GRF OS registers at runtime.
 * Returns total_physical_dram - 4 GB; result is 0 if <= 4 GB. */
uint64_t rk3588_detect_dram2_size(void);
#endif /* __ASSEMBLER__ */


/*******************************************************************************
 * "RMM TF-A shared region" specific defines.
 ******************************************************************************/

/* PLAT_ARM_EL3_RMM_SHARED_SIZE */
#define ARM_EL3_RMM_SHARED_SIZE		(PAGE_SIZE)    /* 4KB */

#define ARM_EL3_RMM_SHARED_BASE		(ARM_DRAM_BL31_RESERVE_BASE +		\
					 ARM_DRAM_BL31_RESERVE_SIZE)

#define ARM_EL3_RMM_SHARED_END		(ARM_EL3_RMM_SHARED_BASE +	\
					 ARM_EL3_RMM_SHARED_SIZE - 1U)

/*******************************************************************************
 * RMM specific defines.
 ******************************************************************************/

#define RMM_UART_BASE 			RK_DBG_UART_BASE
#define RMM_UART_CLK_IN_HZ 	RK_DBG_UART_CLOCK
#define RMM_CONSOLE_BAUDRATE 			RK_DBG_UART_BAUDRATE
#define RMM_CONSOLE_NAME "pl011"


/* ARM_REALM_SIZE: sized for 16-GB board (RMM_MAX_GRANULES=0x400000).
 * Covers RMM binary + 8 MB granule table. */
#define ARM_REALM_SIZE			(UL(0x03200000) -		\
					 ARM_EL3_RMM_SHARED_SIZE)

#define ARM_REALM_BASE			(ARM_EL3_RMM_SHARED_BASE +	\
					 ARM_EL3_RMM_SHARED_SIZE)

#define ARM_REALM_END			(ARM_REALM_BASE + ARM_REALM_SIZE - 1U)

#define RMM_BASE			(ARM_REALM_BASE)
#define RMM_LIMIT			(RMM_BASE + ARM_REALM_SIZE)
#define RMM_SHARED_BASE			(ARM_EL3_RMM_SHARED_BASE)
#define RMM_SHARED_SIZE			(ARM_EL3_RMM_SHARED_SIZE)

#if 0
#define XSTR(x) STR(x)
#define STR(x) #x
#pragma message "The value of RMM_BASE: " XSTR(RMM_BASE)
#endif

/*******************************************************************************
 * L0 GPT specifics
 ******************************************************************************/
/*
 * Assumption we make about GPTs for rk3588:
 * - PPS (Protected Physical Space), GPCCR_PPS_64GB: 64 GB, 0x1000000000 bytes
 * - PGS (Physical Granule Size): 4 KB
 * - L0GPTSZ: GPCCR_L0GPTSZ_30BITS, (1GB regions, 0x40000000 bytes)
 * - RME_GPT_BITLOCK_BLOCK: 1
 * 
 * L0 table size:
 *   PPS / L0GPTSZ * 8 = 0x1000000000 / 0x40000000 * 8 = 512 bytes -> 0x1000 bytes
 *   This must be aligned to either the table size or 4096 bytes, whatever is larger.
 * 
 * L0 entries: 
 *   PPS / L0GPTSZ = 0x1000000000 / 0x40000000 = 64 entries
 *   Bitlocks at the end of L0 table, e.g.:
 * 	 gpt_bitlock_base = (bitlock_t *)(gpt_config.plat_gpt_l0_base +
 *					GPT_L0_TABLE_SIZE(gpt_config.t));
 * 
 * Bitlock size for L1:
 *   PPS / (RME_GPT_BITLOCK_BLOCK * 0x20000000 * 8) = 
 *   0x1000000000 / (1 * 0x20000000 * 8) = 0x10 bytes
 * 
 * L1 table size:
 *    ((L0GPTSZ / PGS) / 2) = 0x40000000 / 0x1000 / 2 = 0x20000 bytes
 * 
*/

#define PLAT_OPENCCA_L0GPTSZ (GPCCR_L0GPTSZ_30BITS)
#define PLAT_OPENCCA_PGS (GPCCR_PGS_4K)
#define PLAT_OPENCCA_PPS (GPCCR_PPS_64GB)
#define PLAT_OPENCCA_GPC (1)
#define PLAT_OPENCCA_GPCP (0)
#define PLAT_OPENCCA_SH GPCCR_SH_IS
#define PLAT_OPENCCA_ORGN GPCCR_ORGN_NC
#define PLAT_OPENCCA_IRGN GPCCR_IRGN_NC

#define PLAT_OPENCCA_GPTBR_EL3 (ARM_L0_GPT_BASE)

#define PLAT_OPENCCA_GPCCR_EL3 \
	((PLAT_OPENCCA_L0GPTSZ << GPCCR_L0GPTSZ_SHIFT) | \
	(PLAT_OPENCCA_GPC << GPCCR_GPC_SHIFT) | \
	(PLAT_OPENCCA_GPCP << GPCCR_GPCP_SHIFT)| \
	(PLAT_OPENCCA_PGS << GPCCR_PGS_SHIFT) | \
	(PLAT_OPENCCA_SH << GPCCR_SH_SHIFT) | \
	(PLAT_OPENCCA_ORGN << GPCCR_ORGN_SHIFT)| \
	(PLAT_OPENCCA_IRGN << GPCCR_IRGN_SHIFT)| \
	(PLAT_OPENCCA_PPS << GPCCR_PPS_SHIFT));


#define ARM_L0_GPT_SIZE			UL(0x2000) /* 8 KB */
#define ARM_L0_GPT_BASE			(ARM_REALM_BASE + ARM_REALM_SIZE) 
#define ARM_L0_GPT_LIMIT		(ARM_L0_GPT_BASE + ARM_L0_GPT_SIZE)


/*******************************************************************************
 * L1GPT specific defines.
 ******************************************************************************/
#ifndef ALIGN_UP
#define ALIGN_UP(num, align)	(((num) + ((align) - 1)) & ~((align) - 1))
#endif

#define ARM_L1_GPT_SIZE			UL(64 * 0x20000) 

/* must be aligned to its size */
#define ARM_L1_GPT_BASE			ALIGN_UP((ARM_L0_GPT_BASE + ARM_L0_GPT_SIZE), ARM_L1_GPT_SIZE)

#define ARM_L1_GPT_END			(ARM_L1_GPT_BASE +		\
					 ARM_L1_GPT_SIZE - 1U)


#if ARM_L1_GPT_END > (ARM_DRAM_RME_RESERVE_BASE + ARM_DRAM_RME_RESERVE_SIZE)
    #error "ARM_L1_GPT_END cannot exceed the available reserve memory for RME"
#endif

/*******************************************************************************
 * RME MMU mappings for EL3, see plat/rockchip/rk3588/drivers/soc/soc.c
 ******************************************************************************/

#define ARM_MAP_L0_GPT_REGION		MAP_REGION_FLAT(ARM_L0_GPT_BASE,	\
						ARM_L0_GPT_SIZE,		\
						MT_MEMORY | MT_RW | MT_ROOT)


#define ARM_MAP_RMM_DRAM	MAP_REGION_FLAT(			\
					ARM_REALM_BASE,		\
					(ARM_REALM_SIZE),	\
					MT_MEMORY | MT_RW | MT_REALM)

#define ARM_MAP_GPT_L1_DRAM	MAP_REGION_FLAT(			\
					ARM_L1_GPT_BASE,		\
					ARM_L1_GPT_SIZE,		\
					MT_MEMORY | MT_RW | MT_ROOT)

#define ARM_MAP_EL3_RMM_SHARED_MEM					\
				MAP_REGION_FLAT(			\
					ARM_EL3_RMM_SHARED_BASE,	\
					ARM_EL3_RMM_SHARED_SIZE,	\
					MT_MEMORY | MT_RW | MT_REALM)




/*****************************************************************************
 * PAS regions used to initialize the Granule Protection Table (GPT)
 ****************************************************************************/

/* XXX: The following mappings can be improved,
 * they cover BL31 mem, RMM, devices and marking the rest as NS 
 */

#define ARM_PAS_EL3_DRAM		GPT_MAP_REGION_GRANULE(ARM_DRAM_BL31_RESERVE_BASE, \
							       ARM_DRAM_BL31_RESERVE_SIZE, \
							       GPT_GPI_ROOT)

#define ARM_PAS_REALM_SHARED_REGION		GPT_MAP_REGION_GRANULE(ARM_EL3_RMM_SHARED_BASE, \
							       ARM_EL3_RMM_SHARED_SIZE, \
							       GPT_GPI_REALM)

#define ARM_PAS_REALM			GPT_MAP_REGION_GRANULE(ARM_REALM_BASE, \
								ARM_REALM_SIZE, \
							       GPT_GPI_REALM)


#define	ARM_PAS_GPT_L0   		GPT_MAP_REGION_GRANULE(ARM_L0_GPT_BASE, \
							       ARM_L0_GPT_SIZE, \
							       GPT_GPI_ROOT)


#define	ARM_PAS_GPT_L1   		GPT_MAP_REGION_GRANULE(ARM_L1_GPT_BASE, \
							       ARM_L1_GPT_SIZE, \
							       GPT_GPI_ROOT)


#define ARM_PAS_DEV				GPT_MAP_REGION_GRANULE(ARM_DEVICES0_BASE, \
									ARM_DEVICES0_SIZE, \
									GPT_GPI_ANY)

#define	ARM_PAS_NS_RAM0			GPT_MAP_REGION_GRANULE(RMM_NS_RAM0_BASE, \
							       RMM_NS_RAM0_SIZE, \
							       GPT_GPI_NS)

#define	ARM_PAS_NS_RAM1			GPT_MAP_REGION_GRANULE(RMM_NS_RAM1_BASE, \
							       RMM_NS_RAM1_SIZE, \
							       GPT_GPI_NS)


#endif