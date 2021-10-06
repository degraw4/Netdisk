/*by Underwater*/
#ifndef __CLIENT_H__
#define __CLIENT_H__
#include <math.h>
#include "md5.h"
#define LIGHT_GREEN  "\033[1;32m"
#define BLUE         "\e[1;34m"
#define YELLOW       "\e[1;33m"
#define NONE         "\e[m"

typedef struct{
    pid_t pid;
    short busy;
    int fd;
}process_t;

typedef struct{//后期改动 存入加密后的psw
    char name[20];
    char psw[20];
    int path[20];
    int pathlen;
}user_t;

typedef struct{
    user_t user;
    char file[20];
    char ip[20];
    char port[10];
}info_t;

typedef struct{
    int len;
    char buf[1024];
}data_t;


int tcpInit(int *socketFd,char *ip,char *port);

int reCycle(int fd,void *p,int len); 
void Print(int i);
void get(int socketFd);
void download_file(int socketFd,char *name);
int redownload_file(int socketFd,char *name,off_t size);
int upload_file(int newFd,char *name);


int getLogin();
int login(int a,int socketFd,user_t *u);
int getpsw(char *buf);

void *th_download(void *p);
void *th_upload(void *p);

long long Mod(long long a, long long  b, long long m);
long long * rsaEncode(char * message);
void rsaDecode(long long code[], size_t n,char *message);

#endif