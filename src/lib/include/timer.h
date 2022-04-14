#ifndef __TIMER__H__
#define __TIMER__H__

#define CORE0_TIMER_IRQ_CTRL ((unsigned int *)(0x40000040))
#define max_queue_size 20
#define max_msg_len 20

#include "exception.h"
#include "mini_uart.h"
#include "utils.h"

typedef struct timeout
{
    uint64_t start;
    uint64_t duration;
    char msg[max_msg_len];
    void (*callback)(char *);
    struct timeout *next;
} timeout;

void core_timer_enable();
void core_timer_disable();
uint64_t cur_time();
void core_timer_interrupt();
void set_next_timeout(unsigned int timeout);

// advanced exercise
void init_timeout();
void add_timer(void (*callback)(), char *msg, int duration);
void timeout_handler();
void print_timout_msg(char *msg);

#endif