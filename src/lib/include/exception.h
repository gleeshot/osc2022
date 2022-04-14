#ifndef __EXCEPTION__H__
#define __EXCEPTION__H__

#include "mini_uart.h"
#include "utils.h"
#include "timer.h"

#define CORE0_INTR_SRC                  ((unsigned int*)(0x40000060))

typedef struct interrupt_task {
    void (*handler) ();
    struct interrupt_task *next_task;
} interrupt_task;

void no_exception_handle();
void lowerEL_sync_interrupt_handle();
void lowerEL_irq_router();
void currentEL_irq_router();
void enable_interrupt(bool enable);

#endif