/*by Underwater*/
#include "client.h"
int pthStat1=0,pthStat2=0;

void *th_download(void *p){
    pthStat1=1;
    info_t *info=(info_t*)p;
    strcpy(info->ip,"192.168.253.128");
    strcpy(info->port,"2000");
    int fd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in ser;
    bzero(&ser,sizeof(ser));
    ser.sin_family=AF_INET;
    ser.sin_port=htons(atoi(info->port));
    ser.sin_addr.s_addr=inet_addr(info->ip);
    connect(fd,(struct sockaddr*)&ser,sizeof(ser));
    send(fd,"1",1,0); //非首次连接 下载发1 
    char buf[80]={0};
    recv(fd,buf,sizeof(buf),0);
    data_t data;
    //发用户名
    data.len=strlen(info->user.name);
    strcpy(data.buf,info->user.name);
    send(fd,&data,4+data.len,0);
    //path
    bzero(buf,sizeof(buf));
    data.len=sizeof(info->user.path);
    memcpy(data.buf,&info->user.path,sizeof(info->user.path));
    send(fd,&data,4+data.len,0);
    //filename
    bzero(buf,sizeof(buf));
    data.len=strlen(info->file);
    strcpy(data.buf,info->file);
    send(fd,&data,4+data.len,0);
    //等待验证文件是否存在
    bzero(buf,sizeof(buf));
    recv(fd,buf,sizeof(buf),0);
    if(atoi(buf)==0){
        printf("No such file or that maybe a dir.\n");
    }else{//选择断点下载or下载
        //printf("Downloading...\n");
        //get(fd);
        int newfd=open(info->file,O_RDWR);
        if(newfd==-1){//下载 发-1
            send(fd,"-1",2,0);
            download_file(fd,info->file);
        }else{//断点续传 发size  用lseek？
            close(newfd);
            struct stat fstat; 
            stat(info->file,&fstat); 
            sprintf(buf,"%ld",fstat.st_size);
            send(fd,buf,strlen(buf),0);//发size 
            redownload_file(fd,info->file,fstat.st_size);
        }
    }
    pthStat1=1;
    return NULL;
}

void *th_upload(void *p){
    pthStat2=0;
    info_t *info=(info_t*)p;
    strcpy(info->ip,"192.168.253.128");
    strcpy(info->port,"2000");
    int fd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in ser;
    bzero(&ser,sizeof(ser));
    ser.sin_family=AF_INET;
    ser.sin_port=htons(atoi(info->port));
    ser.sin_addr.s_addr=inet_addr(info->ip);
    connect(fd,(struct sockaddr*)&ser,sizeof(ser));
    send(fd,"2",1,0); //非首次连接 上传发2 
    char buf[80]={0};
    recv(fd,buf,sizeof(buf),0);//buffer
    data_t data;
    //发用户名
    data.len=strlen(info->user.name);
    strcpy(data.buf,info->user.name);
    send(fd,&data,4+data.len,0);
    //path
    bzero(buf,sizeof(buf));
    data.len=sizeof(info->user.path);
    memcpy(data.buf,&info->user.path,sizeof(info->user.path));
    send(fd,&data,4+data.len,0);
    //filename
    bzero(buf,sizeof(buf));
    data.len=strlen(info->file);
    strcpy(data.buf,info->file);
    send(fd,&data,4+data.len,0);
    //md5
    bzero(buf,sizeof(buf));
    int fileFd=open(info->file,O_RDWR);
    compute_file_md5(fileFd,buf); 
    data.len=strlen(buf);
    strcpy(data.buf,buf);
    send(fd,&data,4+data.len,0);
    bzero(buf,sizeof(buf));
    recv(fd,buf,sizeof(buf),0);//md5情况
    int ans=atoi(buf);
    close(fileFd);
    if(ans==1){
        printf("Upload error,please try again.\n");
    }else if(ans==0){
        printf("Flash upload success.\n");
    }else{//准备上传文件
        upload_file(fd,info->file);
    }
    pthStat2=1;
    return NULL;
}

int main(int argc,char **argv){
    //ARGS_CHECK(argc,3);
    int socketFd=0;
    //tcpInit(&socketFd,argv[1],argv[2]);
    tcpInit(&socketFd,"192.168.253.128","2000");
    send(socketFd,"0",1,0);//首次连接
    user_t user;
    //初始化
    user.pathlen=0;
    bzero(&user.path,sizeof(user.path));
    pthread_t pthid1,pthid2;
    int ret=getLogin();
    if(!ret){
        printf("Client offline\n");
        return 0;
    }
    ret=login(ret,socketFd,&user);//注册or登录
    if(ret==-1){
        printf("Server is offline,please try later.\n");
    }
    char input[64]={0};//记录用户输入
    while(1){
        start:
        {//显示路径
            send(socketFd,"2",1,0);
            char path[30]={0};
            ret=recv(socketFd,path,sizeof(path),0);
            if(ret==-1){
                break;
            }
            fflush(stdout);
            printf(LIGHT_GREEN"%s@:%s$ "NONE,user.name,path);
        }
        fgets(input,sizeof(input),stdin);
        input[strlen(input)-1]=0;
        if(strcmp(input,"exit")==0){
            send(socketFd,"0",1,0);//退出发0
            //设置线程清理函数
            if(pthStat1)
                pthread_join(pthid1,NULL);
            if(pthStat2)
                pthread_join(pthid2,NULL);
            break;
        }else if(strcmp(input,"ls")==0){//ls发1
            send(socketFd,"1",1,0);
            char dir[100]={0};
            char file[100]={0};
            int len,s;
            //recv dir
            s=reCycle(socketFd,&len,4);
            if(s==-1){
                    return -1;
            }
            s=reCycle(socketFd,dir,len);
            if(s==-1){
                    return -1;
            }
            //recv file
            s=reCycle(socketFd,&len,4);
            if(s==-1){
                    return -1;
            }
            s=reCycle(socketFd,file,len);
            if(s==-1){
                    return -1;
            }
            if(strcmp(file," ")==0&&strcmp(dir," ")==0){
                printf("There is nothing.\n");
            }else{
                printf(BLUE"%s"NONE,dir);
                printf("%s\n",file);
            }

            /* send(socketFd,"1",1,0);
            char ls[100]={0};

            ret=recv(socketFd,ls,sizeof(ls),0);
            if(ret==-1){
                break;
            }
            if(strcmp(ls," ")==0){
                printf("There is nothing.\n");
            }else{
                printf("%s\n",ls);
            } */
        }else if(strcmp(input,"pwd")==0){//pwd发2
            send(socketFd,"2",1,0);
            char pwd[30]={0};
            ret=recv(socketFd,pwd,sizeof(pwd),0);
            if(ret==-1){
                break;
            }
            printf("%s\n",pwd);
        }else{
            if(input[0]=='c'&&input[1]=='d'&&input[2]==' '){//cd 发3
                int i=2;
                while(input[i]==' '){
                    i++;
                }
                char dir[20]={0};
                strcpy(dir,input+i);
                i=strlen(dir)-1;
                while(dir[i]==' '){
                    dir[i]=0;
                    i--;
                }
                for(i=0;i<strlen(dir);i++){
                    if(dir[i]==' '){
                        printf("Too many argvments.\n");
                        goto start;
                    }
                }
                if(strcmp(dir,".")==0){
                    goto start;
                }
                send(socketFd,"3",1,0);//发送信号
                char buf[10];
                recv(socketFd,buf,sizeof(buf),0);
                if(strcmp(dir,"..")==0){
                    send(socketFd,"31",2,0);
                }else if(strcmp(dir,"~")==0){
                    send(socketFd,"32",2,0);
                }else{//cd dir
                    send(socketFd,"33",2,0);
                    recv(socketFd,buf,sizeof(buf),0);
                    send(socketFd,dir,sizeof(dir),0);
                    //等结果
                    bzero(buf,sizeof(buf));
                    recv(socketFd,buf,sizeof(buf),0);
                    ret=atoi(buf);
                    if(ret==0){
                        printf("No such dir.\n");
                    }
                }
            }else if(input[0]=='g'&&input[1]=='e'&&input[2]=='t'&&input[3]==' '){//get 发4
                int i=3;
                while(input[i]==' '){
                    i++;
                }
                char file[20]={0};
                strcpy(file,input+i);
                i=strlen(file)-1;
                while(file[i]==' '){
                    file[i]=0;
                    i--;
                }
                for(i=0;i<strlen(file);i++){
                    if(file[i]==' '){
                        printf("Too many argvments.\n");
                        goto start;
                    }
                }
                info_t info;
                strcpy(info.file,file);
                send(socketFd,"4",1,0);
                //要得到下载时的路径
                char buf[80]={0};
                recv(socketFd,buf,sizeof(buf),0);
                memcpy(&user.path,buf,sizeof(buf));
                i=0;
                while(user.path[i]!=0){
                    i++;
                }
                user.pathlen=i;
                info.user=user;
                pthread_create(&pthid1,NULL,th_download,(void*)&info);

            }else if(input[0]=='p'&&input[1]=='u'&&input[2]=='t'&&input[3]==' '){//put 发5
                int i=3;
                while(input[i]==' '){
                    i++;
                }
                char file[20]={0};
                strcpy(file,input+i);
                i=strlen(file)-1;
                while(file[i]==' '){
                    file[i]=0;
                    i--;
                }
                for(i=0;i<strlen(file);i++){
                    if(file[i]==' '){
                        printf("Too many argvments.\n");
                        goto start;
                    }
                }
                //先看本地有没有
                int tmpFd=open(file,O_RDONLY);
                if(tmpFd==-1){
                    printf("No such file.\n");
                }else{
                    close(tmpFd);
                    info_t info;
                    strcpy(info.file,file);
                    send(socketFd,"5",1,0);
                    //要得到下载时的路径
                    char buf[80]={0};
                    recv(socketFd,buf,sizeof(buf),0);
                    memcpy(&user.path,buf,sizeof(buf));
                    i=0;
                    while(user.path[i]!=0){
                        i++;
                    }
                    user.pathlen=i;
                    info.user=user;
                    pthread_create(&pthid2,NULL,th_upload,(void*)&info);
                }
            }else if(input[0]=='r'&&input[1]=='m'&&input[2]==' '){//rm 发6
                int i=2;
                while(input[i]==' '){
                    i++;
                }
                char file[20]={0};
                strcpy(file,input+i);
                i=strlen(file)-1;
                while(file[i]==' '){
                    file[i]=0;
                    i--;
                }
                for(i=0;i<strlen(file);i++){
                    if(file[i]==' '){
                        printf("Too many argvments.\n");
                        goto start;
                    }
                }
                send(socketFd,"6",1,0);
                char buf[10]={0};
                recv(socketFd,buf,sizeof(buf),0);//缓冲
                send(socketFd,file,strlen(file),0);//发文件名
                bzero(buf,sizeof(buf));
                recv(socketFd,buf,sizeof(buf),0);//结果
                i=atoi(buf);
                if(i==1){
                    printf("Remove %s success.\n",file);
                }else if(i==0){
                    printf("No such file.\n");
                }else{
                    printf("Remove %s failed.\n",file);
                }
            }else if(input[0]=='m'&&input[1]=='k'&&input[2]=='d'&&input[3]=='i'&&input[4]=='r'&&input[5]==' '){//mkdir 发7
                //mkdir
                int i=5;
                while(input[i]==' '){
                    i++;
                }
                char file[20]={0};
                strcpy(file,input+i);
                i=strlen(file)-1;
                while(file[i]==' '){
                    file[i]=0;
                    i--;
                }
                for(i=0;i<strlen(file);i++){
                    if(file[i]==' '){
                        printf("Too many argvments.\n");
                        goto start;
                    }
                }
                send(socketFd,"7",1,0);
                char buf[10]={0};
                recv(socketFd,buf,sizeof(buf),0);//缓冲
                send(socketFd,file,strlen(file),0);//发dir名
                bzero(buf,sizeof(buf));

                recv(socketFd,buf,sizeof(buf),0);//结果
                i=atoi(buf);
                if(i==0){
                    printf("Mkdir %s success.\n",file);
                }else{
                    printf("Mkdir %s failed.\n",file);
                }
            }else{
                printf("Wrong command.\n");
            }
        }
    }
    //get(socketFd); 下载文件
    close(socketFd);
    printf("NetDisc offline\n");
    return 0;
}
