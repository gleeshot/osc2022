#ifndef __SYSCALL__H__
#define __SYSCALL__H__

#include "thread.h"
#include "mini_uart.h"
#include "utils.h"
#include "exception.h"
#include "cpio.h"
#include "mmu.h"
#include "mmu_def.h"

typedef int pid_t;
typedef struct trap_frame trap_frame_t;

int getpid();
size_t uart_read(char buf[], size_t size);
size_t uart_write(const char buf[], size_t size);
void exit();
int _exec(trap_frame_t *trap_frame, const char *name, const char **argv);
pid_t fork(trap_frame_t *trap_frame);
void kill(pid_t pid);

#endif