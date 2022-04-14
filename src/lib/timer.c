#include "timer.h"

void core_timer_enable()
{
    asm volatile("mov x1, 1");
    asm volatile("msr cntp_ctl_el0, x1"); // enable (check the last bit)
    asm volatile("mrs x1, cntfrq_el0");
    asm volatile("msr cntp_tval_el0, x1"); // set expired time
    asm volatile("mov x1, 2");
    asm volatile("ldr x4, =0x40000040");
    asm volatile("str w1, [x4]"); // unmask timer interrupt
}

void core_timer_disable()
{
    // disable timer
    register unsigned int enable = 0;
    asm volatile("msr cntp_ctl_el0, %0"
                 :
                 : "r"(enable));
    // disable timer interrupt
    *CORE0_TIMER_IRQ_CTRL &= !(1 << 1);
}

uint64_t cur_time()
{
    uint64_t cntpct_el0, cntfrq_el0, time;
    asm volatile("mrs %0, cntpct_el0"
                 : "=r"(cntpct_el0));
    asm volatile("mrs %0, cntfrq_el0"
                 : "=r"(cntfrq_el0));
    time = cntpct_el0 / cntfrq_el0;
    return time;
}

void core_timer_interrupt()
{
    uint64_t time = cur_time();
    uart_puts("time after booting: ");
    uart_hex(time);
    uart_puts("\n");
    // set next timeout
    set_next_timeout(2);
}

void set_next_timeout(unsigned int timeout)
{
    uint64_t cntfrq_el0;
    asm volatile("mrs %0, cntfrq_el0"
                 : "=r"(cntfrq_el0));
    volatile uint64_t next_timeout = timeout * cntfrq_el0;
    asm volatile("msr cntp_tval_el0, %0"
                 :
                 : "r"(next_timeout));
}

timeout timeout_buffer[max_queue_size];
timeout *timeout_queue;
int buf_idx;

void init_timeout()
{
    // initialize timeout buffer
    for (int i = 0; i < max_queue_size; i++)
    {
        timeout_buffer[i].duration = 0;
        timeout_buffer[i].start = 0;
        timeout_buffer[i].next = 0;
    }

    // set pointer and index to 0
    timeout_queue = 0;
    buf_idx = 0;
}

// add timer function
void add_timer(void (*callback)(), char *msg, int duration)
{
    uint64_t current_time = cur_time();
    timeout_buffer[buf_idx].callback = callback;
    timeout_buffer[buf_idx].start = current_time;
    timeout_buffer[buf_idx].duration = duration;

    // input the message
    for (int i = 0; i < max_msg_len; i++)
    {
        timeout_buffer[buf_idx].msg[i] = msg[i];
    }

    timeout *prev = 0, *cur = 0;
    for (cur = timeout_queue; cur != 0; prev = cur, cur = cur->next)
    {
        // find the position the timeout should be inserted
        if (cur->start + cur->duration > current_time + duration)
            break;
    }

    if (prev == 0 && cur == 0)
    {
        // empty
        core_timer_enable();
        timeout_queue = &timeout_buffer[buf_idx];
        set_next_timeout(timeout_queue->duration);
    }
    else if (prev != 0 && cur == 0)
    { // cur->endtime > all tasks' endtime
        prev->next = &timeout_buffer[buf_idx];
    }
    else if (prev == 0 && cur != 0)
    { // cur->endtime < all tasks' endtime
        timeout_buffer[buf_idx].next = cur;
        timeout_queue = &timeout_buffer[buf_idx];
        set_next_timeout(timeout_buffer[buf_idx].duration);
    }
    else
    {
        // insert into timeout queue
        timeout_buffer[buf_idx].next = cur;
        prev->next = &timeout_buffer[buf_idx];
    }

    if (++buf_idx == max_queue_size)
        buf_idx = 0;
    return;
}

void timeout_handler()
{
    if (timeout_queue == 0)
    {
        core_timer_disable();
        return;
    }
    uart_puts("\r\n");
    timeout_queue->callback(timeout_queue->msg);
    timeout_queue->start = 0;
    timeout_queue = timeout_queue->next;

    // update timeout
    if (timeout_queue != 0)
    {
        uint64_t current_time = cur_time();
        uint64_t expired_time = timeout_queue->start + timeout_queue->duration - current_time;
        if (expired_time <= 0)
            expired_time = 0; // if the expired time is passed, do it immediately
        set_next_timeout(expired_time);
    }
    else
        core_timer_disable();
    return;
}

void print_timout_msg(char *msg)
{
    uart_puts("[0x");
    uart_hex(timeout_queue->duration);
    uart_puts(" seconds] after [0x");
    uart_hex(timeout_queue->start);
    uart_puts("]\r\n");

    uart_puts(msg);
    uart_puts("\r\n");
    uart_puts(">>");
    return;
}