#ifndef __SERVER_H__
#define __SERVER_H__
#include "csapp.h"



typedef struct connection_queue{
#define QUEUE_CAPACITY 32
    int connfds[QUEUE_CAPACITY];
    int front;      //front%QUEUE_CAPACITY访问队首元素
    int rear;       //rear%QUEUE_CAPACITY指向队尾元素后面的空槽
    sem_t mutex;    //互斥体
    sem_t slot_cnt; //队列空位计数器
    sem_t item_cnt; //队列元素个数计数器
}conn_queue;

void start(char *szPort);

#endif