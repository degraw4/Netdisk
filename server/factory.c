/*by Underwater*/
#include "thread_pool.h"

int factoryInit(pFactory_t p,int threadNum,int cap){
    bzero(p,sizeof(Factory_t));
    queInit(&p->que,cap);
    pthread_cond_init(&p->cond,NULL);
    p->threadNum=threadNum;
    p->pthid=(pthread_t*)calloc(threadNum,sizeof(pthread_t));
    return 0;
}

void clean(void *p){
    pthread_mutex_unlock(&((pQue_t)p)->mutex);
}

void *threadFunc(void *p){
    pFactory_t fac=(pFactory_t)p;
    pQue_t q=&fac->que;
    pNode_t pTask;
    pthread_cleanup_push(clean,q);
    user_t user;
    bzero(&user,sizeof(user));
    int ret;
    while(1){
        pthread_mutex_lock(&q->mutex);
        if(!q->queSize){
            pthread_cond_wait(&fac->cond,&q->mutex);
        }
        queGet(q,&pTask);
        pthread_mutex_unlock(&q->mutex);
        int fd=pTask->newFd;
        int type=pTask->flag;
        user_t user;
        bzero(&user,sizeof(user));
        printf("Thread get task\n");
        if(type==0){//第一次连接
            ret=getuser(fd,&user);
            if(ret==-1){
                printf("\tClient is offline.\n");
                continue;
            }
            char buf[30]={0};
            while(1){//接指令
                bzero(buf,sizeof(buf));
                ret=recv(fd,buf,sizeof(buf),0);
                log_op(user,0,0,buf);
                int cmd=atoi(buf);
                if(cmd==0){
                    printf("User %s is offline.\n",user.name);
                    log_op(user,3,0,NULL);
                    break;
                }else if(cmd==1){//ls
                    log_op(user,4,0,NULL);
                    char dir[100]={0};
                    char file[100]={0};
                    ls_query(user.path[user.pathlen-1],user,dir,file);
                    if(strlen(dir)==0){
                        strcpy(dir," ");
                    }
                    if(strlen(file)==0){
                        strcpy(file," ");
                    }
                    data_t data;
                    //send dir
                    data.len=strlen(dir);
                    strcpy(data.buf,dir);
                    int ret=send(fd,&data,4+data.len,0);
                    if(ret==-1){
                        break;
                    }
                    //send file
                    data.len=strlen(file);
                    strcpy(data.buf,file);
                    ret=send(fd,&data,4+data.len,0);
                    if(ret==-1){
                        break;
                    }

                    /* log_op(user,4,0,NULL);
                    char ls[100]={0};=
                    ls_query(user.path[user.pathlen-1],user,ls);
                    if(strlen(ls)==0){
                        strcpy(ls," ");
                    }

                    ret=send(fd,ls,strlen(ls),0);
                    if(ret==-1){
                        break;
                    } */
                }else if(cmd==2){//pwd 由于client自动显示路径 故每次有命令都会记录日志
                    log_op(user,5,0,NULL);
                    char pwd[30]="/";
                    strcat(pwd,user.name);
                    pwd_query(user,pwd);
                    ret=send(fd,pwd,strlen(pwd),0);
                    if(ret==-1){
                        break;
                    }
                }else if(cmd==3){//cd
                    ret=send(fd,"1",1,0);//缓冲
                    if(ret==-1){
                        break;
                    }
                    bzero(buf,sizeof(buf));
                    ret=recv(fd,buf,sizeof(buf),0);
                    log_op(user,0,0,buf);
                    if(ret==-1){
                        break;
                    }
                    int type=atoi(buf);
                    if(type==31){//..
                        log_op(user,6,1,NULL);
                        if(user.pathlen==1){
                            continue;
                        }else{
                            user.path[user.pathlen--]=0;
                        }
                    }if(type==32){//~
                        //教训 此处不能直接让len=-1，因为若get传path会导致len计算错误
                        log_op(user,6,2,NULL);
                        bzero(user.path,sizeof(user.path));
                        user.path[0]=-1;
                        user.pathlen=1;
                    }if(type==33){// dir
                        ret=send(fd,"1",1,0);//缓冲
                        if(ret==-1){
                            break;
                        }
                        char dir[20]={0};
                        ret=recv(fd,dir,sizeof(dir),0);
                        if(ret==-1){
                            break;
                        }
                        int k;
                        ret=cd_query(&k,&user,dir);
                        if(ret){//成功
                            log_op(user,6,3,dir);
                            send(fd,"1",1,0);
                            user.path[user.pathlen]=k;
                            user.pathlen++;
                        }else{//失败
                            log_op(user,6,4,dir);
                            send(fd,"0",1,0);
                        }
                    }
                }else if(cmd==4){
                    //发送path
                    char path[80]={0};
                    memcpy(path,&user.path,sizeof(user.path));
                    ret=send(fd,path,sizeof(path),0);
                    if(ret==-1){
                        break;
                    }
                }else if(cmd==5){
                    //发送path
                    char path[80]={0};
                    memcpy(path,&user.path,sizeof(user.path));
                    ret=send(fd,path,sizeof(path),0);
                    if(ret==-1){
                        break;
                    }
                }else if(cmd==6){//rm  此处涉及到ID和fileID的问题 删除较为复杂 故只删除sql数据
                    char file[20]={0};
                    ret=send(fd,"1",1,0);//缓冲
                    if(ret==-1){
                        break;
                    }
                    ret=recv(fd,file,sizeof(file),0);
                    if(ret==-1){
                        break;
                    }
                    int id=del_exist_query(user.path[user.pathlen-1],user,file);//得到ID
                    if(id==0){//不存在 send 0
                        log_op(user,9,0,file);
                        ret=send(fd,"0",1,0);
                            if(ret==-1){
                                break;
                            }
                    }else{//存在 成功发1 失败发2
                        /* char name[20]={0};
                        sprintf(name,"%s%d","./file/",id);
                        int res1=unlink(name);//删除file */
                        int res2=sql_delete(id);//删除sql
                        if(!res2){
                            log_op(user,9,1,file);
                            ret=send(fd,"1",1,0);
                            if(ret==-1){
                                break;
                            }
                        }else{
                            log_op(user,9,0,file);
                            ret=send(fd,"2",1,0);
                            if(ret==-1){
                                break;
                            }
                        }
                    }
                }else{//mkdir
                    char file[20]={0};
                    ret=send(fd,"1",1,0);//缓冲
                    if(ret==-1){
                        break;
                    }
                    //接dir
                    ret=recv(fd,file,sizeof(file),0);
                    if(ret==-1){
                        break;
                    }
                    int tmp=user.path[user.pathlen-1];
                    if(tmp==-1)
                        tmp=0;
                    ret=mkdir_insert(tmp,user,file);
                    if(ret==0){//成功
                        ret=send(fd,"0",1,0);
                        log_op(user,10,1,file);
                        if(ret==-1){
                            break;
                        }
                    }else{//失败
                        ret=send(fd,"1",1,0);
                        log_op(user,10,0,file);
                        if(ret==-1){
                            break;
                        }
                    }
                }
            }
        }else if(type==1){//下载
            //仍旧记录用户名和path 但为自动传输 不需再次输入 用于记录日志
            int ret=send(fd,"1",1,0);//缓冲
            if(ret==-1){
                goto end;
            }
            int len=0;
            char buf[80]={0};
            char file[20]={0}; //filename
            //username
            reCycle(fd,&len,4);
            reCycle(fd,buf,len);
            strcpy(user.name,buf);
            //path
            bzero(buf,sizeof(buf));
            bzero(&user.path,sizeof(user.path));
            reCycle(fd,&len,4);
            reCycle(fd,buf,len);
            memcpy(&user.path,buf,sizeof(buf));
            //filename
            bzero(buf,sizeof(buf));
            reCycle(fd,&len,4);
            reCycle(fd,buf,len);
            strcpy(file,buf);

            int i=0;
            while(user.path[i]!=0){
                i++;
            }
            user.pathlen=i;
            //验证文件是否存在 id得到文件的fileID 也是文件本地的名字
            int id=exist_query(user.path[user.pathlen-1],user,file);
            if(id){//文件存在
                ret=send(fd,"1",1,0);
                if(ret==-1){
                    goto end;
                }
                char buf[20]={0};
                recv(fd,buf,sizeof(buf),0);
                log_op(user,0,0,buf);
                long size=atoi(buf);
                if(size==-1){//下载
                ret=download_file(fd,id);
                if(!ret){
                    log_op(user,7,1,file);
                }else{
                    log_op(user,7,0,file);
                }
                }else{//断点续传 收到大小
                    ret=redownload_file(fd,id,size);
                    if(!ret){
                        log_op(user,7,1,file);
                    }else{
                        log_op(user,7,0,file);
                    }
                }
            }else{//文件不存在
                log_op(user,7,0,file);
                ret=send(fd,"0",1,0);
                if(ret==-1){
                    goto end;
                }
            }
        }else{//上传
            int ret=send(fd,"1",1,0);//缓冲
            if(ret==-1){
                goto end;
            }
            int len=0;
            char buf[80]={0};
            char file[20]={0}; //filename
            char md5[50]={0};
            //username
            reCycle(fd,&len,4);
            reCycle(fd,buf,len);
            strcpy(user.name,buf);
            //path
            bzero(buf,sizeof(buf));
            bzero(&user.path,sizeof(user.path));
            reCycle(fd,&len,4);
            reCycle(fd,buf,len);
            memcpy(&user.path,buf,sizeof(buf));
            //filename
            bzero(buf,sizeof(buf));
            reCycle(fd,&len,4);
            reCycle(fd,buf,len);
            strcpy(file,buf);
            //md5
            bzero(buf,sizeof(buf));
            reCycle(fd,&len,4);
            reCycle(fd,buf,len);
            strcpy(md5,buf);
            int i=0;
            while(user.path[i]!=0){
                i++;
            }
            user.pathlen=i;
            //查询数据库
            int ans=md5_query(md5);
            if(ans==-1){//错误发1
                send(fd,"1",1,0);
            }else {
                int flag=1;
                i=1;
                char buf[20]={0};
                strcpy(buf,file);
                while(flag){//防止上传的文件夹下有重名
                    flag=name_query(user.path[user.pathlen-1],user,buf);
                    if(flag==2){
                        send(fd,"1",1,0);
                        break;
                    }else if(flag==1){
                        bzero(buf,sizeof(buf));   
                        sprintf(buf,"%s%d",file,i++);
                    }
                }
                if(ans==0){//无md5
                    ret=send(fd,"2",1,0);//缓冲
                    if(ret==-1)
                        goto end;
                    //准备接受文件+插入数据库
                    int tmp=user.path[user.pathlen-1];
                    if(tmp==-1)
                        tmp=0;
                    int fileID=sql_input(tmp,user,buf,md5,ans,0);
                    id_update(fileID);
                    char name[30]={0};
                    sprintf(name,"%s%d","./file/",fileID);
                    long size=upload_file(fd,name);
                    //size
                    size_update(fileID,size);
                    log_op(user,8,1,buf);
                }else if(ans>0){//秒传 ans为fileID
                    ret=send(fd,"0",1,0); //发送上传成功信号
                    long size;
                    struct stat fileSt;
                    //将大小插入数据库
                    char name[30]={0};
                    sprintf(name,"%s%d","./file/",ans);
                    stat(name,&fileSt);
                    size=fileSt.st_size;
                    if(ret==-1)
                        goto end;
                    //准备插入数据库
                    int tmp=user.path[user.pathlen-1];
                    if(tmp==-1)
                        tmp=0;
                    sql_input(tmp,user,buf,md5,ans,size);
                    log_op(user,8,2,buf);
                }
            }
        }   
        end:
        close(fd);
        free(pTask);
        printf("Thread finish task\n");
    }
    pthread_cleanup_pop(1);
}

int factoryStart(pFactory_t fac){
    if(!fac->startFlag){
        for(int i=0;i<fac->threadNum;i++){
            pthread_create(fac->pthid+i,NULL,threadFunc,fac);
        }
        fac->startFlag=1;
    }
    return 0;
}