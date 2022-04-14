#include "utils.h"

unsigned long align_by_4(unsigned long value) {
    unsigned long lower_bits = value & 0x3;
    value = value - lower_bits;
    if (lower_bits > 0) {
        value += 4;
    }
    return value;
}

void* simple_malloc(size_t size) {
    static void *top = 0;
    void *buttom = top;
    top += size;
    return buttom;
}

void unsignedlonglongToStrHex(unsigned long long num, char *buf)
{
	unsigned int n;
    int size=0;
    for(int c=60; c>=0; c-=4,size++) 
	{
        // get highest tetrad
        n=(num>>c)&0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n+=n>9?0x37:0x30;
        buf[size] = (char)n;
    }
    buf[16] = '\0';
    
}

int isdigit(char num) {
    if (num - '0' > 9 || num - '0' < 0) return 0;
    return 1;
}