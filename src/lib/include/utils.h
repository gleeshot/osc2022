#ifndef __UTILS__H__
#define __UTILS__H__

typedef unsigned int            size_t;
typedef enum { false, true }    bool;
#define NULL (void *)(0)

typedef unsigned char           uint8_t;
typedef unsigned short int      uint16_t;
typedef unsigned int            uint32_t;
typedef unsigned long long      uint64_t;

unsigned long align_by_4(unsigned long value);
void* simple_malloc(size_t size);
void unsignedlonglongToStrHex(unsigned long long num, char *buf);
int isdigit(char num);

#endif