#include "./include/mm.h"

buddy_t free_area[MAX_BUDDY_ORDER];
page_t page[PAGE_NUM];
pool_t obj_allocator[MAX_OBJ_ALLOCTOR_NUM];

void page_info(int id)
{
    uart_hex(page[id].order);
}

void buddy_init()
{
    for (int i = 0; i < MAX_BUDDY_ORDER; i++)
    {
        free_area[i].head.next = &free_area[i].head;
        free_area[i].head.prev = &free_area[i].head;
        free_area[i].free_num = 0;
    }
}

/*
 * implement the buddy system only for 0x10000000 to 0x20000000
 * no need to consider other used memory segments now
 */
void page_init()
{
    int cur_order = MAX_BUDDY_ORDER - 1;
    int counter = 0;
    for (int i = 0; i < PAGE_NUM; i++)
    {
        if (counter)
        {
            page[i].idx = i;
            page[i].used = AVAL;
            page[i].order = -1; // order = -1 if this page is available but it belongs to larger contiguous memory block
            page[i].list.next = NULL;
            page[i].list.prev = NULL;
            counter--;
        }
        else if (i + (1 << cur_order) - 1 < PAGE_NUM)
        {
            page[i].idx = i;
            page[i].used = AVAL;
            page[i].order = cur_order;
            buddy_push(&(free_area[cur_order]), &(page[i].list));
            counter = (1 << cur_order) - 1;
        }
        else
        {
            cur_order--;
            i--;
        }
    }
}

void mm_init()
{
    buddy_init();
    page_init();
    for (int i = 0; i < MAX_OBJ_ALLOCTOR_NUM; i++)
    {
        obj_allocator[i].used = AVAL; // init obj allocator
    }
}

/*
 * push an available memory block into buddy
 */
void buddy_push(buddy_t *bd, list_t *p)
{
    bd->free_num++;
    p->next = &(bd->head);
    p->prev = bd->head.prev;
    bd->head.prev->next = p;
    bd->head.prev = p;
}

/*
 * remove an available memory block from buddy
 */
void buddy_remove(buddy_t *bd, list_t *elemt)
{
    bd->free_num--;
    elemt->prev->next = elemt->next;
    elemt->next->prev = elemt->prev;
    elemt->next = NULL;
    elemt->prev = NULL;
}

/*
 * allocate memory block and return address of the first page
 */
void *buddy_alloc(int order)
{
    for (int i = order; i < MAX_BUDDY_ORDER; i++)
    {
        if (free_area[i].free_num > 0)
        {
            page_t *p = buddy_pop(&free_area[i], order);
            uint64_t page_virt_addr = (p->idx * PAGE_SIZE) + alloc_start;
            uart_puts("allocate pages from 0x");
            uart_hex(page_virt_addr);
            uart_puts(" to 0x");
            uint64_t end_addr = page_virt_addr + (1 << (p->order)) * PAGE_SIZE;
            uart_hex(end_addr);
            uart_puts("\r\n");
            return (void *)page_virt_addr;
        }
    }
    return NULL; // return NULL if the order cannot be allocated
}

void buddy_info()
{
    uart_puts("num of free lists in buddy\n");
    for (int i = 0; i < MAX_BUDDY_ORDER; i++)
    {
        uart_hex(free_area[i].free_num);
        uart_puts("\r\n");
    }
}

/* get an available block from buddy and release redundant space to target order */
page_t *buddy_pop(buddy_t *bd, int target_order)
{
    if (bd->free_num == 0)
        return NULL;
    bd->free_num--;
    list_t *target = bd->head.next; // get first item in buddy
    target->next->prev = target->prev;
    target->prev->next = target->next;
    target->next = NULL;
    target->prev = NULL;
    return release_redundant((page_t *)target, target_order);
}

/*
 * release redundant recursively
 */
page_t *release_redundant(page_t *p, int target_order)
{
    uart_puts("release: ");
    page_info(p->idx);
    uart_puts(" target: ");
    uart_hex(target_order);
    uart_puts("\r\n");

    int cur_order = p->order;
    if (cur_order > target_order)
    {
        int prev_order = cur_order - 1;
        page_t *released = p;
        page_t *new_p = p + (1 << prev_order);
        released->order = prev_order;
        new_p->order = prev_order;
        buddy_push(&(free_area[prev_order]), &(released->list));
        return release_redundant(new_p, target_order);
    }

    // label all used pages to USED
    for (int i = 0; i < (1 << target_order); i++)
    {
        (p + i)->used = USED;
    }
    return p;
}

/*
 * find buddy and return pointer to it
 */
page_t *find_buddy(int pfn, int order)
{
    int buddy_pfn = pfn ^ (1 << order);
    return &(page[buddy_pfn]);
}

void buddy_merge(page_t *lower, page_t *top, int order)
{
    int new_o = order + 1;

    // merge message
    uart_puts("merge from order ");
    uart_hex(order);
    uart_puts(" to order ");
    uart_hex(new_o);
    uart_puts(" (");
    uart_hex(lower->idx);
    uart_puts(" -> ");
    uart_hex(top->idx);
    uart_puts(")\n");

    page_t *buddy = find_buddy(lower->idx, new_o);

    if (buddy->used == AVAL && buddy->order == new_o && new_o < MAX_BUDDY_ORDER - 1)
    {
        uart_hex(new_o);
        // there is a larger buddy to be merged recursively
        buddy_remove(&(free_area[new_o]), &(buddy->list));
        if (buddy > lower)
        {
            buddy_merge(lower, buddy, new_o);
        }
        else
        {
            buddy_merge(buddy, lower, new_o);
        }
    }
    else
    {
        // just merge
        lower->order = new_o;
        lower->used = AVAL;
        top->order = -1;
        buddy_push(&(free_area[new_o]), &(lower->list));
    }
}

void memzero(page_t *p)
{
    int counter = 1 << p->order;
    uint64_t *page_addr = (p->idx * PAGE_SIZE) + alloc_start;
    for (int i = 0; i < counter; i++)
    {
        *(page_addr + i * PAGE_SIZE) = 0;
    }
}

void buddy_free(void *addr)
{
    int pfn = ((uint64_t)addr - alloc_start) / PAGE_SIZE;
    page_t *p = &(page[pfn]);
    int order = p->order;
    page_t *buddy = find_buddy(pfn, order); // find the buddy of the given page

    // check if the buddy can be merged
    if (buddy->used == USED || buddy->order != order)
    {
        uart_hex((buddy->idx) * PAGE_SIZE + alloc_start);
        // buddy is used or it's order is different, cannot be merged
        uart_puts(" cannot be merged, just free ");
        uart_hex((p->idx) * PAGE_SIZE + alloc_start);
        uart_puts("\r\n");
        for (int i = 0; i < (1 << order); i++)
        {
            (p + i)->used = AVAL;
        }
        buddy_push(&(free_area[order]), &(p->list));
    }
    else
    {
        buddy_remove(&(free_area[order]), &(buddy->list));
        if (buddy > p)
        {
            buddy_merge(p, buddy, order);
        }
        else
        {
            buddy_merge(buddy, p, order);
        }
    }
}

void pool_init(pool_t *pool, uint64_t size)
{
    pool->obj_size = size;
    pool->obj_per_page = PAGE_SIZE / size;
    pool->obj_used = 0;
    pool->page_used = 0;
    pool->free = NULL;
}

/*
 * return a token for a specific allocated size
 */
int size_register(uint64_t size)
{
    if (size > PAGE_SIZE)
    {
        uart_puts("object too large!\n");
        return -1;
    }

    for (int i = 0; i < MAX_OBJ_ALLOCTOR_NUM; i++)
    {
        if (obj_allocator[i].used == AVAL)
        {
            // take this slot as allocator if available
            obj_allocator[i].used = USED;
            pool_init(&(obj_allocator[i]), size);
            return i;
        }
    }
    uart_puts("no available pool for the request\n");
    return -1;
}

/*
 * object allocation, return addr
 */
void *obj_alloc(int token)
{
    pool_t *pool = &(obj_allocator[token]);

    // if there are some objs available in allocator
    if (pool->free != NULL)
    {
        single_list *obj = pool->free;
        pool->free = pool->free->next;
        return (void *)obj;
    }

    // need new page
    if (pool->obj_used >= pool->page_used * pool->obj_per_page)
    {
        pool->page_addr[pool->page_used] = (uint64_t)buddy_alloc(0);
        pool->page_used++;
    }

    // allocate obj
    uint64_t addr = pool->page_addr[pool->page_used - 1] + pool->obj_used * pool->obj_size;
    pool->obj_used++;
    return (void *)addr;
}

void obj_free(int token, void *virt_addr)
{
    int pool_num = token;
    pool_t *pool = &obj_allocator[pool_num];
    single_list *free_head = pool->free;
    pool->free = (single_list *)virt_addr;
    pool->free->next = free_head;
    pool->obj_used--;
}

void *kmalloc(uint64_t size)
{
    if (size >= PAGE_SIZE)
    {
        uart_puts("allocate with buddy system\n");
        int order;

        for (int i = 0; i < MAX_BUDDY_ORDER; i++)
        {
            if (size <= (uint64_t)((1 << i) * PAGE_SIZE))
            {
                order = i;
                break;
            }
        }
        return buddy_alloc(order);
    }
    else
    {
        uart_puts("allocate with obj allocator\n");
        for (int i = 0; i < MAX_OBJ_ALLOCTOR_NUM; i++)
        {
            if ((uint64_t)obj_allocator[i].obj_size == size)
                return obj_alloc(i);
        }

        int token = size_register(size);
        return obj_alloc(token);
    }
}

void kfree(void *addr)
{
    for (int i = 0; i < MAX_OBJ_ALLOCTOR_NUM; i++)
    {
        pool_t *pool = &obj_allocator[i];
        for (int j = 0; j < pool->page_used; j++)
        {
            int addr_pfn = ((uint64_t)addr - alloc_start) >> 12;
            int page_pfn = (pool->page_addr[j] - alloc_start) / PAGE_SIZE;
            if (addr_pfn == page_pfn)
            {
                uart_puts("free using obj allocator\n");
                obj_free(i, addr);
                return;
            }
        }
    }
    uart_puts("free using buddy\n");
    buddy_free(addr);
}