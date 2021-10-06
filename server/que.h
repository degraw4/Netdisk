/*by Underwater*/
#ifndef __QUE_H__
#define __QUE_H__
#include "head.h"

typedef struct node{
    int newFd;
    struct node *next;
    int flag;//0为新连接 1为get put
}Node_t,*pNode_t;

typedef struct{
    pNode_t head,tail;
    int queCap;
    int queSize;
    pthread_mutex_t mutex;
}Que_t,*pQue_t;

int queInit(pQue_t q,int cap);
int quePush(pNode_t newNode,pQue_t q);
int queGet(pQue_t q,pNode_t *task);
#endif