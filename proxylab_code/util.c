#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "util.h"

//在pNode后面插入一个元素


void list_init(list_node* pNode){
    pNode->pElem = NULL;
    pNode->pPrev = pNode;
    pNode->pNext = pNode;
}

list_node* list_insert(list_node* pNode,void* pElem)
{
    list_node* pNew = malloc(sizeof(list_node));
    pNew->pElem = pElem;
    pNew->pPrev = pNode;
    pNew->pNext = pNode->pNext;
    pNew->pNext->pPrev = pNew;
    pNode->pNext = pNew;
    return pNew;
}

int list_empty(list_node* pNode){
    if(pNode->pPrev == pNode && pNode->pNext == pNode){
        return 1;
    }else{
        return 0;
    }
}

//删除当前节点, 不帮助释放elem的内存,因为elem的内存不是由list模块分配的
void list_remove(list_node* pNode)
{
    pNode->pPrev->pNext = pNode->pNext;
    pNode->pNext->pPrev = pNode->pPrev;
    free(pNode);
}

//不帮助释放elem的内存,因为elem的内存不是由list模块分配的

void list_free(list_node* pNode){
    list_node* pNow,*pNext;
    pNode->pPrev->pNext = NULL;
    for(pNow = pNode; pNow != NULL; pNow=pNext){
        pNext = pNow->pNext;
        free(pNow);
    }
}

list_node* list_get_first(list_node* pHead){
    return pHead->pNext;
}

list_node* list_get_last(list_node* pHead){
    return pHead->pPrev;
}