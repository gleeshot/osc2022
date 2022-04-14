#ifndef __MINI_UART__H__
#define __MINI_UART__H__
#include "gpio.h"

/* Auxilary mini UART registers */
#define AUX_ENABLE      ((volatile unsigned int*)(MMIO_BASE+0x00215004))
#define AUX_MU_IO       ((volatile unsigned int*)(MMIO_BASE+0x00215040))
#define AUX_MU_IER      ((volatile unsigned int*)(MMIO_BASE+0x00215044))
#define AUX_MU_IIR      ((volatile unsigned int*)(MMIO_BASE+0x00215048))
#define AUX_MU_LCR      ((volatile unsigned int*)(MMIO_BASE+0x0021504C))
#define AUX_MU_MCR      ((volatile unsigned int*)(MMIO_BASE+0x00215050))
#define AUX_MU_LSR      ((volatile unsigned int*)(MMIO_BASE+0x00215054))
#define AUX_MU_MSR      ((volatile unsigned int*)(MMIO_BASE+0x00215058))
#define AUX_MU_SCRATCH  ((volatile unsigned int*)(MMIO_BASE+0x0021505C))
#define AUX_MU_CNTL     ((volatile unsigned int*)(MMIO_BASE+0x00215060))
#define AUX_MU_STAT     ((volatile unsigned int*)(MMIO_BASE+0x00215064))
#define AUX_MU_BAUD     ((volatile unsigned int*)(MMIO_BASE+0x00215068))

#define IRQ_BASE        ((volatile unsigned int*)(MMIO_BASE+0x0000b000))
#define ENABLE_IRQs1    ((volatile unsigned int*)(MMIO_BASE+0x0000b210))
#define DISABLE_IRQs1   ((volatile unsigned int*)(MMIO_BASE+0x0000b21c))
#define AUX_IRQ 		(1 << 29)
#define IRQ_PENDING_1	((volatile unsigned int*)(MMIO_BASE+0x0000b204))
#define MAX_UART_BUFFER	1024

void uart_init();
void uart_send(unsigned int c);
char uart_getc();
void uart_puts(char *s);
void uart_hex(unsigned int d);
void uart_getline(char *input);
void init_uart_interrupt();
void enable_mini_uart_interrupt();
void disable_mini_uart_interrupt();
void enable_write_interrupt();
void disable_write_interrupt();
void enable_read_interrupt();
void disable_read_interrupt();
void mini_uart_interrupt_handler();
char uart_async_getc();
void uart_async_puts(char *s);

#endif