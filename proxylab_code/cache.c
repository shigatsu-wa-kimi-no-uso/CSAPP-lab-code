#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "csapp.h"
#include "cache.h"

unsigned int get_string_hash32(char* s,size_t len){
    int i;
    unsigned int h = 0;
    for(i=0;i<len;i++){
        h = 31*h + s[i];
    }
    return h;
}

void init_cache(cache* pCache){
    int i;
    for(i=0;i<HASH_TABLE_GROUP_COUNT;i++){
        list_init(&pCache->hashTable[i]);
    }
    list_init(&pCache->queue);
    pCache->reader_cnt = 0;
    Sem_init(&pCache->mutex,0,1);
    Sem_init(&pCache->index_lock,0,1);
    Sem_init(&pCache->queue_lock,0,1);
}


void create_hash_index(cache* pCache,object_meta* pMeta){
    pMeta->pHashNode = list_insert(&pCache->hashTable[pMeta->hash32%HASH_TABLE_GROUP_COUNT],pMeta);   //建立联系
}

void push_front_metadata_queue_node(cache* pCache,object_meta* pMeta){
    pMeta->pQueueNode = list_insert(&pCache->queue,pMeta);  //建立联系
}

void remove_metadata_queue_node(cache* pCache,object_meta* pMeta){
    list_remove(pMeta->pQueueNode);
}

object_meta* pack_object(char* szKey,void* pObject, size_t objLen){
    object_meta* pMeta = malloc(sizeof(object_meta));
    size_t szKeyLen = strlen(szKey);
    pMeta->hash32 = get_string_hash32(szKey,szKeyLen);
    pMeta->timestamp = time(NULL);
    pMeta->szKey = malloc(szKeyLen*sizeof(char));
    pMeta->objLen = objLen;
    pMeta->pObject = malloc(objLen);
    memcpy(pMeta->pObject,pObject,objLen);
    memcpy(pMeta->szKey,szKey,szKeyLen);
    return pMeta;
}

void free_object(object_meta* p){
    free(p->szKey);
    free(p->pObject);
    free(p);
}

void evict_one_by_LRU(cache* pCache){
    queue_node* pOldRear = list_get_first(&pCache->queue);
    object_meta* pExpiringMeta = pOldRear->pElem;
    size_t objLen = pExpiringMeta->objLen;
    list_remove(pOldRear); //从队列中删除
    list_remove(pExpiringMeta->pHashNode); //从哈希索引中删除
    free_object(pExpiringMeta);
    pCache->currentSize -= objLen;
}

int cache_insert_if_possible(cache* pCache,char* szKey,void* pObject, size_t objLen){
    if(objLen > MAX_OBJECT_SIZE){
        return 0;
    }
    object_meta* pMeta = pack_object(szKey,pObject,objLen);
    P(&pCache->index_lock); //获取了index_lock后, queue_lock必然是未被占用的, 因为queue_lock的释放一定在index_lock释放前
    while(pCache->currentSize + objLen > MAX_CACHE_SIZE){
        evict_one_by_LRU(pCache);
    }
    push_front_metadata_queue_node(pCache,pMeta);
    create_hash_index(pCache,pMeta);
    pCache->currentSize += pMeta->objLen;
    V(&pCache->index_lock);
    return 1;
}


int cache_retrieve_if_possible(cache* pCache,char* szKey,char* buf,size_t* pObjLen){
    unsigned int hash32 = get_string_hash32(szKey,strlen(szKey));
    int success = 0;
    hash_table* pTable = &pCache->hashTable[hash32%HASH_TABLE_GROUP_COUNT];
    P(&pCache->mutex);
    pCache->reader_cnt++;
    if(pCache->reader_cnt == 1){ //第一个读者需要占用index_lock,防止缓存插入改变哈希索引
        P(&pCache->index_lock);
    }
    V(&pCache->mutex);

    hash_node* pNow,*pFirst = list_get_first(pTable), *pHead = pTable;
    for(pNow = pFirst; pNow!= pHead; pNow=pNow->pNext){
        object_meta* pMeta = pNow->pElem;
        if(strcmp(pMeta->szKey,szKey) == 0){
            P(&pCache->queue_lock); //刷新缓存时需要占用queue_lock, 防止其他线程更新缓存队列
            remove_metadata_queue_node(pCache,pMeta);
            pMeta->timestamp = time(NULL);
            push_front_metadata_queue_node(pCache,pMeta);
            V(&pCache->queue_lock);
            memcpy(buf,pMeta->pObject,pMeta->objLen);
            *pObjLen=pMeta->objLen;
            success = 1;
            break;
        }
    }

    P(&pCache->mutex);
    pCache->reader_cnt--;
    if(pCache->reader_cnt == 0){ //最后一个读者需要释放writer_lock
        V(&pCache->index_lock);
    }
    V(&pCache->mutex);
    return success;
}