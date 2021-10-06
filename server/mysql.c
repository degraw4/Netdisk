/*by Underwater*/
#include "thread_pool.h"
#include <mysql/mysql.h>

MYSQL *conn;   
MYSQL_RES *res;
MYSQL_ROW row;
unsigned int t;
const char* server="localhost";
const char* user="root";
const char* password="root";
const char* database="netdisc";//要访问的数据库名称

int sql_connect(){
	conn=mysql_init(NULL);
	if(!mysql_real_connect(conn,server,user,password,database,0,NULL,0))
	{
		printf("Error connecting to database:%s\n",mysql_error(conn));
		return -1;
	}else{
        return 0;
	}
}

int sql_close(){
    mysql_close(conn);
    return 0;
}

int login_query(user_t user){  
    sql_connect();
    char query[300]="select * from user where name='";
    char name[20];
    strcpy(name,user.name);
	sprintf(query,"%s%s%s",query, name,"'");
    t=mysql_query(conn,query);
	if(t)
	{
		printf("Error making query:%s\n",mysql_error(conn));
		mysql_close(conn);
        return 3;//3 查询cmd错误
	}else{
		res=mysql_use_result(conn);
		if(res)
		{
			if((row=mysql_fetch_row(res))!=NULL)
			{	//密码存在第三列
                if(strcmp(row[3],user.psw)==0){
                    printf("User %s is online.\n",user.name);
					mysql_close(conn);
                    return 0; //正确 0
                }else{//密码错误 2
					mysql_close(conn);
                    return 2;
                }
			}else{//查无此人 1
				mysql_close(conn);
                return 1;
		    }
		}
		mysql_free_result(res);
	}
    mysql_close(conn);
	return 0;
}

int reg_query(user_t user){//注册时用户名查重
    sql_connect();
    char query[300]="select * from user where name='";
    char name[20];
    strcpy(name,user.name);
	sprintf(query,"%s%s%s",query, name,"'");
    t=mysql_query(conn,query);
	if(t)
	{
		printf("Error making query:%s\n",mysql_error(conn));
		mysql_close(conn);
        return 3;//3 查询cmd错误
	}else{
		res=mysql_use_result(conn);
		if(res)
		{
			if((row=mysql_fetch_row(res))!=NULL){//密码存在第三列
				mysql_close(conn);
                return 1;
			}else{//查无此人 1
				mysql_close(conn);
                return 0;
		    }
		}
		mysql_free_result(res);
	}
    mysql_close(conn);
	return 0;
}

int reg_insert(user_t user){
    sql_connect();
    char query[300]="insert into user(name,psw) values('";
	int t;
    sprintf(query,"%s%s%s%s%s",query, user.name,"','",user.psw,"')");
	t=mysql_query(conn,query);
	if(t)
	{
		printf("Error making query:%s\n",mysql_error(conn));
		mysql_close(conn);
        return -1;
	}
	mysql_close(conn);
	return 0;
}

int pwd_query(user_t user,char *buf){//
    for(int i=1;i<user.pathlen;i++){
		path_query(user.path[i],user,buf);
	}
	return 0;
}

int path_query(int id,user_t user,char *buf){//文件ID->文件名
	sql_connect();
    char query[300]="select * from file where ID=";
	sprintf(query,"%s%d",query,id);
    t=mysql_query(conn,query);
	if(t)
	{
		printf("Error making query:%s\n",mysql_error(conn));
		mysql_close(conn);
        return 2;//2 查询cmd错误
	}else{
		res=mysql_use_result(conn);
		if(res)
		{
			if((row=mysql_fetch_row(res))!=NULL){//文件名在第二列
				sprintf(buf,"%s%s%s",buf,"/",row[2]);
				mysql_close(conn);
                return 0;
			}else{//查无此人 1
				mysql_close(conn);
                return 1;
		    }
		}
		mysql_free_result(res);
	}
    mysql_close(conn);
	return 0;
}

int ls_query(int id,user_t user,char *dir,char *file){
	sql_connect();
	if(id==-1){//为家目录
		char query[300]="select * from file where precode=0 and belong='";
		sprintf(query,"%s%s%s",query,user.name,"'");
		t=mysql_query(conn,query);
		if(t)
		{
			printf("Error making query:%s\n",mysql_error(conn));
			mysql_close(conn);
			return 2;//2 查询cmd错误
		}else{
			res=mysql_use_result(conn);
			if(res)
			{
				while((row=mysql_fetch_row(res))!=NULL)//不断获取结果集的下一行
				{	//文件名在第二列
					if(strcmp(row[6],"1")==0){//dir
						sprintf(dir,"%s%s%s",dir,row[2],"    ");
					}else{
						sprintf(file,"%s%s%s",file,row[2],"    ");
					}
				}	
			}
			mysql_free_result(res);
		}	
		mysql_close(conn);
		return 0;
	}else{
		char query[300]="select * from file where precode=";
		sprintf(query,"%s%d",query,id);
		t=mysql_query(conn,query);
		if(t)
		{
			printf("Error making query:%s\n",mysql_error(conn));
			mysql_close(conn);
			return 2;//2 查询cmd错误
		}else{
			res=mysql_use_result(conn);
			if(res)
			{
				while((row=mysql_fetch_row(res))!=NULL)//不断获取结果集的下一行
				{	//文件名在第二列
					if(strcmp(row[6],"1")==0){//dir
						sprintf(dir,"%s%s%s",dir,row[2],"    ");
					}else{
						sprintf(file,"%s%s%s",file,row[2],"    ");
					}
				}	
			}
			mysql_free_result(res);
		}
		mysql_close(conn);
		return 0;
	}
}

int cd_query(int *k,user_t *user,char *dir){
	int id=(*user).path[(*user).pathlen-1];
	if(id==-1){
		id=0;
	}
	sql_connect();
	char query[300]="select * from file where precode=";
	sprintf(query,"%s%d%s%s%s",query,id," and belong='",(*user).name,"' and type=1");
	t=mysql_query(conn,query);
	int flag=0;
	if(t)
	{
		printf("Error making query:%s\n",mysql_error(conn));
		mysql_close(conn);
		return 2;//2 查询cmd错误
	}else{
		res=mysql_use_result(conn);
		if(res)
		{
			while((row=mysql_fetch_row(res))!=NULL)//不断获取结果集的下一行
			{	//文件名在第二列
				if(strcmp(dir,row[2])==0){
					*k=atoi(row[0]);
					flag=1;
					break;
				}
			}	
		}
		mysql_free_result(res);
	}
	mysql_close(conn);
	return flag;//0未找到 1找到
}

int exist_query(int id,user_t user,char *ls){//存在返回fileID 无返回0
	sql_connect();
	if(id==-1){//为家目录
		char query[300]="select * from file where precode=0 and belong='";
		sprintf(query,"%s%s%s",query,user.name,"' and type=0");
		t=mysql_query(conn,query);
		if(t)
		{
			printf("Error making query:%s\n",mysql_error(conn));
			mysql_close(conn);
			return 2;//2 查询cmd错误
		}else{
			res=mysql_use_result(conn);
			if(res)
			{
				while((row=mysql_fetch_row(res))!=NULL)//不断获取结果集的下一行
				{	//文件名在第二列
					if(strcmp(ls,row[2])==0){
						int tmp=atoi(row[7]);
						mysql_close(conn);
						return tmp;//返回fileID
					}
				}	
			}
			mysql_free_result(res);
		}	
		mysql_close(conn);
		return 0;
	}
	else{
		char query[300]="select * from file where type=0 and precode=";
		sprintf(query,"%s%d",query,id);
		t=mysql_query(conn,query);
		if(t)
		{
			printf("Error making query:%s\n",mysql_error(conn));
			mysql_close(conn);
			return 2;//2 查询cmd错误
		}else{
			res=mysql_use_result(conn);
			if(res)
			{
				while((row=mysql_fetch_row(res))!=NULL)//不断获取结果集的下一行
				{	//文件名在第二列
					if(strcmp(ls,row[2])==0){
						int tmp=atoi(row[7]);
						mysql_close(conn);
						return tmp;
					}
				}	
			}
			mysql_free_result(res);
		}
		mysql_close(conn);
		return 0;
	}
}

int del_exist_query(int id,user_t user,char *ls){//存在返回ID 无返回0
	sql_connect();
	if(id==-1){//为家目录
		char query[300]="select * from file where precode=0 and belong='";
		sprintf(query,"%s%s%s",query,user.name,"' and type=0");
		t=mysql_query(conn,query);
		if(t)
		{
			printf("Error making query:%s\n",mysql_error(conn));
			mysql_close(conn);
			return 2;//2 查询cmd错误
		}else{
			res=mysql_use_result(conn);
			if(res)
			{
				while((row=mysql_fetch_row(res))!=NULL)//不断获取结果集的下一行
				{	//文件名在第二列
					if(strcmp(ls,row[2])==0){
						int tmp=atoi(row[0]);
						mysql_close(conn);
						return tmp;//返回fileID
					}
				}	
			}
			mysql_free_result(res);
		}	
		mysql_close(conn);
		return 0;
	}
	else{
		char query[300]="select * from file where type=0 and precode=";
		sprintf(query,"%s%d",query,id);
		t=mysql_query(conn,query);
		if(t)
		{
			printf("Error making query:%s\n",mysql_error(conn));
			mysql_close(conn);
			return 2;//2 查询cmd错误
		}else{
			res=mysql_use_result(conn);
			if(res)
			{
				while((row=mysql_fetch_row(res))!=NULL)//不断获取结果集的下一行
				{	//文件名在第二列
					if(strcmp(ls,row[2])==0){
						int tmp=atoi(row[0]);
						mysql_close(conn);
						return tmp;
					}
				}	
			}
			mysql_free_result(res);
		}
		mysql_close(conn);
		return 0;
	}
}

int sql_delete(int id){
	sql_connect();
	char query[300]="delete from file where ID=";
	sprintf(query,"%s%d",query,id);
	t=mysql_query(conn,query);
	if(t)
	{
		printf("Error making query:%s\n",mysql_error(conn));
		mysql_close(conn);
		return -1;
	}
	mysql_close(conn);
	return 0;
}

int md5_query(char *md5){//error -1, 有 fileID,无 0
	sql_connect();
	char query[300]="select * from file where md5=";
	sprintf(query,"%s%s%s%s",query,"'",md5,"'");
	t=mysql_query(conn,query);
	if(t)
	{
		printf("Error making query:%s\n",mysql_error(conn));
		mysql_close(conn);
		return -1;//-1 查询cmd错误
	}else{
		res=mysql_use_result(conn);
		if(res)
		{
			while((row=mysql_fetch_row(res))!=NULL)//不断获取结果集的下一行
			{	//md5在第4列
				if(strcmp(md5,row[4])==0){//找到md5相同 返回fileID
					int tmp=atoi(row[7]);
					mysql_close(conn);
					return tmp;
				}
			}	
		}
		mysql_free_result(res);
	}
	mysql_close(conn);
	return 0;//没找到md5
}

int name_query(int precode,user_t user,char *name){//查目录下是否有重名 0无 1有 2error
	sql_connect();
	if(precode==-1){//为家目录
		char query[300]="select * from file where precode=0 and name='";
		sprintf(query,"%s%s%s",query,name,"' and type=0");
		t=mysql_query(conn,query);
		if(t)
		{
			printf("Error making query:%s\n",mysql_error(conn));
			mysql_close(conn);
			return 2;//2 查询cmd错误
		}else{
			res=mysql_use_result(conn);
			if(res)
			{
				while((row=mysql_fetch_row(res))!=NULL)//不断获取结果集的下一行
				{	//文件名在第二列
					if(strcmp(name,row[2])==0){
						mysql_close(conn);
						return 1;
					}
				}	
			}
			mysql_free_result(res);
		}	
		mysql_close(conn);
		return 0;
	}
	else{
		char query[300]="select * from file where type=0 and precode=";
		sprintf(query,"%s%d%s%s%s",query,precode," and name='",user.name,"'");
		t=mysql_query(conn,query);
		if(t)
		{
			printf("Error making query:%s\n",mysql_error(conn));
			mysql_close(conn);
			return 2;//2 查询cmd错误
		}else{
			res=mysql_use_result(conn);
			if(res)
			{
				while((row=mysql_fetch_row(res))!=NULL)//不断获取结果集的下一行
				{	//文件名在第二列
					if(strcmp(name,row[2])==0){
						mysql_close(conn);
						return 1;
					}
				}	
			}
			mysql_free_result(res);
		}
		mysql_close(conn);
		return 0;
	}
}

int sql_input(int precode,user_t user,char *name,char *md5,int fileID,long size){//将文件信息插入数据库
	sql_connect();
	if(fileID==0){//fileID=0说明本地无 插入  返回fileID用于非秒传的本地文件创建
		char query1[300]="insert into file(precode,name,belong,md5,type) values(";
		int t;
		sprintf(query1,"%s%d%s%s%s%s%s%s%s%d%s",query1,precode, ",'",name,"','",user.name,"','",md5,"',",0,")");
		t=mysql_query(conn,query1);
		if(t)
		{
			printf("Error making query:%s\n",mysql_error(conn));
			mysql_close(conn);
			return -1;//-1 查询错误
		}
		//插入后获取其ID 即fileID
		int id;
		char query[300]="select * from file where md5='";
		sprintf(query,"%s%s%s",query,md5,"'");
		t=mysql_query(conn,query);
		if(t)
		{
			printf("Error making query:%s\n",mysql_error(conn));
			mysql_close(conn);
			return -1;//-1 查询错误
		}else{
			res=mysql_use_result(conn);
			if(res)
			{
				while((row=mysql_fetch_row(res))!=NULL)//不断获取结果集的下一行
				{	//md5在第4列
					if(strcmp(md5,row[4])==0){//找到md5相同 返回fileID
						id=atoi(row[0]);
						mysql_close(conn);
						break;
					}
				}	
			}
			mysql_free_result(res);
		}
		//mysql_close(conn);
		return id;//得到ID
	}else{//插入
		char query[300]="insert into file(precode,name,belong,md5,size,type,fileID) values(";
		int t;
		sprintf(query,"%s%d%s%s%s%s%s%s%s%ld%s%d%s%d%s",query,precode, ",'",name,"','",user.name,"','",md5,"',",size,",",0,",",fileID,")");
		t=mysql_query(conn,query);
		if(t)
		{
			printf("Error making query:%s\n",mysql_error(conn));
			mysql_close(conn);
			return -1;
		}
		mysql_close(conn);
		return 0;
	}
}

int id_update(int id){
	sql_connect();
    int t;
    char query[300]="update file set fileID=";
	sprintf(query,"%s%d%s%d",query,id," where ID=",id);
	t=mysql_query(conn,query);
	if(t)
	{
		printf("Error making query:%s\n",mysql_error(conn));
		mysql_close(conn);
        return -1;
	}
	mysql_close(conn);
	return 0;
}

int size_update(int id,long size){
    sql_connect();
    int t;
    char query[300]="update file set size="; 
	sprintf(query,"%s%ld%s%d",query,size," where ID=",id);
	t=mysql_query(conn,query);
	if(t)
	{
		printf("Error making query:%s\n",mysql_error(conn));
		mysql_close(conn);
        return -1;
	}
	mysql_close(conn);
	return 0;
}

int mkdir_insert(int precode,user_t user,char *dir){
	sql_connect();
	char query[300]="insert into file(precode,name,belong,type) values(";
	int t;
	sprintf(query,"%s%d%s%s%s%s%s%d%s",query,precode, ",'",dir,"','",user.name,"',",1,")");
	t=mysql_query(conn,query);
	if(t)
	{
		printf("Error making query:%s\n",mysql_error(conn));
		mysql_close(conn);
		return -1;
	}
	mysql_close(conn);
	return 0;
}