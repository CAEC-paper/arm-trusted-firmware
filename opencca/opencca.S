#ifndef OPENCCA_S
#define OPENCCA_S

#include <arch.h>
#include <asm_macros.S>
#include <assert_macros.S>
#include <context.h>
#include <lib/xlat_tables/xlat_tables_defs.h>

.globl opencca_tlbipaallos


func opencca_tlbipaallos    
    // TLB in Secure world
    tlbi alle3
    tlbi alle1
    dsb sy
    isb

    // Set NS bit to be in NS
    mrs x1, scr_el3
    mov_imm x2, SCR_NS_BIT
    orr x0, x1, x2
    msr scr_el3, x0
    isb

    // NS TLBs
    tlbi alle2
    tlbi alle1
    dsb sy
    isb

    // Restore SCR_EL3 to original value
    msr scr_el3, x1
    
    ret
endfunc opencca_tlbipaallos


#endif