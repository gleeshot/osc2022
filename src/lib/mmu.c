#include "mmu.h"

unsigned long va_to_pa(unsigned long va)
{
    unsigned long pa;
    pa = va & 0x0000ffffffffffff;

    return pa;
}

unsigned long pa_to_va(unsigned long pa)
{
    unsigned long va;
    va = pa | 0xffff000000000000;

    return va;
}

unsigned long user_va_to_pa(unsigned long user_va)
{
    unsigned long offset;
    unsigned long *cur_table;
    unsigned long pd_type;
    unsigned long pa = 0;

    asm volatile("mrs %0, ttbr0_el1"
                 : "=r"(cur_table));
    cur_table = pa_to_va(cur_table);

    // uart_puts("[va_user_to_kernel] cur_table: ");
    // uart_hex((unsigned long)cur_table >> 32);
    // uart_hex((unsigned long)cur_table);
    // uart_puts("\r\n");

    for (int level = 3; level >= 0; level--)
    {
        offset = (user_va >> (12 + level * 9)) & 0x1FF;
        pd_type = (cur_table[offset] & 3);

        if (pd_type == PD_TABLE)
        {
            cur_table = (unsigned long *)(pa_to_va(cur_table[offset] & ~0xfff));
        }
        else if (pd_type == PD_BLOCK)
        {
            cur_table = (unsigned long *)(pa_to_va(cur_table[offset] & ~0xfff));
            break;
        }
        else
        {
            uart_puts("[va_user_to_kernel] Error, encounter an invalid entry.\r\n");
        }
    }

    pa = ((unsigned long)cur_table) | (user_va & 0xFFF);
    pa = va_to_pa(pa);

    // uart_puts("[va_user_to_kernel] user_va: ");
    // uart_hex(user_va >> 32);
    // uart_hex(user_va);
    // uart_puts(", pa: ");
    // uart_hex(pa >> 32);
    // uart_hex(pa);
    // uart_puts("\r\n");

    return pa;
}

unsigned long *create_page_table()
{
    unsigned long *page_table = buddy_alloc(0);
    page_table_init(page_table);

    return page_table;
}

void page_table_init(unsigned long *page_table)
{
    for (int i = 0; i < PAGE_TABLE_ENTRY_NUM; i++)
    {
        page_table[i] = 0;
    }

    // uart_puts("[page_table_init] *page_table: 0x");
    // uart_hex((unsigned long)page_table >> 32);
    // uart_hex((unsigned long)page_table);
    // uart_puts("\r\n");

    return;
}

void set_page_table(unsigned long *page_table, unsigned long offset, unsigned long next_level, unsigned long attribute)
{
    page_table[offset] = next_level | attribute;
}

unsigned long *alloc_page_table(unsigned long *pgd, unsigned long va, unsigned long pa, unsigned attribute)
{
    unsigned long offset;
    unsigned long *cur_table = pgd;
    unsigned long *page_va;

    // Four level page table:
    //     - level = 3 : PGD -> PUD
    //     - level = 2 : PUD -> PMD
    //     - level = 1 : PMD -> PTE
    for (int level = 3; level > 0; level--)
    {
        offset = (va >> (12 + level * 9)) & 0x1FF; // 9 bits

        if ((cur_table[offset] & 1) == 0) // entry not alloc yet
        {
            // alloc a table for this entry
            cur_table[offset] = va_to_pa((unsigned long)create_page_table());
            // set attribute
            cur_table[offset] = cur_table[offset] | PD_TABLE;
        }

        // move to next level
        cur_table = (unsigned long *)(pa_to_va(cur_table[offset] & ~0xfff));
    }

    // PTE -> physical page
    offset = (va >> 12) & 0x1FF;

    if (pa)
    {
        cur_table[offset] = pa | attribute | PD_PAGE;
        page_va = (unsigned long *)pa_to_va(pa);
    }
    else
    {
        // If no physical page provided, allocate a page
        page_va = buddy_alloc(0);
        cur_table[offset] = va_to_pa((unsigned long)page_va) | attribute | PD_PAGE;
    }

    // uart_puts("[alloc_page_table] pgd: 0x");
    // uart_hex((unsigned long)pgd >> 32);
    // uart_hex((unsigned long)pgd);
    // uart_puts(", pa: 0x");
    // uart_hex((unsigned long)pa_to_va(page_va) >> 32);
    // uart_hex((unsigned long)pa_to_va(page_va));
    // uart_puts("\n");
    return page_va;
}