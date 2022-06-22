#include "./include/syscall.h"

extern uint32_t CPIO_BASE;

pid_t getpid()
{
    pid_t pid = get_thread_pid();
    return pid;
}

size_t uart_read(char buf[], size_t size)
{
    for (int i = 0; i < size; i++)
    {
        buf[i] = uart_getc();
    }
    return size;
}

size_t uart_write(const char buf[], size_t size)
{
    for (int i = 0; i < size; i++)
    {
        uart_send(buf[i]);
    }
    return size;
}

void exit()
{
    thread_exit();
    return;
}

int _exec(trap_frame_t *trap_frame, const char *name, const char **argv)
{
    thread_t *cur_thread = get_cur_thread();
    cpio_new_header *header = (cpio_new_header *)CPIO_BASE;
    void (*prog)();

    prog = cpio_load(header, name);
    cur_thread->code_addr = (uint64_t)prog;

    trap_frame->elr_el1 = (uint64_t)prog;
    trap_frame->sp_el0 = (uint64_t)(cur_thread->user_stack + THREAD_STACK_SIZE);

    return 0;
}

pid_t fork(trap_frame_t *trap_frame)
{
    pid_t pid = copy_process(trap_frame);
    return pid;
}

void kill(pid_t pid)
{
    thread_kill(pid);
    return;
}