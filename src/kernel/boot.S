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

set_exception_vector_table:                 // set the exception vector table
    adr     x2, exception_vector_table
    msr     vbar_el1, x2     

    // set the stack pointer just before out code
    ldr     x1, =_start
    mov     sp, x1

    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
3:  cbz     w2, main_loop
    str     xzr, [x1], #8    // set content in [x1] to zero
    sub     w2, w2, #1       // decrease bss size 8 bytes
    cbnz    w2, 3b

    // jump to C code
main_loop:  
    bl      main
    // for failsafe, halt this core too
    b       1b

from_el2_to_el1:
    mov     x2, (1 << 31) // EL1 uses aarch64
    msr     hcr_el2, x2 // The Execution state for EL1 is AArch64.
    mov     x2, 0x345 // EL1h (sp_el1) with interrupt enable 
    msr     spsr_el2, x2 // save PSTATE
    msr     elr_el2, lr // return address
    eret    // return to EL1


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
    sub sp, sp, 32 * 8
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
.endm

// load general registers from stack
.macro load_all
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
    add sp, sp, 32 * 8
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
    bl lowerEL_sync_interrupt_handle
    load_all
    eret