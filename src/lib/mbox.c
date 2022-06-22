#include "mbox.h"

int mbox_call(unsigned char ch)
{
    unsigned int r = (((unsigned int)((unsigned long)&mbox)&~0xF) | (ch&0xF));
    /* wait until we can write to the mailbox */
    do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_FULL);
    /* write the address of our message to the mailbox with channel identifier */
    *MBOX_WRITE = r;
    /* now wait for the response */
    while(1) {
        /* is there a response? */
        do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_EMPTY);
        /* is it a response to our message? */
        if(r == *MBOX_READ)
            /* is it a valid successful response? */
            return mbox[1]==MBOX_RESPONSE;
    }
    return 0;
}

void get_board_revision(unsigned int* revision) {
    mbox[0] = 7 * 4; // buffer size in bytes
    mbox[1] = REQUEST_CODE;
    //tags begin
    mbox[2] = GET_BOARD_REVISION; // tag identifier
    mbox[3] = 4; // max of request and response value of buffer's length
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0; // value buffer
    //tags end
    mbox[6] = END_TAG;
    mbox_call(MBOX_CH_PROP);
    
    *revision = mbox[5];
    return;
}

void get_arm_memory(unsigned int* mem_base, unsigned int* mem_size) {
    mbox[0] = 8 * 4;
    mbox[1] = REQUEST_CODE;
    // tags begin
    mbox[2] = GET_ARM_MEMORY;
    mbox[3] = 8;
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0; // value buffer
    mbox[6] = 0; // value buffer
    // tags end
    mbox[7] = END_TAG;
    mbox_call(MBOX_CH_PROP);
    *mem_base = mbox[5];
    *mem_size = mbox[6];
    return;
}

int _mbox_call(mail_t *mbox, unsigned char ch)
{
    unsigned int addr_channel;

    /* Wait for mbox not full */
    while (mmio_get(MBOX_STATUS) & MBOX_FULL) asm volatile ("nop");
    /* Write the address of message to the mbox with channel identifier */
    addr_channel = (((unsigned int)(unsigned long)mbox) & 0xFFFFFFF0) | (ch & 0xF);
    mmio_put(MBOX_WRITE, addr_channel);
    /* Wait for our mbox response */
    do {

        while (mmio_get(MBOX_STATUS) & MBOX_EMPTY) asm volatile("nop");

    } while (mmio_get(MBOX_READ) != addr_channel);

    /* check response vaild */
    return mbox->header.code == MBOX_RESPONSE;
}