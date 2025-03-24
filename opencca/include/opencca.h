#ifndef OPENCCA_RME_QUIRKS_H
#define OPENCCA_RME_QUIRKS_H

#define RMM_RMI_TEST				0xc40002df

#if ENABLE_OPENCCA

#ifndef __ASSEMBLER__
#include <stddef.h>
#include <stdint.h>
#include <lib/cassert.h>
#include <platform_def.h>
#include <opencca_debug.h>

#endif /* __ASSEMBLER__ */


/**********************************************************************
 * system register quirks
 *********************************************************************/

static inline u_register_t read_dummy_sysreg()
{
	u_register_t v=0;
	__asm__ volatile ("mrs %0, AFSR0_EL3" : "=r" (v));	
	return 0;
}

static inline void write_dummy_sysreg(u_register_t v)
{									\
	__asm__ volatile ("msr AFSR0_EL3, %0" : : "r" (v));
}


#define OPENCCA_GPCCR_EL3 PLAT_OPENCCA_GPCCR_EL3
#define OPENCCA_GPTBR_EL3 PLAT_OPENCCA_GPTBR_EL3

/*
* Granule Protection Check Control Register
* stores info about the hardawre such as address space 
* covered by GPC. We spoof this register with a dummy register access
* and return sane defaults.
* This implies that the configurations cannot be changed.
* Alternatively use AFSR*_EL3 register.
*/
#define read_gpccr_el3 read_gpccr_el3_spoofed
#define write_gpccr_el3 write_dummy_sysreg

static inline u_register_t read_gpccr_el3_spoofed(void)
{					
	read_dummy_sysreg();
	/*
	 * Return sane defaults for gptbr_el.
	   These align with the L0, L1 size assumption in rk3588 bl31 memory layout.
	   Assume FEAT_RME but no FEAT_RME_GPC2
	*/
	return OPENCCA_GPCCR_EL3;
}

/*
 * gptbr stores base address of l0 gpt.
 * We spoof this access with a hard coded platform default
*/
#define read_gptbr_el3 read_gptbr_el3_spoofed
#define write_gptbr_el3 write_dummy_sysreg

static inline u_register_t read_gptbr_el3_spoofed(void)
{
	read_dummy_sysreg();
	/*
	 * write_gptbr_el3(((gpt_config.plat_gpt_l0_base >> GPTBR_BADDR_VAL_SHIFT)
	 * >> GPTBR_BADDR_SHIFT) & GPTBR_BADDR_MASK);
	 */
	return (((OPENCCA_GPTBR_EL3 >> GPTBR_BADDR_VAL_SHIFT)
	 	>> GPTBR_BADDR_SHIFT) & GPTBR_BADDR_MASK);
}

/*
 * TLBI PAALLOS instruction
 * (TLB Invalidate GPT Information by PA, All Entries, Outer Shareable)
 */
void opencca_tlbipaallos(void);

/* Estimate this with TLB flush across all worlds/EL */
#define tlbipaallos opencca_tlbipaallos

/*
 * TLBI RPALOS instructions
 * (TLB Range Invalidate GPT Information by PA, Last level, Outer Shareable)
 *
 * command SIZE, bits [47:44] field:
 * 0b0000	4KB
 * 0b0001	16KB
 * 0b0010	64KB
 * 0b0011	2MB
 * 0b0100	32MB
 * 0b0101	512MB
 * 0b0110	1GB
 * 0b0111	16GB
 * 0b1000	64GB
 * 0b1001	512GB
 */
#define TLBI_SZ_4K		0UL
#define TLBI_SZ_16K		1UL
#define TLBI_SZ_64K		2UL
#define TLBI_SZ_2M		3UL
#define TLBI_SZ_32M		4UL
#define TLBI_SZ_512M		5UL
#define TLBI_SZ_1G		6UL
#define TLBI_SZ_16G		7UL
#define TLBI_SZ_64G		8UL
#define TLBI_SZ_512G		9UL

#define	TLBI_ADDR_SHIFT		U(12)
#define	TLBI_SIZE_SHIFT		U(44)

#define TLBIRPALOS(_addr, _size) tlbipaallos();

/* Note: addr must be aligned to 4KB */
static inline void tlbirpalos_4k(uintptr_t addr)
{
	TLBIRPALOS(addr, TLBI_SZ_4K);
}

/* Note: addr must be aligned to 16KB */
static inline void tlbirpalos_16k(uintptr_t addr)
{
	TLBIRPALOS(addr, TLBI_SZ_16K);
}

/* Note: addr must be aligned to 64KB */
static inline void tlbirpalos_64k(uintptr_t addr)
{
	TLBIRPALOS(addr, TLBI_SZ_64K);
}

/* Note: addr must be aligned to 2MB */
static inline void tlbirpalos_2m(uintptr_t addr)
{
	TLBIRPALOS(addr, TLBI_SZ_2M);
}

/* Note: addr must be aligned to 32MB */
static inline void tlbirpalos_32m(uintptr_t addr)
{
	TLBIRPALOS(addr, TLBI_SZ_32M);
}

/* Note: addr must be aligned to 512MB */
static inline void tlbirpalos_512m(uintptr_t addr)
{
	TLBIRPALOS(addr, TLBI_SZ_512M);
}

// No support for dc_cipapa_x0, dc_cigdpapa_x0, we do nop series
// TODO: Is there a better estimate?
static inline void flush_dcache_to_popa_range(uintptr_t addr, size_t size) {
	for(unsigned long i=0; i<size; i += 64) {
		// dccvac(addr + i);
		asm volatile ("nop");
	}
}
static inline void flush_dcache_to_popa_range_mte2(uintptr_t addr, size_t size) {
	for(unsigned long i=0; i<size; i += 64) {
		// dccvac(addr + i);
		asm volatile ("nop");
	}
}

/**********************************************************************
 * arch_features quirks
 *********************************************************************/

#define CREATE_FEATURE_SUPPORTED_SPOOFED(name, read_func, guard, result)			\
__attribute__((always_inline))							\
static inline bool is_ ## name ## _supported(void)				\
{		\
    read_dummy_sysreg(); \
    /* todo some bit manipulation in assembly */ \
	return result;							\
}

#define CREATE_FEATURE_PRESENT_SPOOFED(name, idreg, idfield, mask, idval, result)		\
__attribute__((always_inline))							\
static inline bool is_ ## name ## _present(void)				\
{										\
	return result; \
}

#define CREATE_FEATURE_FUNCS_TRUE(name, idreg, idfield, mask, idval, guard)		\
CREATE_FEATURE_PRESENT_SPOOFED(name, idreg, idfield, mask, idval, true)			\
CREATE_FEATURE_SUPPORTED_SPOOFED(name, is_ ## name ## _present, guard, true)


#define CREATE_FEATURE_FUNCS_FALSE(name, idreg, idfield, mask, idval, guard)		\
CREATE_FEATURE_PRESENT_SPOOFED(name, idreg, idfield, mask, idval, false)			\
CREATE_FEATURE_SUPPORTED_SPOOFED(name, is_ ## name ## _present, guard, false)



/* Return the RME version, zero if not supported. */
CREATE_FEATURE_FUNCS_TRUE(feat_rme, id_aa64pfr0_el1, ID_AA64PFR0_FEAT_RME_SHIFT,
		    ID_AA64PFR0_FEAT_RME_MASK, 1U, ENABLE_RME)


CREATE_FEATURE_FUNCS_FALSE(feat_gcs, id_aa64pfr1_el1, ID_AA64PFR1_EL1_GCS_SHIFT,
		     ID_AA64PFR1_EL1_GCS_MASK, 1U, ENABLE_FEAT_GCS)


CREATE_FEATURE_FUNCS_FALSE(feat_s2pie, id_aa64mmfr3_el1, ID_AA64MMFR3_EL1_S2PIE_SHIFT,
		     ID_AA64MMFR3_EL1_S2PIE_MASK, 1U, ENABLE_FEAT_S2PIE)


static inline void opencca_tlb_flush() {
	/* 
	 * We are multiplexing RMM and linux in normal world.
	 * Flush TLB upon context switch
	 */
	__asm__ volatile (
		"TLBI ALLE2 \n"
		"dsb ish \n"
		"isb \n");
}

#endif
#endif 
