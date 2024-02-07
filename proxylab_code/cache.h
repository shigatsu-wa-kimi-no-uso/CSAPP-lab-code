#ifndef __CACHE_H__
#define __CACHE_H__
#include <semaphore.h>
#include "util.h"
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/*
缓存系统由缓存数据结构和缓存索引组成,缓存数据结构采用双向队列,索引采用哈希表
若干个服务线程读取缓存时,使用mutex维护reader_cnt, 当reader_cnt=1时,P(writer_lock)
以键szKey的hash值为哈希表的值,算法采用java的算法, szKey采用请求行
某个线程读取缓存完毕后,立即独占地更新缓存队列和缓存索引,将所读缓存项移至队首
若需要驱逐缓存项,则某个线程立即P(writer_lock)以独占整个缓存,获得缓存队列队尾项并删除之,并从哈希表中删除对应的项
*/

typedef struct list_node hash_node,queue_node,hash_table,metadata_queue;

typedef struct object_meta{
    unsigned int hash32;
    char* szKey;
    size_t objLen;
    time_t timestamp;   //不必要
    void* pObject;
    hash_node* pHashNode;   //元数据在哈希索引中的记录的指针
    queue_node* pQueueNode; //元数据在队列中的记录的指针
}object_meta;


typedef struct cache{
#define HASH_TABLE_GROUP_COUNT 128
    sem_t mutex;
    sem_t index_lock;  //插入缓存用的互斥体
    sem_t queue_lock; //刷新缓存用的互斥体
    int reader_cnt; 
    size_t currentSize;
    hash_table hashTable[HASH_TABLE_GROUP_COUNT]; //采用双向循环链表存储,hashTable[i].pMeta恒为NULL,hashTable[i].pPrev视为链表尾指针,hashTable[i].pNext视为链表首指针,初始时,prev和next都指向自己
    metadata_queue queue; //采用双向循环链表存储,queue.pPrev视为队尾,queue.pNext视为队首,初始时,prev和next都指向自己
}cache;


void init_cache(cache* pCache);

int cache_insert_if_possible(cache* pCache,char* szKey,void* pObject, size_t objLen);

int cache_retrieve_if_possible(cache* pCache,char* szKey,char* buf,size_t* pObjLen);

#endif