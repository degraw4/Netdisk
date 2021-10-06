/*by Underwater*/
#define _GNU_SOURCE  //添加宏
#include "client.h"

void get(int socketFd){
   int len,ret;
   char buf[1024]={0};
   //先接文件名
   reCycle(socketFd,&len,4);
   reCycle(socketFd,buf,len);
   int fd;
   int i=1;
   char tmp[1024];
   strcpy(tmp,buf);
   //创建文件
   while((fd=open(tmp,O_RDWR|O_CREAT|O_EXCL,0666))==-1){
      bzero(tmp,sizeof(tmp));
      sprintf(tmp,"%s%d",buf,i++);
   }
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
      /* double d=(double)downloadSize/size*50;
      int i=(int)d;
      Print(i);*/
   }
   time(&end);
   if(ret==-1||tmpSize==-1){
      printf("\nDownload error\n");
   }else{
      printf("Download %ldMB, time use %lds.\n",size/1048576,end-start);
      if(end>start)
         printf("Average speed %ldMB/s.\n",(size/1048576)/(end-start));
   }
   close(socketFd);
}

int reCycle(int fd,void *p,int len){
   char *buf=(char *)p;
   int total=0,ret;
   while(total<len){
      ret=recv(fd,buf+total,len-total,0);
      if(ret==0){
         printf("Conession close\n");
         return -1;
      }
      total+=ret;
   }
   return 0;
}

void Print(int i){
   char bar[52]={0};
	for(int a=0;a<=50;a++)
	bar[a]='#';
	const char* lable = "|/-\\";
   printf("[");
	for(int j=0;j<=i;j++)
		printf("%c",bar[i]);
	for(int j=50-i;j>0;j--)
		printf(" ");		
   printf("]");	
	printf("[%d%%][%c]\r",2*i,lable[i%4]);
	fflush(stdout); 
}

void download_file(int socketFd,char *name){
   int len,ret;
   char buf[1024]={0};
   //创建文件
   int fd,i=1;
   char tmp[20];
   strcpy(tmp,name);
   while((fd=open(name,O_RDWR|O_CREAT|O_EXCL,0666))==-1){
      bzero(tmp,sizeof(tmp));
      sprintf(tmp,"%s%d",buf,i++);
   }
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
      printf("\nDownload error\n");
   }else{
      printf("Download %ldMB, time use %lds.\n",size/1048576,end-start);
      if(end>start)
         printf("Average speed %ldMB/s.\n",(size/1048576)/(end-start));
   }
   close(socketFd);
}

int redownload_file(int socketFd,char *name,off_t offset){
   int len,ret;
   //打开文件
   int fd=open(name,O_RDWR);//or lseek
   ret=lseek(fd,0,SEEK_END);
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
      printf("\nDownload error\n");
   }else if(size==0){
      printf("\n%s already exist.\n",name);
   }else{
      printf("Download %ldMB, time use %lds.\n",size/1048576,end-start);
      if(end>start)
         printf("Average speed %ldMB/s.\n",(size/1048576)/(end-start));
   }
   close(socketFd);
   return 0;
}

int upload_file(int newFd,char *name){
   data_t data;
   //发送文件大小
   struct stat buf;
   stat(name,&buf);
   data.len=sizeof(buf.st_size);
   memcpy(data.buf,&buf.st_size,sizeof(buf.st_size));
   send(newFd,&data,4+sizeof(buf.st_size),0);
   //发送文件
   int fileFd=open(name,O_RDWR);
   ERROR_CHECK(fileFd,-1,"open");
   int ret=sendfile(newFd,fileFd,NULL,buf.st_size); 
   ERROR_CHECK(ret,-1,"sendfile");
   printf("\nUpload %s success.\n",name);
   printf("\tSend %dMB\n",ret/1048576);
   close(fileFd);
   return 0;
}