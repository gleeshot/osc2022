#ifndef __MM__H__
#define __MM__H__

#include "list.h"
#include "mini_uart.h"
#include "mmu_def.h"
#include "dtb.h"
#include "utils.h"
#include "exception.h"

#define PAGE_SIZE 4096    // page size = 4kb
#define MAX_BUDDY_ORDER 16 // 4kb to 1mb

#define BUDDY_BASE (0xFFFF000000000000)
#define alloc_start (0x00000000)
#define alloc_end (0x30000000)  // 2 ^ 24 bytes
#define BUDDY_SIZE (0x30000000)

#define PAGE_NUM ((alloc_end - alloc_start) / PAGE_SIZE)

#define MAX_POOL_PAGES 16
#define MAX_OBJ_ALLOCTOR_NUM 32

#define SPIN_TABLE_START (0x0000)

extern unsigned long __start;
extern unsigned long _end;
static unsigned long kernel_start = (unsigned long) &__start;
static unsigned long kernel_end = (unsigned long) &_end;
extern uint32_t CPIO_BASE;

typedef enum
{
    AVAL,
    USED
} booking_status;

/* buddy allocator */
typedef struct page_t
{
    list_t list;
    booking_status used;
    int order;
    int idx;
    bool reserve;

} page_t;

typedef struct buddy_t
{
    uint32_t free_num;
    list_t head;
} buddy_t;

void buddy_init();
void page_init();
void mm_init(uint64_t DTB_BASE, uint32_t dtb_size);
void buddy_push(buddy_t *bd, list_t *p);
void buddy_remove(buddy_t *bd, list_t *elemt);
void *buddy_alloc(int order);
page_t *buddy_pop(buddy_t *bd, int target_order);
page_t *release_redundant(page_t *p, int target_order);
void buddy_merge(page_t *lower, page_t *top, int order);
void buddy_free(void *addr);

// advance
void startup_alloc(uint64_t DTB_BASE, uint32_t dtb_size);
void *page_idx_to_addr(int idx);
void memory_reserve(unsigned long start, unsigned long end);

/* obj allocator */
typedef struct single_list
{
    struct single_list *next;
} single_list;

typedef struct pool_t
{
    booking_status used;
    int obj_size;
    int obj_per_page;
    int obj_used;
    int page_used;
    uint64_t page_addr[MAX_POOL_PAGES];
    single_list * free;
} pool_t;

void pool_init(pool_t *pool, uint64_t size);
int size_register(uint64_t size);
void *obj_alloc(int token);
void obj_free(int token, void *virt_addr);

/* kmalloc */
void *kmalloc(uint64_t size);
void kfree(void *addr);

/* debug */
void page_info(int id);
void buddy_info();

unsigned int page_num_to_order(unsigned int page_num);

#endif