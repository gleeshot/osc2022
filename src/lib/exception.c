#include "exception.h"

void no_exception_handle() {
    return;
}

void lowerEL_sync_interrupt_handle(trap_frame_t *trap_frame) {

    // get reg info
    // unsigned long spsr_el1, elr_el1, esr_el1;
    // asm volatile("mrs %0, spsr_el1" : "=r"(spsr_el1));
	// asm volatile("mrs %0, elr_el1" : "=r"(elr_el1));
	// asm volatile("mrs %0, esr_el1" : "=r"(esr_el1));

    // print regs' info
    // uart_puts("SPSR_EL1: 0x");
    // uart_hex(spsr_el1);
    // uart_puts("\r\n");

    // uart_puts("ELR_EL1: 0x");
    // uart_hex(elr_el1);
    // uart_puts("\r\n");

    // uart_puts("ESR_EL1: 0x");
    // uart_hex(esr_el1);
    // uart_puts("\r\n");

    uint32_t syscall_num = trap_frame->regs[8];
    uint32_t ret;
    // if (syscall_num < 11)
    // {
    //     uart_hex(syscall_num);
    //     uart_puts("\r\n");
    // }
    enable_interrupt(true);

    switch (syscall_num)
    {
        case 0:
            ret = getpid();
            trap_frame->regs[0] = ret;
            break;
        case 1:
            ret = uart_read((char *)trap_frame->regs[0], (size_t)trap_frame->regs[1]);
            trap_frame->regs[0] = ret;
            break;
        case 2:
            ret = uart_write((char *)trap_frame->regs[0], (size_t)trap_frame->regs[1]);
            trap_frame->regs[0] = ret;
            break;
        case 3:
            ret = (uint32_t)_exec(trap_frame, (char *)trap_frame->regs[0], (char **)trap_frame->regs[1]);
            trap_frame->regs[0] = ret;
            break;
        case 4:
            ret = (uint32_t)copy_process(trap_frame);
            // trap_frame->regs[0] = ret;
            break;
        case 5:
            exit();
            break;
        case 6:
            ret = _mbox_call((mail_t *)trap_frame->regs[1],(unsigned char)trap_frame->regs[0]);
            trap_frame->regs[0] = ret;
            break;
        case 7:
            kill((pid_t)trap_frame->regs[0]);
            break;
        case 8:
            thread_signal_register(trap_frame->regs[0], trap_frame->regs[1]);
            break;
        case 9:
            thread_signal_kill(trap_frame->regs[0], trap_frame->regs[1]);
            break;
        case 10:
            thread_signal_return();
            break;
        default:
            uart_puts("[lower_syn_handler] unsupported syscall_num: ");
            uart_hex(syscall_num);
            uart_puts("\r\n");
            break;
    }
    
    return;
}

void lowerEL_irq_router() {
    unsigned int core_irq = (*CORE0_INTR_SRC & (1 << 1));
    unsigned int uart_irq = (*IRQ_PENDING_1 & AUX_IRQ);

    if (core_irq) {
        // core_timer_interrupt();
        timeout_handler();
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
        set_timer_interrupt(false);
    }
    else if (uart_irq) {
        handler = mini_uart_interrupt_handler;
    }
    enable_interrupt(true);
}