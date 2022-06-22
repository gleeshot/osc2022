#ifndef __EXCEPTION__H__
#define __EXCEPTION__H__

#include "mini_uart.h"
#include "utils.h"
#include "timer.h"
#include "syscall.h"
#include "mbox.h"
#include "thread.h"
#include "mmu_def.h"

#define CORE0_INTR_SRC ((unsigned int *)(0x40000060))

typedef struct interrupt_task
{
    void (*handler)();
    struct interrupt_task *next_task;
} interrupt_task;

typedef struct trap_frame
{
    uint64_t regs[31];
    uint64_t spsr_el1;
    uint64_t elr_el1;
    uint64_t sp_el0;
} trap_frame_t;

void no_exception_handle();
void lowerEL_sync_interrupt_handle(trap_frame_t *trap_frame);
void lowerEL_irq_router();
void currentEL_irq_router();
void enable_interrupt(bool enable);

#endif