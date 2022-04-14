#include "exception.h"

void no_exception_handle() {
    return;
}

void lowerEL_sync_interrupt_handle() {

    // get reg info
    unsigned long spsr_el1, elr_el1, esr_el1;
    asm volatile("mrs %0, spsr_el1" : "=r"(spsr_el1));
	asm volatile("mrs %0, elr_el1" : "=r"(elr_el1));
	asm volatile("mrs %0, esr_el1" : "=r"(esr_el1));

    // print regs' info
    uart_puts("SPSR_EL1: 0x");
    uart_hex(spsr_el1);
    uart_puts("\r\n");

    uart_puts("ELR_EL1: 0x");
    uart_hex(elr_el1);
    uart_puts("\r\n");

    uart_puts("ESR_EL1: 0x");
    uart_hex(esr_el1);
    uart_puts("\r\n");

    return;
}

void lowerEL_irq_router() {
    unsigned int core_irq = (*CORE0_INTR_SRC & (1 << 1));
    unsigned int uart_irq = (*IRQ_PENDING_1 & AUX_IRQ);

    if (core_irq) {
        core_timer_interrupt();
    }
    else if (uart_irq) {
        mini_uart_interrupt_handler();
    }
}

void currentEL_irq_router() {
    unsigned int core_irq = (*CORE0_INTR_SRC & (1 << 1));
    unsigned int uart_irq = (*IRQ_PENDING_1 & AUX_IRQ);

    if (core_irq) {
        timeout_handler();
    }
    else if (uart_irq) {
        mini_uart_interrupt_handler();
    }
}

void enable_interrupt(bool enable) {
    if (enable) {
        // set {D, A, I, F} interrupt mask bits to 0
        asm volatile ("msr DAIFClr, 0xf");
    }
    else {
        // set {D, A, I, F} interrupt mask bits to 1
        asm volatile ("msr DAIFSet, 0xf");
    }
}

interrupt_task *interrupt_task_list;

void irq_router() {
    interrupt_task *new_task = NULL;
    void (*handler) () = NULL;

    /* critical section */
    enable_interrupt(false);
    unsigned int core_irq = (*CORE0_INTR_SRC & (1 << 1));
    unsigned int uart_irq = (*IRQ_PENDING_1 & AUX_IRQ);

    // irq sources
    if (core_irq) {
        handler = timeout_handler;
    }
    else if (uart_irq) {
        handler = mini_uart_interrupt_handler;
    }
}