#ifndef SIMULATOR_H
#define SIMULATOR_H
#include "common.h"
#include "opparser.h"
#include "cache.h"

typedef struct SIMULATOR SIMULATOR;

typedef enum ACCESS_STATE                                //缓存访问状态枚举
{
    COLD_MISS,CONFLICT_MISS,HIT,EVICTION,WARMED_UP       //冷不命中(组中的缓存行尚未被占满),冲突不命中(缓存行全满),命中,驱逐(将缓存块的值写回内存,并让位给其他数据),缓存变暖(读取值到空的缓存)
}ACCESS_STATE;

typedef enum ACCESS_MODE                                //缓存访问类型枚举(读/写)
{
    READ,WRITE
}ACCESS_MODE;

struct SIMULATOR
{
    ACCESS_STATE lastState;                             //上次访问时的状态,每次accessCache,loadToCache和evictCache被调用时都会修改这个枚举变量
    int stateCnt[5];                                    //四种访问状态计数,对应上面的ACCESS_STATE枚举
    char* szAccessState[4];                             //四种访问状态字符串,以供打印({"miss","miss","hit","eviction"})
    QWORD lineForUse;                                   //可用行(根据最近最少使用规则LRU得出的待驱逐行,或者是有效位为0的空缓存行)
    QWORD accessOrder;
    void (*accessCache)(SIMULATOR* thisObj,CACHE* cache,QWORD setIndex,QWORD tag,QWORD blockOffset,ACCESS_MODE mode,BYTE size,BYTE data[]);
    void (*loadToCache)(SIMULATOR* thisObj,CACHE* cache,QWORD setIndex,QWORD tag,DWORD lineId,BYTE data[]);
    void (*evictCache)(SIMULATOR* thisObj,CACHE* cache,QWORD setIndex,DWORD lineId,BYTE data[]);
    void (*operate)(SIMULATOR* thisObj,CACHE* cache,OPINFO* opinfo,BOOL isVerboseOn);
};


SIMULATOR* new_SIMULATOR();                                 //构造
void delete_SIMULATOR(SIMULATOR* thisObj);                  //析构
void accessCache(SIMULATOR* thisObj,CACHE* cache,QWORD setIndex,QWORD tag,QWORD blockOffset,ACCESS_MODE mode,BYTE size,BYTE data[]); 
void loadToCache(SIMULATOR* thisObj,CACHE* cache,QWORD setIndex,QWORD tag,DWORD lineId,BYTE data[]);
void evictCache(SIMULATOR* thisObj,CACHE* cache,QWORD setIndex,DWORD lineId,BYTE data[]);
void operate(SIMULATOR* thisObj,CACHE* cache,OPINFO* opinfo,BOOL isVerboseOn); //模拟操作,对操作信息处理,模拟缓存访问/驱逐/加载的行为

#endif


