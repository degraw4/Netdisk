/*by Underwater*/
#include "thread_pool.h"
int exitFd[2];

void sigFunc(int signum){
    write(exitFd[1],&signum,1);
    printf("\nServer offline\n");
}

int main(int argc,char **argv){
    //ARGS_CHECK(argc,5);
    pipe(exitFd);
    if(fork()){
        log_start();
        close(exitFd[0]);
        signal(SIGUSR1,sigFunc);
        signal(SIGINT,sigFunc);
        wait(NULL);
        log_end();
        exit(0);
    }
    close(exitFd[1]);
    Factory_t fac;
    //int num=atoi(argv[3]);
    //int cap=atoi(argv[4]);
    int num=2,cap=5;
    factoryInit(&fac,num,cap);
    factoryStart(&fac);
    int socketFd;
    //tcpInit(&socketFd,argv[1],argv[2]);
    tcpInit(&socketFd,"192.168.253.128","2000");
    printf("Server online, waiting for connection\n");
    int newFd;
    pQue_t q=&fac.que;
    pNode_t pRequest;
    struct sockaddr_in cli;
    int epfd=epoll_create(1);
    struct epoll_event event,evs[2];
    event.data.fd=exitFd[0];
    event.events=EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,exitFd[0],&event);
    event.data.fd=socketFd;
    epoll_ctl(epfd,EPOLL_CTL_ADD,socketFd,&event);
    int ready;
    while(1){
        ready=epoll_wait(epfd,evs,2,-1);
        for(int i=0;i<ready;i++){
            if(evs[i].data.fd==socketFd){
                socklen_t len=sizeof(cli);
                newFd=accept(socketFd,(struct sockaddr *)&cli,&len);
                ERROR_CHECK(newFd,-1,"accept");
                printf("Accept client IP=%s, port=%d\n",inet_ntoa(cli.sin_addr),ntohs(cli.sin_port));
                char buf[10];//接连接方式 flag=0/1 0为首次连接
                recv(newFd,buf,sizeof(buf),0);
                pRequest=(pNode_t)calloc(1,sizeof(Node_t));
                pRequest->newFd=newFd;
                pRequest->flag=atoi(buf);
                pthread_mutex_lock(&q->mutex);
                quePush(pRequest,q);
                pthread_mutex_unlock(&q->mutex);
                pthread_cond_signal(&fac.cond);
            }
            if(evs[i].data.fd==exitFd[0]){
                close(socketFd);
                for(int j=0;j<fac.threadNum;j++){
                    pthread_cancel(fac.pthid[i]);
                }
                for(int j=0;j<fac.threadNum;j++){
                    pthread_join(fac.pthid[i],NULL);
                }
                exit(0);
            }
        }
    }
    return 0;
}