/*by Underwater*/
#include "thread_pool.h"
long long N = 6651937;
long long e = 13007, d = 511;

int tcpInit(int *socketFd,char *ip,char *port){
    int fd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in ser;
    bzero(&ser,sizeof(ser));
    ser.sin_family=AF_INET;
    ser.sin_port=htons(atoi(port));
    ser.sin_addr.s_addr=inet_addr(ip);
    int reuse=1;
    int ret=setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(int));
    ERROR_CHECK(ret,-1,"setsockopt");
    ret=bind(fd,(struct sockaddr *)&ser,sizeof(struct sockaddr));
    ERROR_CHECK(ret,-1,"bind");
    listen(fd,10);
    *socketFd=fd;
    return 0;
}

int queInit(pQue_t q,int cap){
    bzero(q,sizeof(Que_t));
    q->queCap=cap;
    pthread_mutex_init(&q->mutex,NULL);
    return 0;
}

int quePush(pNode_t newNode,pQue_t q){
    if(!q->queSize){
        q->head=q->tail=newNode;
    }else{
        q->tail->next=newNode;
        q->tail=newNode;
    }
    q->queSize++;
    return 0;
}

int queGet(pQue_t q,pNode_t *pTask){
    if(q->queSize){
        *pTask=q->head;
        q->head=q->head->next;
        q->queSize--;
        if(!q->queSize){
            q->head=NULL;
            q->tail=NULL;
        }
    }
    return 0;
}

int getuser(int fd,user_t *user){
	char buf[20];
    int len,s;
    bzero(buf,sizeof(buf));
	recv(fd,buf,sizeof(buf),0);
    int ret=1;
	if(strcmp(buf,"1")==0){//登录
        while(ret){
            //接用户名
            bzero(buf,sizeof(buf));
            bzero(user,sizeof(user_t));
            s=reCycle(fd,&len,4);
            if(s==-1){
                    return -1;
                }
            s=reCycle(fd,buf,len);
            if(s==-1){
                    return -1;
                }
            strcpy((*user).name,buf);
            //接密码
            bzero(buf,sizeof(buf));
            s=reCycle(fd,&len,4);
            if(s==-1){
                    return -1;
                }
            s=reCycle(fd,buf,len);
            if(s==-1){
                    return -1;
                }
            long long *code=(long long*)malloc(len);
            memcpy(code,buf,len);
            rsaDecode(code,len/sizeof(long long),(*user).psw);
            //查询
            ret=login_query(*user);
            if(ret==0){//成功
                s=send(fd,"0",1,0);
                (*user).path[0]=-1; 
                (*user).pathlen=1;
                log_op(*user,1,1,NULL);
                if(s==-1){
                    return -1;
                }
            }else if(ret==1){//查无此人
                log_op(*user,1,0,NULL);
                s=send(fd,"1",1,0);
                if(s==-1){
                    return -1;
                }
            }else if(ret==2){//密码错误
                log_op(*user,1,0,NULL);
                s=send(fd,"2",1,0);
                if(s==-1){
                    return -1;
                }
            }
        }
    }else{//注册
        while(ret){
            //接用户名
            bzero(buf,sizeof(buf));
            bzero(user,sizeof(user_t));
            s=reCycle(fd,&len,4);
            if(s==-1){
                    return -1;
                }
            s=reCycle(fd,buf,len);
            if(s==-1){
                    return -1;
                }
            strcpy((*user).name,buf);
            ret=reg_query(*user);
            if(ret==1){//重名
                log_op(*user,2,0,NULL);
                s=send(fd,"1",1,0);
                if(s==-1){
                    return -1;
                }
            }else if(ret==0){
                s=send(fd,"0",1,0);
                break;
            }
        }
        //接密码
        bzero(buf,sizeof(buf));
        s=reCycle(fd,&len,4);
        if(s==-1){
                return -1;
            }
        s=reCycle(fd,buf,len);
        if(s==-1){
                return -1;
            }
        long long *code=(long long*)malloc(len);
        memcpy(code,buf,len);
        rsaDecode(code,len/sizeof(long long),(*user).psw);
        //注册
        ret=reg_insert(*user);
        if(ret==0){
            send(fd,"0",1,0);
            (*user).path[0]=-1;
            (*user).pathlen=1;
            log_op(*user,2,1,NULL);
        }else{
            send(fd,"1",1,0);
            log_op(*user,2,0,NULL);
        }
    }
    return 0;
}

int log_start(){
    int fd=open("log",O_RDWR|O_CREAT|O_APPEND,0666);
    ERROR_CHECK(fd,-1,"open");
    time_t now;
    char buf[128]={0};
    time(&now);
    memcpy(buf,ctime(&now),strlen(ctime(&now))-1);
    sprintf(buf,"%s%s",buf," Server is online.\n");
    write(fd,buf,strlen(buf));
    close(fd);
    return 0;
}

int log_end(){
    int fd=open("log",O_RDWR|O_APPEND);
    ERROR_CHECK(fd,-1,"open");
    time_t now;
    char buf[128]={0};
    time(&now);
    memcpy(buf,ctime(&now),strlen(ctime(&now))-1);
    sprintf(buf,"%s%s",buf," Server is offline.\n");
    write(fd,buf,strlen(buf));
    write(fd,"\n",1);
    close(fd);
    return 0;
}

int log_op(user_t user,int op,int res,char *info){
    /* res 0 失败 1 成功
    op码
    0 客户发缓冲信息
    1 登录
    2 注册
    3 客户退出
    4 ls
    5 pwd
    6 cd
    7 put 下载线程记录
    8 get 上传线程记录
    9 rm 
    10 mkdir
    log_op(user,0,0,NULL);
    */
    int fd=open("log",O_RDWR|O_CREAT|O_APPEND,0666);
    ERROR_CHECK(fd,-1,"open");
    time_t now;
    char buf[128]={0};
    time(&now);
    memcpy(buf,ctime(&now),strlen(ctime(&now))-1);
    if(op==0){
        sprintf(buf,"%s%s%s%s%s%s",buf," ",user.name," send info:",info,".\n");
    }else if(op==1){
        if(res==1){
            sprintf(buf,"%s%s%s%s",buf," ",user.name," login success.\n");
        }else{
            sprintf(buf,"%s%s%s%s",buf," ",user.name," login failed.\n");
        }
    }else if(op==2){
        if(res==1){
            sprintf(buf,"%s%s%s%s",buf," ",user.name," register success.\n");
        }else{
            sprintf(buf,"%s%s%s%s",buf," ",user.name," register failed.\n");
        }
    }else if(op==3){
        sprintf(buf,"%s%s%s%s",buf," ",user.name," exit.\n");
    }else if(op==4){
        sprintf(buf,"%s%s%s%s",buf," ",user.name," command ls.\n");
    }else if(op==5){
        sprintf(buf,"%s%s%s%s",buf," ",user.name," command pwd.\n");
    }else if(op==6){
        if(res==1){
            sprintf(buf,"%s%s%s%s",buf," ",user.name," command cd ../.\n");
        }else if(res==2){
            sprintf(buf,"%s%s%s%s",buf," ",user.name," command cd ~/.\n");
        }else if(res==3){
            sprintf(buf,"%s%s%s%s%s%s",buf," ",user.name," command cd ",info," success.\n");
        }else if(res==4){
            sprintf(buf,"%s%s%s%s%s%s",buf," ",user.name," command cd ",info," failed.\n");
        }
    }else if(op==7){
        if(res==1){
            sprintf(buf,"%s%s%s%s%s%s",buf," ",user.name," download ",info," success.\n");
        }else{
            sprintf(buf,"%s%s%s%s%s%s",buf," ",user.name," download ",info," failed.\n");
        }
    }else if(op==8){
        if(res==1){//普通上传成功
            sprintf(buf,"%s%s%s%s%s%s",buf," ",user.name," upload ",info," success.\n");
        }else{
            sprintf(buf,"%s%s%s%s%s%s",buf," ",user.name," flash upload ",info," success.\n");
        }
    }else if(op==9){
        if(res==1){
            sprintf(buf,"%s%s%s%s%s%s",buf," ",user.name," remove ",info," success.\n");
        }else{
            sprintf(buf,"%s%s%s%s%s%s",buf," ",user.name," remove ",info," failed.\n");
        }
    }else if(op==10){
        if(res==1){
            sprintf(buf,"%s%s%s%s%s%s",buf," ",user.name," mkdir ",info," success.\n");
        }else{
            sprintf(buf,"%s%s%s%s%s%s",buf," ",user.name," mkdir ",info," failed.\n");
        }
    }
    write(fd,buf,strlen(buf));
    close(fd);
    return 0;
}

long long Mod(long long a, long long  b, long long m){
    long long r = 1;
    for (long long j = 0; j < b; j++){
        r = (r * a) % m;
        
    }
    return r;
}

long long * rsaEncode(char * message){
    size_t length = strlen(message);
    long long *ascii =(long long *)malloc(length*sizeof(long long));  //存储信息的每个字符
    long long *code =(long long *)malloc(length*sizeof(long long));   //存储每个字符的rsa编码
    for (size_t i = 0; i < length; i++){
        ascii[i] = message[i];
    }
 
    for (size_t i = 0; i < length; i++){
        code[i] = Mod(ascii[i], e, N ); //C=M^e (modN)
    }
    
    return code;
}

 void rsaDecode(long long code[], size_t n,char *message){
    long long  *ascii = (long long *)malloc(n*sizeof(long long)); 
    for (size_t i = 0; i < n; i++){
        ascii[i] = Mod(code[i], d, N); //M=C^d (modN)
        message[i] = ((char)(ascii[i]));
    }
    return;
}