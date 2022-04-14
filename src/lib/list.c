#include "list.h"

void list_init(list_t *list)
{
    list->prev = list;
    list->next = list;
}

/*
 * append the provided entry to the end of the list
 */
void list_push(list_t *list, list_t *entry)
{
    list_t *prev = list->prev;
    entry->prev = prev;
    entry->next = list;
    prev->next = entry;
    list->prev = entry;
}

/*
 * remove the provided entry from whichever list it's currently in
 */
void list_remove(list_t *entry)
{
    list_t *prev = entry->prev;
    list_t *next = entry->next;
    prev->next = next;
    next->prev = prev;
}

/*
 * remove and return the last entry in the list or NULL if the list is empty
 */
list_t *list_pop(list_t *list)
{
    list_t *back = list->prev;
    if (back == list)
        return NULL;
    list_remove(back);
    return back;
}

bool list_empty(list_t *list)
{
    return list->next == list ? true : false;
}