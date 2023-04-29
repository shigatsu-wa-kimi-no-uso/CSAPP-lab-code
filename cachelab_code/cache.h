#ifndef CACHELINE_H
#define CACHELINE_H
#include "common.h"

typedef struct CACHELINE CACHELINE;
typedef struct CACHESET CACHESET;
typedef struct CACHE CACHE;

struct CACHELINE
{
    BOOL isValid;               //有效位
    QWORD tag;                  //标签位
    BYTE* data;                 //缓存块
    QWORD accessOrder;          //访问次序(标志着访问时间的先后)
};

struct CACHESET
{
    CACHELINE** cacheLine;      //缓存行数组
};

struct CACHE
{
    DWORD lineCnt;              //行数
    DWORD setCnt;               //组数
    DWORD blockCnt;             //缓存块字节数
    CACHESET** cacheSet;        //缓存组数组
};

/*构造函数*/
CACHELINE* new_CACHELINE(DWORD blockCnt);
CACHESET* new_CACHESET(DWORD lineCnt,DWORD blockCnt);
CACHE* new_CACHE(DWORD setCnt,DWORD lineCnt,DWORD blockCnt);

/*析构函数*/
void delete_CACHELINE(CACHELINE* obj);
void delete_CACHESET(CACHESET* obj,DWORD lineCnt);
void delete_CACHE(CACHE* obj);

#endif