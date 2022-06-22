// #include "timer.h"

// void core_timer_enable()
// {
//     asm volatile("mov x1, 1");
//     asm volatile("msr cntp_ctl_el0, x1"); // enable (check the last bit)
//     asm volatile("mrs x1, cntfrq_el0");
//     asm volatile("msr cntp_tval_el0, x1"); // set expired time
//     asm volatile("mov x1, 2");
//     asm volatile("ldr x4, =0x40000040");
//     asm volatile("str w1, [x4]"); // unmask timer interrupt
// }

// void core_timer_disable()
// {
//     // disable timer
//     register unsigned int enable = 0;
//     asm volatile("msr cntp_ctl_el0, %0"
//                  :
//                  : "r"(enable));
//     // disable timer interrupt
//     *CORE0_TIMER_IRQ_CTRL &= !(1 << 1);
// }

// uint64_t cur_time()
// {
//     uint64_t cntpct_el0, cntfrq_el0, time;
//     asm volatile("mrs %0, cntpct_el0"
//                  : "=r"(cntpct_el0));
//     asm volatile("mrs %0, cntfrq_el0"
//                  : "=r"(cntfrq_el0));
//     time = cntpct_el0 / cntfrq_el0;
//     return time;
// }

// void core_timer_interrupt()
// {
//     uint64_t time = cur_time();
//     uart_puts("time after booting: ");
//     uart_hex(time);
//     uart_puts("\n");
//     // set next timeout
//     set_next_timeout(2);
// }

// void set_next_timeout(unsigned int timeout)
// {
//     uint64_t cntfrq_el0;
//     asm volatile("mrs %0, cntfrq_el0"
//                  : "=r"(cntfrq_el0));
//     volatile uint64_t next_timeout = timeout * cntfrq_el0;
//     asm volatile("msr cntp_tval_el0, %0"
//                  :
//                  : "r"(next_timeout));
// }

// timeout timeout_buffer[max_queue_size];
// timeout *timeout_queue;
// int buf_idx;

// void init_timeout()
// {
//     // initialize timeout buffer
//     for (int i = 0; i < max_queue_size; i++)
//     {
//         timeout_buffer[i].duration = 0;
//         timeout_buffer[i].start = 0;
//         timeout_buffer[i].next = 0;
//     }

//     // set pointer and index to 0
//     timeout_queue = 0;
//     buf_idx = 0;
// }

// // add timer function
// void add_timer(void (*callback)(), char *msg, int duration)
// {
//     uint64_t current_time = cur_time();
//     timeout_buffer[buf_idx].callback = callback;
//     timeout_buffer[buf_idx].start = current_time;
//     timeout_buffer[buf_idx].duration = duration;

//     // input the message
//     // for (int i = 0; i < max_msg_len; i++)
//     // {
//     //     timeout_buffer[buf_idx].msg[i] = msg[i];
//     // }

//     timeout *prev = 0, *cur = 0;
//     for (cur = timeout_queue; cur != 0; prev = cur, cur = cur->next)
//     {
//         // find the position the timeout should be inserted
//         if (cur->start + cur->duration > current_time + duration)
//             break;
//     }

//     if (prev == 0 && cur == 0)
//     {
//         // empty
//         // core_timer_enable();
//         timeout_queue = &timeout_buffer[buf_idx];
//         // set_next_timeout(timeout_queue->duration);
//         set_timeout_by_ticks(duration);
//         set_timer_interrupt(true);
//     }
//     else if (prev != 0 && cur == 0)
//     { // cur->endtime > all tasks' endtime
//         prev->next = &timeout_buffer[buf_idx];
//     }
//     else if (prev == 0 && cur != 0)
//     { // cur->endtime < all tasks' endtime
//         timeout_buffer[buf_idx].next = cur;
//         timeout_queue = &timeout_buffer[buf_idx];
//         // set_next_timeout(timeout_buffer[buf_idx].duration);
//         set_timeout_by_ticks(duration);
//     }
//     else
//     {
//         // insert into timeout queue
//         timeout_buffer[buf_idx].next = cur;
//         prev->next = &timeout_buffer[buf_idx];
//     }

//     if (++buf_idx == max_queue_size)
//         buf_idx = 0;
//     return;
// }

// void timeout_handler()
// {
//     if (timeout_queue == 0)
//     {
//         // core_timer_disable();
//         set_timer_interrupt(false);
//         return;
//     }
//     uart_puts("\r\n");
//     timeout_queue->callback(timeout_queue->msg);
//     timeout_queue->start = 0;
//     timeout_queue = timeout_queue->next;

//     // update timeout
//     if (timeout_queue != 0)
//     {
//         uint64_t current_time = cur_time();
//         uint64_t expired_time = timeout_queue->start + timeout_queue->duration - current_time;
//         if (expired_time <= 0)
//             expired_time = 0; // if the expired time is passed, do it immediately
//         // set_next_timeout(expired_time);
//         set_timeout_by_ticks(expired_time);
//         // core_timer_enable();
//         set_timer_interrupt(true);
//     }
//     else
//     {
//         // core_timer_disable();
//         set_timer_interrupt(false);
//     }
    
//     thread_sched();
//     return;
// }

// void print_timout_msg(char *msg)
// {
//     uart_puts("[0x");
//     uart_hex(timeout_queue->duration);
//     uart_puts(" seconds] after [0x");
//     uart_hex(timeout_queue->start);
//     uart_puts("]\r\n");

//     uart_puts(msg);
//     uart_puts("\r\n");
//     uart_puts(">>");
//     return;
// }

// void set_timer_interrupt (bool enable) {

//     /* Needs to set timeout first */

//     if (enable)
//     {
//         // Enable
//         asm volatile ("mov x0, 1");
//         asm volatile ("msr cntp_ctl_el0, x0");
//         // Unmask timer interrupt
//         asm volatile ("mov x0, 2");
//         asm volatile ("ldr x1, =0x40000040");
//         asm volatile ("str w0, [x1]");
//     }
//     else
//     {
//         // mask timer interrupt
//         asm volatile ("mov x0, 0");
//         asm volatile ("ldr x1, =0x40000040");
//         asm volatile ("str w0, [x1]");
//     }

//     return;
// }

// void set_timeout_by_ticks (unsigned int ticks) {

//     unsigned long cntpct_el0;
//     unsigned long cntp_cval_el0;

//     asm volatile("mrs %0,  cntpct_el0" : "=r"(cntpct_el0) : );

//     cntp_cval_el0 = cntpct_el0 + ticks;

//     // Set next expire time
//     asm volatile ("msr cntp_cval_el0, %0" :: "r"(cntp_cval_el0));

//     return;
// }
#include "timer.h"

struct timer_task *timer_task_list = NULL;

void add_timer (void (*callback) (), void *data, unsigned int after) {

    unsigned long c_time;
    struct timer_task *new_task;
    
    c_time = time();
    
    /* Create nww timer task */
    new_task = kmalloc(sizeof(struct timer_task));
    new_task->execute_time = c_time + after;
    new_task->callback     = callback;
    new_task->data         = data;
    new_task->next_task    = NULL;

    /* First task */
    if (timer_task_list == NULL)
    {

        timer_task_list = new_task;
        
        /* Set timeout first */
        set_timeout_by_ticks(after);

        /* Enable core timer interrupt */
        set_timer_interrupt(true);

    }
    else /* Insert into list */
    {

        struct timer_task *prev;
        struct timer_task *c;
        prev = NULL;
        c    = timer_task_list;

        while (c != NULL)
        {

            if (c->execute_time > new_task->execute_time)
            {
                /* Position found */
                break;
            }

            prev = c;
            c = c->next_task;
        }

        /* Insert */
        if (prev != NULL)
        {
            new_task->next_task = prev->next_task;
            prev->next_task = new_task;
        }
        else
        {
            /* Insert at head */
            new_task->next_task = timer_task_list;
            timer_task_list = new_task;

            /* Update timeout */
            set_timeout_by_ticks(after);
        }

    }

    /* Print prompt */
    // uart_puts("[ ");
    // uart_putu(c_time);
    // uart_puts(" secs ] Timer task will be execute ");
    // uart_putu(after);
    // uart_puts(" secs later. (at ");
    // uart_putu(new_task->execute_time);
    // uart_puts(" secs)\n");

    return;
}

void set_timeout (unsigned int seconds) {

    // Set next expire time
    asm volatile ("mrs x2, cntfrq_el0");
    asm volatile ("mul x1, x2, %0" :: "r"(seconds));
    asm volatile ("msr cntp_tval_el0, x1");

    return;
}

void set_timeout_by_ticks (unsigned int ticks) {

    unsigned long cntpct_el0;
    unsigned long cntp_cval_el0;

    asm volatile("mrs %0,  cntpct_el0" : "=r"(cntpct_el0) : );

    cntp_cval_el0 = cntpct_el0 + ticks;

    // Set next expire time
    asm volatile ("msr cntp_cval_el0, %0" :: "r"(cntp_cval_el0));

    return;
}

unsigned long time () {

    unsigned long cntpct_el0;
    unsigned long cntfrq_el0;
    unsigned long time_in_sec;

    asm volatile("mrs %0,  cntpct_el0" : "=r"(cntpct_el0) : );
    asm volatile("mrs %0,  cntfrq_el0" : "=r"(cntfrq_el0) : );

    time_in_sec = cntpct_el0 / cntfrq_el0;

    return time_in_sec;
}

void print_time () {

    unsigned long c_time = time();

    uart_puts("[Current time] ");
    uart_hex(c_time);
    uart_puts("secs\n");

    return;
}

void set_timer_interrupt (bool enable) {

    /* Needs to set timeout first */
    
    if (enable)
    {
        // Enable
        asm volatile ("mov x0, 1");
        asm volatile ("msr cntp_ctl_el0, x0");
        // Unmask timer interrupt
        asm volatile ("mov x0, 2");
        asm volatile ("ldr x1, =0x40000040");
        asm volatile ("str w0, [x1]");
    }
    else
    {
        // mask timer interrupt
        asm volatile ("mov x0, 0");
        asm volatile ("ldr x1, =0x40000040");
        asm volatile ("str w0, [x1]");
    }
    
    return;
}

void timeout_handler () {

    unsigned long c_time, after;
    struct timer_task *curr_task;

    c_time = time();

    /* Print prompt */
    // uart_puts("\n\n");
    // uart_puts("[ ");
    // uart_putu(c_time);
    // uart_puts(" secs ] Timer irq occur, execute pre-scheduled task.");
    // uart_puts("\n\n");

    /* Do callback */
    curr_task = timer_task_list;
    timer_task_list->callback(timer_task_list->data);
    timer_task_list = timer_task_list->next_task;

    /* Update timeout or disable timer interrupt */
    if (timer_task_list == NULL)
    {
        /* No task to do, disable timer interrupt */
        set_timer_interrupt(false);
    }
    else
    {
        if (timer_task_list->execute_time > c_time)
        {
            after = timer_task_list->execute_time - c_time;
        }
        else
        {
            after = 0;
        }

        /* Set next timeout */
        set_timeout_by_ticks(after);

        /* Enable the timer interrupt */
        set_timer_interrupt(true);

    }

    kfree(curr_task);
    set_timer_interrupt(false);
    check_signal();
    set_timer_interrupt(true);
    thread_sched();

    return;
}