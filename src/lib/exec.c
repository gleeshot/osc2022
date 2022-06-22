#include "exec.h"
#include "string.h"

void exec(cpio_new_header *header ,char *fpath, int enable_timer) {
    char *file_name;
    int header_info = 0;
    unsigned long file_size;
    char *data;
    cpio_new_header *cur_header = header;
    cpio_new_header *nxt_header;
    volatile char *prog = (char*)0x60000;

    while (1) {
        header_info = cpio_header_parser(cur_header, &file_name, &file_size, &data, &nxt_header);
        // if it is the end of cpio
        if (header_info) {
            uart_puts("no such file");
            break;
        }
        // check if it is the file we want
        if (!strcmp(fpath, file_name)) {
            // todo
            for (int i = 0; i < file_size; i++) {
                prog[i] = data[i];
            }

            if (enable_timer) {
                // core_timer_enable();
                set_timer_interrupt(true);
            }
            volatile unsigned long temp = 0x340;
            asm volatile("msr spsr_el1, %0" : : "r" (temp)); // save el1 PSTATE
            temp = 0x60000;
            asm volatile("msr elr_el1, %0" : : "r" (temp));
            asm volatile("msr sp_el0, %0" : : "r" (temp));
            asm volatile("eret");
            break;
        }
        cur_header = nxt_header;
    }
    return;
}