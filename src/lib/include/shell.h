#ifndef __SHELL__H__
#define __SHELL__H__
#include "mini_uart.h"
#include "string.h"
#include "reboot.h"
#include "mbox.h"
#define MAX_BUFFER_SIZE 256u

void print_sys_info();
void welcome_msg();
void help();
void cmd_handler(char *cmd);
void cmd_reader(char *cmd);
void exe_shell();
void clear_screen();

#endif