#ifndef OPENCCA_RME_QUIRKS_ASM_H
#define OPENCCA_RME_QUIRKS_ASM_H

#if ENABLE_OPENCCA
#ifdef __ASSEMBLER__


#include <arch.h>
#include <asm_macros.S>
#include <assert_macros.S>
#include <context.h>
#include <lib/xlat_tables/xlat_tables_defs.h>

/* -----------------------------------------------------------------
* The below macro returns the address of the per_world context for
* the security state, retrieved through "get_security_state" macro.
* The per_world context address is returned in the register argument.
* Clobbers: x9, x10, x11
* ------------------------------------------------------------------
*/

.macro get_security_state_spoofed _ret:req, _scr_reg:req, _scr_spoofed_reg:req
		ubfx 	\_ret, \_scr_spoofed_reg, #SCR_NSE_SHIFT, #1
		cmp 	\_ret, #1
		beq 	realm_state
		bfi	\_ret, \_scr_reg, #0, #1
		b 	end
	realm_state:
		mov 	\_ret, #2
	end:
.endm

.macro get_per_world_context_spoofed _reg:req
    /* 
	 * Only NSE bit is valid in CTX_SCR_EL3_SPOOFED.
	 * Read NS bit from CTX_SCR_EL3.
	 */
	ldr 	x10, [sp, #CTX_EL3STATE_OFFSET + CTX_SCR_EL3]
	ldr 	x11, [sp, #CTX_EL3STATE_OFFSET + CTX_SCR_EL3_SPOOFED]
	get_security_state_spoofed x9, x10, x11
	mov_imm	x10, (CTX_PERWORLD_EL3STATE_END - CTX_CPTR_EL3)
	mul	x9, x9, x10
	adrp	x10, per_world_context
	add	x10, x10, :lo12:per_world_context
	add	x9, x9, x10
	mov 	\_reg, x9
.endm



#endif /* __ASSEMBLER__ */
#endif
#endif /* OPENCCA_RME_QUIRKS_ASM_H */