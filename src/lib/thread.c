#include "thread.h"

extern void switch_to(thread_t *cur_thread, thread_t *nxt_thread);
extern void store_context(cpu_context_t *context);
extern void load_context(cpu_context_t *context);
extern int sys_get_pid();
extern int sys_fork();
extern int sys_exit();
extern void sys_ret();
extern void from_el1_to_el0(unsigned long func, unsigned long user_sp, unsigned long kernel_sp);

thread_queue_t *idle_queue = NULL;
thread_queue_t *wait_queue = NULL;
bool used_pid[THREAD_POOL_SIZE];

void thread_queue_init(thread_queue_t *queue)
{
    queue->front = NULL;
    queue->rear = NULL;
}

// push the node to the tail of the queue
void thread_queue_push(thread_queue_t *queue, thread_t *node)
{
    // check if the queue is null
    if (queue->rear == NULL)
    {
        queue->front = node;
    }
    else
    {
        queue->rear->next = node;
    }

    queue->rear = node;
    node->next = NULL;
    return;
}

// pop out the first node in the queue
thread_t *thread_queue_pop(thread_queue_t *queue)
{
    thread_t *node = NULL;
    if (queue->front != NULL)
    {
        node = queue->front;
        if (queue->front->next == NULL)
        {
            queue->rear = NULL;
        }
        queue->front = queue->front->next;
        node->next = NULL;
    }
    return node;
}

void _thread_timer_task()
{

    unsigned long cntfrq_el0;

    asm volatile("mrs %0,  cntfrq_el0"
                 : "=r"(cntfrq_el0)
                 :);

    add_timer(_thread_timer_task, NULL, cntfrq_el0 >> 5);

    return;
}

void thread_init()
{
    idle_queue = kmalloc(sizeof(thread_queue_t));
    wait_queue = kmalloc(sizeof(thread_queue_t));

    thread_queue_init(idle_queue);
    thread_queue_init(wait_queue);

    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        used_pid[i] = false;
    }

    thread_t *kernel_thread;
    kernel_thread = kmalloc(sizeof(thread_t));

    kernel_thread->status = THREAD_WAIT;
    kernel_thread->pid = 0;

    for (int i = 0; i < THREAD_MAX_SIG_NUM; i++)
    {
        kernel_thread->signal_handlers[i] = NULL;
        kernel_thread->signal_num[i] = 0;
    }

    thread_queue_push(wait_queue, kernel_thread);
    set_cur_thread(kernel_thread);
    used_pid[0] = true;

    return;
}

pid_t get_unused_pid()
{
    pid_t pid = -1;
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        if (used_pid[i] == false)
        {
            pid = (pid_t)i;
            break;
        }
    }
    return pid;
}

void set_cur_thread(thread_t *cur)
{
    asm volatile("msr tpidr_el1, %0"
                 :
                 : "r"(cur));
    return;
}

thread_t *get_cur_thread()
{
    thread_t *cur;
    asm volatile("mrs %0, tpidr_el1"
                 : "=r"(cur));
    return cur;
}

thread_t *thread_create(void (*func)())
{
    thread_t *t = NULL;
    pid_t pid;

    if (idle_queue == NULL)
    {
        thread_init();
    }
    pid = get_unused_pid();
    if (pid != -1)
    {
        t = kmalloc(sizeof(thread_t));

        t->pid = pid;
        t->status = THREAD_IDLE;
        t->kernel_stack = (uint64_t)kmalloc(THREAD_STACK_SIZE);
        t->user_stack = (uint64_t)kmalloc(THREAD_STACK_SIZE);
        t->code_addr = (uint64_t)func;

        // set context
        t->regs.lr = (uint64_t)func;
        t->regs.fp = (uint64_t)t->user_stack + THREAD_STACK_SIZE;
        t->regs.sp = (uint64_t)t->user_stack + THREAD_STACK_SIZE;

        t->next = NULL;

        for (int i = 0; i < THREAD_MAX_SIG_NUM; i++)
        {
            t->signal_handlers[i] = NULL;
            t->signal_num[i] = 0;
        }

        thread_queue_push(idle_queue, t);
        used_pid[pid] = true;
        uart_puts("[thread_create] pid: 0x");
        uart_hex(pid);
        uart_puts("\r\n");
    }
    return t;
}

void *thread_sched()
{
    thread_t *cur_thread = NULL;
    thread_t *nxt_thread = NULL;

    cur_thread = get_cur_thread();

    while (1)
    {
        nxt_thread = thread_queue_pop(idle_queue);

        if (nxt_thread == NULL)
        {
            // uart_puts("no thread anymore!");
            break;
        }
        else if (nxt_thread->status == THREAD_IDLE)
        {
            // uart_puts("\r\n");
            // uart_puts("[thread sched] current thread's id: 0x");
            // uart_hex(cur_thread->pid);
            // uart_puts(", next thread's id: ");
            // uart_hex(nxt_thread->pid);
            // uart_puts("\r\n");
            // if (nxt_thread->pid != 0)
            {
                thread_queue_push(idle_queue, cur_thread);
            }
            switch_to(cur_thread, nxt_thread);
            // print_pid_in_queue(idle_queue);
            break;
        }
        else if (nxt_thread->status == THREAD_EXIT)
        {
            // kill zombie
            // uart_puts("\r\n");
            // uart_puts("[kill zombie] killed: 0x");
            // uart_hex(nxt_thread->pid);
            // uart_puts("\r\n");
            used_pid[nxt_thread->pid] = false;
            buddy_free((void *)nxt_thread->kernel_stack);
            buddy_free((void *)nxt_thread->user_stack);
            kfree((void *)nxt_thread);
        }
    }

    return;
}

void thread_exit()
{
    thread_t *cur;
    cur = get_cur_thread();
    cur->status = THREAD_EXIT;
    thread_sched();
    return;
}

void idle_thread()
{
    while (1)
    {
        // uart_puts("idle_thread\n");
        thread_sched();
    }
}

pid_t get_thread_pid()
{
    // get current thread's pid
    thread_t *cur;
    cur = get_cur_thread();

    pid_t pid;
    pid = cur->pid;

    return pid;
}

pid_t copy_process(trap_frame_t *trap_frame)
{
    thread_t *parent;
    thread_t *child;
    trap_frame_t *child_trap_frame;
    uint64_t ks_offset, us_offset;
    thread_t *cur;

    enable_interrupt(false);

    parent = get_cur_thread();
    child = thread_create((void *)parent->code_addr); // create child thread with parent's function
    ks_offset = child->kernel_stack - parent->kernel_stack;
    us_offset = child->user_stack - parent->user_stack;
    child_trap_frame = (trap_frame_t *)((uint64_t)trap_frame + ks_offset);
    
    store_context(&child->regs);
    cur = get_cur_thread();
    if (cur->pid == parent->pid)
    {
        /* lab5 */
        trap_frame->regs[0] = child->pid;
        for (int i = 0; i < THREAD_STACK_SIZE; i++)
        {
            ((char *)child->kernel_stack)[i] = ((char *)parent->kernel_stack)[i];
            ((char *)child->user_stack)[i] = ((char *)parent->user_stack)[i];
        }
        for (int i = 0; i < THREAD_MAX_SIG_NUM; i++)
        {
            child->signal_handlers[i] = parent->signal_handlers[i];
            child->signal_num[i] = parent->signal_num[i];
        }
        child->regs.sp += ks_offset;
        child->regs.fp += ks_offset;
        child_trap_frame->sp_el0 += us_offset;
        child_trap_frame->regs[0] = 0;
        return child->pid;

        /* lab 6 */
        // child->code_size = parent->code_size;

        // for (int i = 0; i < child->code_size; i++)
        // {
        //     ((char *)prog_base)[i] = ((char *)(parent->code_addr))[i];
        // }

        // for (int i = 0; i < child->code_size; i += PAGE_TABLE_SIZE)
        // {
        //     unsigned long va = USER_PROG_VA + i;
        //     unsigned long pa = va_to_pa(child->code_addr+ i);

        //     alloc_page_table(child->pgd, va, pa, PD_USER_ATTR);
        // }

        // for (int i = 0; i < THREAD_STACK_SIZE; i++)
        // {
        //     ((char *)child->kernel_stack)[i] = ((char *)parent->kernel_stack)[i];
        //     ((char *)child->user_stack)[i] = ((char *)parent->user_stack)[i];
        // }

        // for (int i = 0; i < THREAD_MAX_SIG_NUM; i++)
        // {
        //     child->signal_handlers[i] = parent->signal_handlers[i];
        //     child->signal_num[i] = parent->signal_num[i];
        // }
        // enable_interrupt(true);
        // return child->pid;
    }

    enable_interrupt(true);
    return 0;
}

void thread_kill(pid_t pid)
{
    thread_t *cur_thread;
    bool found = false;

    enable_interrupt(false);

    // check threads in the idle queue
    cur_thread = idle_queue->front;
    while (cur_thread != NULL)
    {
        if (cur_thread->pid == pid)
        {
            // uart_puts("[thread kill] killed: 0x");
            // uart_hex(pid);
            // uart_puts("\r\n");
            cur_thread->status = THREAD_EXIT;
            found = true;
            break;
        }
        cur_thread = cur_thread->next;
    }

    // check threads in the wait queue
    cur_thread = wait_queue->front;
    if (!found)
    {
        while (cur_thread != NULL)
        {
            if (cur_thread->pid == pid)
            {
                // uart_puts("[thread kill] killed: 0x");
                // uart_hex(pid);
                // uart_puts("\r\n");
                cur_thread->status = THREAD_EXIT;
                found = true;
                break;
            }
            cur_thread = cur_thread->next;
        }
    }

    if (!found)
    {
        // uart_puts("[thread kill] pid: ");
        // uart_hex(pid);
        // uart_puts("not found\r\n");
    }

    enable_interrupt(true);

    return;
}

void check_signal()
{
    void (*handler)();
    void *signal_ustack;
    thread_t *cur = get_cur_thread();

    uint64_t sp, spsr_el1;

    while (1)
    {
        store_context(&cur->signal_save_context);
        handler = get_signal_handler(cur);
        if (handler == NULL)
        {
            break;
        }
        else
        {
            if (handler == _default_signal_handler)
            {
                handler();
            }
            else
            {
                uart_puts("there's a user handler!\r\n");
                set_timer_interrupt(true);
                signal_ustack = kmalloc(THREAD_STACK_SIZE);
                // Run in user mode
                asm volatile("msr     elr_el1, %0 \n\t"
                             "msr     sp_el0,  %1 \n\t"
                             "mov     lr, %2      \n\t"
                             "eret" :: "r" (handler), "r"(signal_ustack + THREAD_STACK_SIZE), "r" (sys_ret));
                kfree(signal_ustack);
                set_timer_interrupt(false);
            }
        }
    }
    return;
}

void *get_signal_handler(thread_t* thread)
{
    void *handler = NULL;
    for (int i = 0; i < THREAD_MAX_SIG_NUM; i++)
    {
        if (thread->signal_num[i] > 0)
        {
            if (thread->signal_handlers[i] == NULL)
            {
                handler = _default_signal_handler;
            }
            else
            {
                handler = thread->signal_handlers[i];
            }
            thread->signal_num[i] -= 1;
            break;
        }
    }
    return handler;
}

void _default_signal_handler()
{
    thread_t *cur = get_cur_thread();
    cur->status = THREAD_EXIT;

    return;
}

void thread_signal_register(int signal, void (*handler)())
{
    thread_t *cur = get_cur_thread();

    if (signal > THREAD_MAX_SIG_NUM)
    {
        uart_puts("[thread signal register] error signal!\r\n");
    }

    cur->signal_handlers[signal] = handler;
    uart_puts("[thread signal register] signal: 0x");
    uart_hex(signal);
    uart_puts(" is registered\r\n");

    return;
}

void thread_signal_kill(int pid, int signal)
{
    thread_t *thread;
    thread = idle_queue->front;

    while (thread != NULL)
    {
        if (thread->pid == pid)
        {
            uart_puts("[thread signal kill] signal: ");
            uart_hex(signal);
            uart_puts(", pid: ");
            uart_hex(pid);
            uart_puts("\r\n");

            thread->signal_num[signal]++;
            break;
        }
        thread = thread->next;
    }
    return;
}

void thread_signal_return()
{
    thread_t *cur = get_cur_thread();
    load_context(&cur->signal_save_context);
}

void foo()
{
    for (int i = 0; i < 5; ++i)
    {
        uart_puts("[thread test] Thread id: ");
        uart_hex(get_cur_thread()->pid);
        uart_puts(", iteration: ");
        uart_hex(i);
        uart_puts("\r\n");
        // print_pid_in_queue(idle_queue);
        thread_sched();
    }
    thread_exit();
}

void thread_create_test()
{
    thread_t *sched_thread = thread_create(idle_thread);
    // set_cur_thread(sched_thread);
    for (int i = 0; i < 5; i++)
    {
        thread_create(foo);
    }
    uart_puts("create finish\r\n");
    print_pid_in_queue(idle_queue);
    thread_sched();
}

void print_used_pid()
{
    uart_puts("used pid: ");
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        if (used_pid[i] == true)
        {
            uart_hex(i);
            uart_send(' ');
        }
    }
    uart_puts("\r\n");
}

void print_pid_in_queue(thread_queue_t *queue)
{
    thread_t *cur = queue->front;
    while (cur != NULL)
    {
        uart_hex(cur->pid);
        cur = cur->next;
    }
    uart_puts("\r\n");
    return;
}

void fork_test()
{
    uart_puts("\n[fork test] pid: 0x");
    uart_hex(sys_get_pid());
    uart_puts("\r\n");

    int cnt = 1;
    int ret = 0;

    ret = sys_fork();

    if (ret == 0)
    {
        // child thread
        uint64_t cur_sp;
        asm volatile("mov %0, sp"
                     : "=r"(cur_sp));

        uart_puts("first child pid: 0x");
        uart_hex(sys_get_pid());
        uart_puts(" ,cnt: 0x");
        uart_hex(cnt);
        uart_puts(" ,ptr: 0x");
        uart_hex(&cnt);
        uart_puts(" ,sp: 0x");
        uart_hex(cur_sp);
        uart_puts("\r\n");

        ++cnt;

        if ((ret = sys_fork()) != 0)
        {
            asm volatile("mov %0, sp"
                         : "=r"(cur_sp));
            uart_puts("first child pid: 0x");
            uart_hex(sys_get_pid());
            uart_puts(" ,cnt: 0x");
            uart_hex(cnt);
            uart_puts(" ,ptr: 0x");
            uart_hex(&cnt);
            uart_puts(" ,sp: 0x");
            uart_hex(cur_sp);
            uart_puts("\r\n");
        }
        else
        {
            while (cnt < 5)
            {
                asm volatile("mov %0, sp"
                             : "=r"(cur_sp));
                uart_puts("second child pid: 0x");
                uart_hex(sys_get_pid());
                uart_puts(" ,cnt: 0x");
                uart_hex(cnt);
                uart_puts(" ,ptr: 0x");
                uart_hex(&cnt);
                uart_puts(" ,sp: 0x");
                uart_hex(cur_sp);
                uart_puts("\r\n");

                ++cnt;
            }
        }
        sys_exit();
    }
    else
    {
        uart_puts("parent pid: 0x");
        uart_hex(sys_get_pid());
        uart_puts(" child pid: 0x");
        uart_hex(ret);
        uart_puts("\r\n");
    }
}

void thread_exec(void (*func)())
{
    thread_t *t = NULL;
    pid_t pid;

    if (idle_queue == NULL)
    {
        thread_init();
    }
    pid = get_unused_pid();
    if (pid != -1)
    {
        t = kmalloc(sizeof(thread_t));

        t->pid = pid;
        t->status = THREAD_IDLE;
        t->code_addr = (uint64_t)func;
        t->kernel_stack = (uint64_t)kmalloc(THREAD_STACK_SIZE);
        t->user_stack = (uint64_t)kmalloc(THREAD_STACK_SIZE);
        
        // set context
        t->regs.lr = (uint64_t)func;
        t->regs.fp = (uint64_t)t->user_stack + THREAD_STACK_SIZE;
        t->regs.sp = (uint64_t)t->user_stack + THREAD_STACK_SIZE;

        for (int i = 0; i < THREAD_MAX_SIG_NUM; i++)
        {
            t->signal_handlers[i] = NULL;
            t->signal_num[i] = 0;
        }
        
        t->next = NULL;
        used_pid[pid] = true;

        unsigned long tmp;
        asm volatile("mrs %0, cntkctl_el1"
                     : "=r"(tmp));
        tmp |= 1;
        asm volatile("msr cntkctl_el1, %0"
                     :
                     : "r"(tmp));
        uart_puts("hi");
        add_timer(_thread_timer_task, NULL, 5);
        uart_puts("hi");
        // Get current EL
        uint64_t current_el;
        asm volatile("mrs %0, CurrentEL"
                     : "=r"(current_el));
        current_el = current_el >> 2;

        // Print prompt
        uart_puts("Current EL: 0x");
        uart_hex(current_el);
        uart_puts("\n");
        uart_puts("User program at: 0x");
        uart_hex((unsigned long)func);
        uart_puts("\n");
        uart_puts("User program stack top: 0x");
        uart_hex((unsigned long)t->regs.sp);
        uart_puts("\n");
        uart_puts("-----------------Entering user program-----------------\n");

        disable_mini_uart_interrupt();
        disable_read_interrupt();
        disable_write_interrupt();

        set_cur_thread(t);
        from_el1_to_el0((unsigned long)func, (unsigned long)t->user_stack + THREAD_STACK_SIZE, (unsigned long)t->kernel_stack + THREAD_STACK_SIZE);
        // sys_exit();
    }
    return;
}