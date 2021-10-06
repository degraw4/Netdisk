/*by Underwater*/
#include <termios.h>
#include "client.h"

long long N = 6651937;
long long e = 13007, d = 511;

int tcpInit(int *socketFd,char *ip,char *port){
    int Fd=socket(AF_INET,SOCK_STREAM,0);
    ERROR_CHECK(Fd,-1,"socket");
    struct sockaddr_in ser;
    bzero(&ser,sizeof(ser));
    ser.sin_family=AF_INET;
    ser.sin_port=htons(atoi(port));
    ser.sin_addr.s_addr=inet_addr(ip);
    int ret=connect(Fd,(struct sockaddr*)&ser,sizeof(ser));
    ERROR_CHECK(ret,-1,"connect");
    printf("Connection success\n");
    *socketFd=Fd;
    return 0;
}

int getLogin(){
    char s[20];
    printf(BLUE"Welcome to use NetDisc!\n");
    while(1){
        printf("You want to:\n");
        printf("1.\tLogin in.\n");
        printf("2.\tRegister.\n");
        printf("3.\tExit.\n");
        scanf("%s",s);
        getchar();
        if(strcmp(s,"1")==0||strcmp(s,"2")==0||strcmp(s,"3")==0){
            int i=atoi(s);
            return i;
        }else{
            printf("Input incorrect,please input again.\n");
            bzero(s,sizeof(s));
            fflush(stdin);
        }
    }
}

int getpsw(char *buf){
    struct termios oldflags, newflags;
    int len;
    //设置终端为不回显模式
    tcgetattr(fileno(stdin), &oldflags);
    newflags = oldflags;
    newflags.c_lflag &= ~ECHO;
    newflags.c_lflag |= ECHONL;
    if (tcsetattr(fileno(stdin), TCSANOW, &newflags) != 0){
        perror("tcsetattr");
        return -1;
    }
    //获取来自键盘的输入
    fgets(buf, 20, stdin);
    len = strlen(buf);
    buf[len-1] = 0;
    //恢复原来的终端设置
    if (tcsetattr(fileno(stdin), TCSANOW, &oldflags) != 0){
        perror("tcsetattr");
        return -1;
    }
    return 0;
}

int login(int a,int socketFd,user_t *u){
    data_t data;
    user_t user;
    int error=1;
    char buf[10];
    if(a==1){//登录
        send(socketFd,"1",1,0);
        while(error){
            printf("Please input username:\n");
            scanf("%s",user.name);
            getchar();//接\n
            printf("Please input password:\n");
            getpsw(user.psw);
            //发用户名
            data.len=strlen(user.name);
            strcpy(data.buf,user.name);
            int ret=send(socketFd,&data,4+data.len,0);
            if(ret==-1){
                return -1;
            }
            //发密码
            long long *code=rsaEncode(user.psw);
            size_t len=strlen(user.psw);

            data.len=sizeof(long long)*len;
            memcpy(data.buf,code,data.len);
            ret=send(socketFd,&data,4+data.len,0);
            if(ret==-1){
                return -1;
            }
            //等待验证
            recv(socketFd,buf,sizeof(buf),0);
            error=atoi(buf);
            if(error==0){
                printf("Login success.\n");
                memcpy(u,&user,sizeof(user_t));
                return 0;
            }else if(error==1){//查无此人
                printf("Username doesn't exit,please try again.\n");
            }else if(error==2){//密码错误
                printf("Password error,please try again.\n");
            }
        }
    }else{//注册
        send(socketFd,"2",1,0);
        while(error){
            printf("Please input username:\n");
            scanf("%s",user.name);
            getchar();//接\n
            //发用户名
            data.len=strlen(user.name);
            strcpy(data.buf,user.name);
            int ret=send(socketFd,&data,4+data.len,0);
            if(ret==-1){
                return -1;
            }
            //等待验证
            recv(socketFd,buf,sizeof(buf),0);
            error=atoi(buf);
            if(error==0){
                strcpy((*u).name,user.name);
                break;
            }else{
                printf("This name is already exist,please try another one.\n");
                bzero(&user,sizeof(user_t));
                bzero(buf,sizeof(buf));
            }
        }
        error=1;
        bzero(buf,sizeof(buf));
        while(error){
            printf("Please input password:\n");
            getpsw(user.psw);

            printf("Please input password again:\n");
            char tmp[20];
            getpsw(tmp);
            if(strcmp(tmp,user.psw)==0){
                break;
            }else{
                printf("Two inputs are not same,please try again.\n");
                bzero(tmp,sizeof(tmp));
                bzero(user.psw,sizeof(user.psw));
            }
        }
        //发密码
        long long *code=rsaEncode(user.psw);
        size_t len=strlen(user.psw);
        data.len=sizeof(long long)*len;
        memcpy(data.buf,code,data.len);
        int ret=send(socketFd,&data,4+data.len,0);
        if(ret==-1){
            return -1;
        }
        //等待注册成功
        recv(socketFd,buf,sizeof(buf),0);
        error=atoi(buf);
        if(error==0){
            printf("Register success.\n");
            strcpy((*u).psw,user.psw);
        }else{
            printf("Register failed.\n");
        }
    }
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