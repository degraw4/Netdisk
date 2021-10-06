/*by Underwater*/
#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__
#include <math.h>
#include "head.h"
#include "que.h"

typedef struct{
    Que_t que;
    pthread_t *pthid;
    pthread_cond_t cond;
    int threadNum;
    int startFlag;
}Factory_t,*pFactory_t;

typedef struct{
    int len;
    char buf[1024];
}data_t;

typedef struct{//后期改动 存入加密后的psw
    char name[20];
    char psw[20];
    int path[20];
    int pathlen;
}user_t;

void *threadFunc(void *p);
int factoryInit(pFactory_t p,int threadNum,int cap);
int factoryStart(pFactory_t fac);
int tcpInit(int *socketFd,char *ip,char* port);

int put(int newFd);
int reCycle(int fd,void *p,int len);
int download_file(int newFd,int id);
int redownload_file(int newFd,int id,off_t offset);
int upload_file(int socketFd,char *name);

int getuser(int fd,user_t *user);

int sql_connect();
int sql_close();
int login_query(user_t user);
int reg_query(user_t user);
int reg_insert(user_t user);
int pwd_query(user_t user,char *buf);
int path_query(int id,user_t user,char *buf);
int ls_query(int id,user_t user,char *dir,char *file);
int cd_query(int *k,user_t *user,char *dir);
int exist_query(int id,user_t user,char *ls);
int sql_delete(int id);
int md5_query(char *md5);
int name_query(int precode,user_t user,char *name);
int sql_input(int precode,user_t user,char *name,char *md5,int fileID,long size);
int id_update(int id);
int size_update(int id,long size);
int del_exist_query(int id,user_t user,char *ls);
int mkdir_insert(int precode,user_t user,char *dir);

int log_start();
int log_end();
int log_op(user_t user,int op,int res,char *info);

long long Mod(long long a, long long  b, long long m);
long long * rsaEncode(char * message);
void rsaDecode(long long code[], size_t n,char *message);

#endif