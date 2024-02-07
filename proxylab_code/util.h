#ifndef __UTIL_H__
#define __UTIL_H__

typedef struct list_node{
    void* pElem;
    struct list_node* pPrev;
    struct list_node* pNext;
}list_node;


void list_init(list_node* pNode);

list_node* list_insert(list_node* pNode,void* pElem);

int list_empty(list_node* pNode);

void list_remove(list_node* pNode);

void list_free(list_node* pNode);

list_node* list_get_first(list_node* pHead);

list_node* list_get_last(list_node* pHead);

#endif