#include "mini_uart.h"
#include "shell.h"
#include "dtb.h"

void main()
{
    register unsigned long x0 asm("x0");
    unsigned long DTB_BASE = x0;
    // set up serial console
    uart_init();

    uart_puts("DTB_BASE: ");
    uart_hex(DTB_BASE);
    uart_puts("\n\r");
    // uart_puts("start finding cpio");
    fdt_parser((fdt_header *)(DTB_BASE), initramfs_callback);
    // init timeout queue
    init_timeout();
    // init buddy system
    mm_init();
    // execute shell
    exe_shell();
    
    return;
}