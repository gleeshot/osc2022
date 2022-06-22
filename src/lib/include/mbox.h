#ifndef __MBOX__H__
#define __MBOX__H__

#include "gpio.h"
#include "mini_uart.h"
#include "mmio.h"
#include "mmu.h"

volatile unsigned int  __attribute__((aligned(16))) mbox[36];

#define VIDEOCORE_MBOX  (MMIO_BASE+0x0000B880)
#define MBOX_READ       ((volatile unsigned int*)(VIDEOCORE_MBOX+0x0))
#define MBOX_POLL       ((volatile unsigned int*)(VIDEOCORE_MBOX+0x10))
#define MBOX_SENDER     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x14))
#define MBOX_STATUS     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x18))
#define MBOX_CONFIG     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x1C))
#define MBOX_WRITE      ((volatile unsigned int*)(VIDEOCORE_MBOX+0x20))
#define MBOX_RESPONSE   0x80000000
#define MBOX_FULL       0x80000000
#define MBOX_EMPTY      0x40000000

/* channels */
#define MBOX_CH_POWER   0
#define MBOX_CH_FB      1
#define MBOX_CH_VUART   2
#define MBOX_CH_VCHIQ   3
#define MBOX_CH_LEDS    4
#define MBOX_CH_BTNS    5
#define MBOX_CH_TOUCH   6
#define MBOX_CH_COUNT   7
#define MBOX_CH_PROP    8

#define GET_BOARD_REVISION  0x00010002
#define GET_ARM_MEMORY     0x00010005
#define REQUEST_CODE        0x00000000
#define REQUEST_SUCCEED     0x80000000
#define REQUEST_FAILED      0x80000001
#define TAG_REQUEST_CODE    0x00000000
#define END_TAG             0x00000000

#define MAIL_BODY_BUF_LEN    (4)
#define MAIL_BUF_SIZE        (MAIL_BODY_BUF_LEN << 2)
#define MAIL_PACKET_SIZE     (MAIL_BUF_SIZE + 24)

struct mail_header {
    unsigned int packet_size;
    unsigned int code;
};

struct mail_body {
    unsigned int identifier;
    unsigned int buffer_size;
    unsigned int code;
    unsigned int buffer[MAIL_BODY_BUF_LEN];
    unsigned int end;
};

typedef struct mail_t {
    struct mail_header header;
    struct mail_body   body;
} mail_t __attribute__((aligned(16)));

int mbox_call(unsigned char ch);
void get_board_revision(unsigned int* revision);
void get_arm_memory(unsigned int* mem_base, unsigned int* mem_size);
int _mbox_call(mail_t *mbox, unsigned char ch);
#endif