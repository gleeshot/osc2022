#ifndef __LIST__H__
#define __LIST__H__

#include "utils.h"

typedef struct list_t
{
    /* data */
    int index;
    struct list_t *next, *prev;
    
} list_t;

void list_init(list_t *list);
void list_push(list_t *list, list_t *entry);
void list_remove(list_t *entry);
list_t* list_pop(list_t *list);
bool list_empty(list_t *list);

#endif