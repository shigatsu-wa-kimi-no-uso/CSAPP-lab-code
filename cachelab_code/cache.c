#include "cache.h"

CACHELINE* new_CACHELINE(DWORD blockCnt)
{
    CACHELINE* obj=(CACHELINE*)malloc(sizeof(CACHELINE));
    assert(obj);
    obj->isValid=0;
    obj->accessOrder=0;
    obj->tag=0;
    obj->data=(BYTE*)calloc(sizeof(BYTE),blockCnt); 
    return obj;
}

CACHESET* new_CACHESET(DWORD lineCnt,DWORD blockCnt)
{
    CACHESET* obj=(CACHESET*)malloc(sizeof(CACHESET));
    obj->cacheLine=(CACHELINE**)malloc(sizeof(CACHELINE*)*lineCnt);  //为指针数组开辟空间
    for(int i=0;i<lineCnt;i++)                                      //为指针数组中的每个元素单独开辟空间
    {
        obj->cacheLine[i]=new_CACHELINE(blockCnt);
    }
    return obj;
}

CACHE* new_CACHE(DWORD setCnt,DWORD lineCnt,DWORD blockCnt)
{
    CACHE* obj=(CACHE*)malloc(sizeof(CACHE));
    obj->cacheSet=(CACHESET**)malloc(sizeof(CACHESET*)*setCnt);
    for(int i=0;i<setCnt;i++)
    {
        obj->cacheSet[i]=new_CACHESET(lineCnt,blockCnt);
    }
    obj->lineCnt=lineCnt;
    obj->setCnt=setCnt;
    obj->blockCnt=blockCnt;
    return obj;
}

void delete_CACHELINE(CACHELINE* obj)
{
    free(obj->data);
    free(obj);
}

void delete_CACHESET(CACHESET* obj,DWORD lineCnt)
{
    for(int i=0;i<lineCnt;i++)
    {
        delete_CACHELINE(obj->cacheLine[i]);
    }
    free(obj->cacheLine);
    free(obj);
}

void delete_CACHE(CACHE* obj)
{
    for(int i=0;i<obj->setCnt;i++)
    {
        delete_CACHESET(obj->cacheSet[i],obj->lineCnt);
    }
    free(obj->cacheSet);
    free(obj);
}