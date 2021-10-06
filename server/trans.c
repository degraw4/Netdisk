/*by Underwater*/
#define _GNU_SOURCE  //添加宏
#include "thread_pool.h"
#define NAME "FILE" 

int put(int newFd){//以前的函数
   data_t data;
   //发送文件名
   data.len=strlen(NAME);
   strcpy(data.buf,NAME);
   send(newFd,&data,4+data.len,0);
   //发送文件大小
   struct stat buf;
   stat(NAME,&buf);
   data.len=sizeof(buf.st_size);
   memcpy(data.buf,&buf.st_size,sizeof(buf.st_size));
   send(newFd,&data,4+sizeof(buf.st_size),0);
   //发送文件
   int fd=open(NAME,O_RDWR);
   ERROR_CHECK(fd,-1,"open");
   int ret=sendfile(newFd,fd,NULL,buf.st_size); 
   printf("\tSend file %dMB\n",ret/1048576);
   ERROR_CHECK(ret,-1,"sendfile");
   return 0;
}


int reCycle(int fd,void *p,int len){
   char *buf=(char *)p;
   int total=0,ret;
   while(total<len){
      ret=recv(fd,buf+total,len-total,0);
      if(ret==0){
         printf("\tConession close\n");
         return -1;
      }
      total+=ret;
   }
   return 0;
}

int download_file(int newFd,int id){
   data_t data;
   //发送文件大小
   struct stat buf;
   char name[30]={0};
   sprintf(name,"%s%d","./file/",id);
   stat(name,&buf);
   data.len=sizeof(buf.st_size);
   memcpy(data.buf,&buf.st_size,sizeof(buf.st_size));
   send(newFd,&data,4+sizeof(buf.st_size),0);
   //发送文件
   int fd=open(name,O_RDWR);
   ERROR_CHECK(fd,-1,"open");
   int ret=sendfile(newFd,fd,NULL,buf.st_size); 
   printf("\tSend %dMB\n",ret/1048576);
   ERROR_CHECK(ret,-1,"sendfile");
   return 0;
}

int redownload_file(int newFd,int id,off_t offset){
   data_t data;
   //发送文件大小
   struct stat buf;
   char name[30]={0};
   sprintf(name,"%s%d","./file/",id);
   stat(name,&buf);
   off_t remain=buf.st_size-offset;
   data.len=sizeof(remain);
   memcpy(data.buf,&remain,data.len);
   send(newFd,&data,4+data.len,0);
   //发送文件
   int fd=open(name,O_RDWR);
   ERROR_CHECK(fd,-1,"open");
   int ret=lseek(fd,offset,SEEK_SET);
   ret=sendfile(newFd,fd,NULL,remain); 
   printf("\tSend %dMB\n",ret/1048576);
   ERROR_CHECK(ret,-1,"sendfile");
   return 0;
}

int upload_file(int socketFd,char *name){//接收 newFd,文件名 name 返回大小
   int len,ret;
   //创建文件
   int fd=open(name,O_RDWR|O_CREAT,0666);
   //接收文件大小
   off_t size,total=0,downloadSize=0,tmpSize;
   reCycle(socketFd,&len,4);
   reCycle(socketFd,&size,len);
   //接收文件
   time_t start,end;
   int fds[2];
   pipe(fds);
   time(&start);
   while(downloadSize<size){//splice
      ret=splice(socketFd,NULL,fds[1],NULL,size-total,/*SPLICE_F_MOVE*/ 5);
      total+=ret; 
      tmpSize=splice(fds[0],NULL,fd,NULL,ret,/*SPLICE_F_MOVE*/ 5);
      downloadSize+=tmpSize;
   }
   time(&end);
   if(ret==-1||tmpSize==-1){
      printf("\nUpload error\n");
   }else{
      printf("\nUpload %s success.\n",name);
      printf("Receive %ldMB, Time use %lds.\n",size/1048576,end-start);
   }
   return size;
   close(socketFd);
}