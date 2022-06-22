#ifndef __THREAD__H__
#define __THREAD__H__

#include "utils.h"
#include "mm.h"
#include "exception.h"
#include "mmu.h"
#include "mmu_def.h"

#define THREAD_POOL_SIZE 64

#define THREAD_RUNNING 0
#define THREAD_IDLE 1
#define THREAD_WAIT 2
#define THREAD_EXIT 3

#define THREAD_STACK_NPAGE 1
#define THREAD_STACK_SIZE (THREAD_STACK_NPAGE * PAGE_SIZE)

// POSIX signal
#define THREAD_MAX_SIG_NUM 16

typedef int pid_t;
typedef struct trap_frame trap_frame_t;

typedef struct cpu_context
{
    /* x0 - x18 can be overwritten
     * save callee saved registers
     */
    uint64_t x19;
    uint64_t x20;
    uint64_t x21;
    uint64_t x22;
    uint64_t x23;
    uint64_t x24;
    uint64_t x25;
    uint64_t x26;
    uint64_t x27;
    uint64_t x28;
    uint64_t fp;
    uint64_t lr;
    uint64_t sp;
} cpu_context_t;

typedef struct thread
{
    cpu_context_t regs;
    pid_t pid;
    uint64_t code_addr;
    uint64_t kernel_stack;
    uint64_t user_stack;
    uint32_t status;

    // For POSIX Signal
    cpu_context_t signal_save_context;

    void (*signal_handlers[THREAD_MAX_SIG_NUM])();
    uint32_t signal_num[THREAD_MAX_SIG_NUM];

    struct thread *next;
} thread_t;

typedef struct thread_queue
{
    thread_t *front;
    thread_t *rear;
} thread_queue_t;

// thread queue operations
void thread_queue_init(thread_queue_t *queue);
void thread_queue_push(thread_queue_t *queue, thread_t *node);
thread_t *thread_queue_pop(thread_queue_t *queue);

void _thread_timer_task();
void thread_init();
pid_t get_unused_pid();
thread_t *get_cur_thread();
thread_t *thread_create(void (*func)());
void *thread_sched();
void thread_exit();
void idle_thread();
pid_t get_thread_pid();
pid_t copy_process(trap_frame_t *trap_frame);
void thread_kill(pid_t pid);
void thread_exec(void (*func)());

// POSIX signal
void check_signal();
void *get_signal_handler(thread_t* thread);
void _default_signal_handler();
void thread_signal_register(int signal, void (*handler)());
void thread_signal_kill(int pid, int signal);
void thread_signal_return();

// test functionality
void foo();
void thread_create_test();
void print_used_pid();
void print_pid_in_queue();
void fork_test();

#endif