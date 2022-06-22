#include "mmu_def.h"

.section ".text.boot"

.global _start

_start:
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f
    // if cpu id > 0, stop
1:  wfe
    b       1b
2:  // else if cpu id == 0
    bl      from_el2_to_el1                 // set the kernel from el2 to el1
    // bl      setup_vmem

set_exception_vector_table:                 // set the exception vector table
    adr     x2, exception_vector_table
    msr     vbar_el1, x2     

    // set the stack pointer just before out code
    ldr     x1, =_start
    mov     sp, x1

    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
clear_bss_loop:  
    cbz     w2, main_loop
    str     xzr, [x1], #8    // set content in [x1] to zero
    sub     w2, w2, #1       // decrease bss size 8 bytes
    cbnz    w2, clear_bss_loop

    // jump to C code
main_loop:  
    // mov     x0, x10
    // ldr     x1, =KERNEL_VIRT_BASE
    // add     x0, x0, x1
    bl      main
    // for failsafe, halt this core too
    b       1b

.global from_el2_to_el1
from_el2_to_el1:
    mov     x2, (1 << 31) // EL1 uses aarch64
    msr     hcr_el2, x2 // The Execution state for EL1 is AArch64.
    mov     x2, 0x345 // EL1h (sp_el1) with interrupt enable 
    msr     spsr_el2, x2 // save PSTATE
    msr     elr_el2, lr // return address
    eret    // return to EL1

.global from_el1_to_el0
from_el1_to_el0:
    msr     elr_el1, x0
    msr     sp_el0, x1
    mov     sp, x2
    mov     x1, 0x340
    msr     spsr_el1, x1
    eret

.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:

    // set all handlers which are not going to be implemented to exception_invalid_handler

    b exception_invalid_handler // branch to a handler function.
    .align 7					// entry size is 0x80, .align will pad 0, 0x80 = 2^7
    b exception_invalid_handler
    .align 7
    b exception_invalid_handler
    .align 7
    b exception_invalid_handler
    .align 7
	
	// Exception from current EL while using SP_ELX (ex. EL1 -> EL1)
	// only handler irq

    b exception_invalid_handler
    .align 7
    b currentEL_irq_handler
    .align 7
    b exception_invalid_handler
    .align 7
    b exception_invalid_handler
    .align 7
	
	// Exception from a lower and at least one lower EL is AArch64 (ex. EL0 -> EL1)
	// only handler Sync & irq

    b lowerEL_sync_handler
    .align 7
    b lowerEL_irq_handler
    .align 7
    b exception_invalid_handler
    .align 7
    b exception_invalid_handler
    .align 7
	
	// Exception from a lower and all lower ELs are AArch32

    b exception_invalid_handler
    .align 7
    b exception_invalid_handler
    .align 7
    b exception_invalid_handler
    .align 7
    b exception_invalid_handler
    .align 7


.macro save_all
    sub sp, sp, 16 * 17
    stp x0, x1, [sp ,16 * 0]
    stp x2, x3, [sp ,16 * 1]
    stp x4, x5, [sp ,16 * 2]
    stp x6, x7, [sp ,16 * 3]
    stp x8, x9, [sp ,16 * 4]
    stp x10, x11, [sp ,16 * 5]
    stp x12, x13, [sp ,16 * 6]
    stp x14, x15, [sp ,16 * 7]
    stp x16, x17, [sp ,16 * 8]
    stp x18, x19, [sp ,16 * 9]
    stp x20, x21, [sp ,16 * 10]
    stp x22, x23, [sp ,16 * 11]
    stp x24, x25, [sp ,16 * 12]
    stp x26, x27, [sp ,16 * 13]
    stp x28, x29, [sp ,16 * 14]
    str x30, [sp, 16 * 15]

    // saving el registers
    mrs     x0, SPSR_EL1
    str     x0, [sp, 16 * 15 + 8]
    mrs     x0, ELR_EL1
    mrs     x1, SP_EL0
    stp     x0, x1, [sp, 16 * 16]
.endm

// load general registers from stack
.macro load_all
    // load el registers
    ldr     x0, [sp, 16 * 15 + 8]
    msr     SPSR_EL1, x0
    ldp     x0, x1, [sp, 16 * 16]
    msr     ELR_EL1, x0
    msr     SP_EL0, x1

    ldp x0, x1, [sp ,16 * 0]
    ldp x2, x3, [sp ,16 * 1]
    ldp x4, x5, [sp ,16 * 2]
    ldp x6, x7, [sp ,16 * 3]
    ldp x8, x9, [sp ,16 * 4]
    ldp x10, x11, [sp ,16 * 5]
    ldp x12, x13, [sp ,16 * 6]
    ldp x14, x15, [sp ,16 * 7]
    ldp x16, x17, [sp ,16 * 8]
    ldp x18, x19, [sp ,16 * 9]
    ldp x20, x21, [sp ,16 * 10]
    ldp x22, x23, [sp ,16 * 11]
    ldp x24, x25, [sp ,16 * 12]
    ldp x26, x27, [sp ,16 * 13]
    ldp x28, x29, [sp ,16 * 14]
    ldr x30, [sp, 16 * 15]
    add sp, sp, 16 * 17
.endm

exception_invalid_handler:
    save_all
    bl no_exception_handle
    load_all
    eret

currentEL_irq_handler:
    save_all
    bl currentEL_irq_router
    load_all
    eret

lowerEL_irq_handler:
    save_all
    bl lowerEL_irq_router
    load_all
    eret

lowerEL_sync_handler:
    save_all
    mov x0, sp
    bl lowerEL_sync_interrupt_handle
    load_all
    eret

.global switch_to
switch_to:
    stp x19, x20, [x0, 16 * 0]
    stp x21, x22, [x0, 16 * 1]
    stp x23, x24, [x0, 16 * 2]
    stp x25, x26, [x0, 16 * 3]
    stp x27, x28, [x0, 16 * 4]
    stp fp, lr, [x0, 16 * 5]
    mov x9, sp
    str x9, [x0, 16 * 6]

    ldp x19, x20, [x1, 16 * 0]
    ldp x21, x22, [x1, 16 * 1]
    ldp x23, x24, [x1, 16 * 2]
    ldp x25, x26, [x1, 16 * 3]
    ldp x27, x28, [x1, 16 * 4]
    ldp fp, lr, [x1, 16 * 5]
    ldr x9, [x1, 16 * 6]
    mov sp,  x9
    msr tpidr_el1, x1

.global get_current
get_current:
    mrs x0, tpidr_el1
    ret

.global store_context
store_context:
    stp x19, x20, [x0, 16 * 0]
    stp x21, x22, [x0, 16 * 1]
    stp x23, x24, [x0, 16 * 2]
    stp x25, x26, [x0, 16 * 3]
    stp x27, x28, [x0, 16 * 4]
    stp fp, lr, [x0, 16 * 5]
    mov x9, sp
    str x9, [x0, 16 * 6]
    ret

.global load_context
load_context:
    ldp x19, x20, [x0, 16 * 0]
    ldp x21, x22, [x0, 16 * 1]
    ldp x23, x24, [x0, 16 * 2]
    ldp x25, x26, [x0, 16 * 3]
    ldp x27, x28, [x0, 16 * 4]
    ldp fp, lr, [x0, 16 * 5]
    ldr x9, [x0, 16 * 6]
    mov sp,  x9
    ret

.global sys_get_pid
sys_get_pid:
    mov x8, 0
    svc 0
    ret

.global sys_uart_read
sys_uart_read:
    mov x8, 1
    svc 0
    ret

.global sys_uart_write
sys_uart_write:
    mov x8, 2
    svc 0
    ret

.global sys_exec
sys_exec:
    mov x8, 3
    svc 0
    ret

.global sys_fork
sys_fork:
    mov x8, 4
    svc 0
    ret

.global sys_exit
sys_exit:
    mov x8, 5
    svc 0
    ret

.global sys_mbox_call
sys_mbox_call:
    mov x8, 6
    svc 0
    ret

.global sys_kill
sys_kill:
    mov x8, 7
    svc 0
    ret

.global sys_ret
sys_ret:
    mov x8, 10
    svc 0
    ret

setup_vmem:
    // setup TCR
    ldr x1, = TCR_CONFIG_DEFAULT
    msr tcr_el1, x1

    //setup MAIR
    ldr x1, = MAIR_CONFIG
    msr mair_el1, x1

    ldr x0, =PGD_BASE
    ldr x1, =PUD_BASE
    ldr x2, =PMD_BASE
    ldr x3, =PTE_BASE

    msr ttbr0_el1, x0 // load PGD to the bottom translation-based register.
    msr ttbr1_el1, x0 // also load PGD to the upper translation based register.

    // set PGD
    ldr x4, = BOOT_PGD_ATTR
    orr x4, x1, x4                      // combine the physical address of next level page with attribute.
    str x4, [x0]

    // set 1st entry of PUD 
    ldr x4, = BOOT_PUD_ATTR
    mov x5, x2
    orr x6, x5, x4                      // combine the physical address of next level page with attribute.
    str x6, [x1]
    // set 2nd entry of PUD 
    add x5, x5, #0x1000
    orr x6, x5, x4
    str x6, [x1, 8]

    // set PMD
    ldr x4, = BOOT_PMD_ATTR
    mov x5, x2                          // addr of PMD
    mov x6, x3                          // addr of PTE
    mov x7, #(512 * 2)                  // 2 PMD table, 1024 entries
setup_pmd:
    orr x8, x6, x4                      // PTE table | ATTR
    str x8, [x5]
    add x5, x5, #8                      // next PMD entry
    add x6, x6, #0x1000                   // next PTE table
    sub x7, x7, #1
    cbnz x7, setup_pmd

    // set PTE
    ldr x4, = BOOT_PTE_RAM_ATTR
    mov x5, #0x00000000
    mov x6, x3                          // addr of PTE
    mov x7, #(1024 * 512)
    ldr x8, = PERIPHERAL_BASE
    
setup_pte_loop:
    cmp x5, x8
    blt setup_pte
    ldr x4, = BOOT_PTE_DEVICE_ATTR

setup_pte:
    orr x9, x5 ,x4
    str x9, [x6]                         // 4KB page
    add x6, x6, #8                       // next PTE entry
    add x5, x5, #4096                    // next phy page
    sub x7, x7, #1
    cbnz x7, setup_pte_loop

    // Enable MMU
    mrs x2, sctlr_el1
    orr x2 , x2, 1
    msr sctlr_el1, x2                    // enable MMU, cache remains disabled

    ldr x2, = set_exception_vector_table // indirect branch to the virtual address
    br  x2

.global switch_pgd
switch_pgd:
    dsb ish                              // ensure write has completed
    msr ttbr0_el1, x0                    // switch translation based address.
    tlbi vmalle1is                       // invalidate all TLB entries
    dsb ish                              // ensure completion of TLB invalidatation
    isb                                  // clear pipeline
    ret