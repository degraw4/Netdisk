# Netdisk
A netdisk with server and client

操作手册
0、简介
支持cmd基本操作 可以创建文件夹，上传+秒传，下载+断点下载，数据库虚拟文件系统，密码密文传输（但数据库还是明文存储...），ls上色，客户端多线程，超时触发等

1、客户端
ip和端口为了启动方便被我写死在了程序里，其中client.c为客户端主线程，处理除了put和get之外的命令，get和put用了两个线程函数，再次连接服务器，实现get和put与其他命令分离。在client.c的主程序和两个线程中要修改连接的ip和端口（ctrl+c即可退出）

2、客户端
客户端也要修改ip、端口、子线程数和最大线程数，对应line30和24、25
	
3、数据库初始化
程序用了数据库储存用户信息和文件信息，要先建立数据库再启动程序
	Create database netdisc;   建立网盘数据库
	
	Create table user(ID int NOT NULL AOTU_INCREMENT,PRIMARY KEY(ID),name varchar(20),slat varchar(20),psw varchar(20));   用户表(salt为盐值 但没用到 为了不改变列数而修改程序 要插入)
	
	Create table file((ID int NOT NULL AOTU_INCREMENT,PRIMARY KEY(ID),precode int,name varchr(20),belong varchar(20),md5 varchar(50),size varchar(40),type int,fileID int); 
	
	文件表
	
4、注册登录后 支持命令：ls、pwd、cd xxx（./../~/dir）、get xxx（下载psw路径下有的文件 不支持下载文件夹）、put xxx（上传client目录下有的文件）、rm xxx（删除psw下文件，不可删文件夹）、mkdir xxx（在psw下创建文件夹 注意不要重名 因为懒得处理了）exit退出

5、运行后 会在服务器文件夹下生产log日志文件记录操作

6、程序分析（相关函数注释在thread_pool.h和client.h中）：

6.1、客户端收到命令，识别后发送操作码，如ls为1，put和get拉起线程处理

6.2、服务器每个线程处理一个socket连接，put和get也会收到新的连接 但是可以识别此连接和新用户连接的区别
	
6.3、file数据库，ID为主键 precode为该文件所在目录的主键，name为名字 belong为所属用户，MD5为文件MD5 目录无，size为大小 目录无，type1为目录 0为文件，fileID为文件实际存在file文件夹下的名字，设计这个的原因为使得MD5相同的文件只存一次，即fileID=ID为第一个上传的文件，后面上传了相同MD5的文件，file文件夹就不存了，数据库中让后上传的文件的fileID=第一个上传的文件的ID, 因此第一个上传的文件的ID=fileID，以此实现秒传，但是给数据库插入和文件实际删除造成了困难，因此rm功能只删除了数据库信息，没有实际删除文件
	
6.4、user_t结构存储用户name，psw密码，path路径数组和pathlen数组长度，path数组即为每一级路径对应的文件夹ID，数组初始化为0，用户刚登陆默认path[0]=-1，表示在家目录，但在数据库中家目录显示为precode=0
	
6.5、服务器线程取任务是在任务队列取节点，因此线程取任务时，会根据对方发的flag为0（表示是新连接）或1（表示为已连接用户请求下载）或2（上传）来识别
	
6.6、user_t user结构体为每个线程一个，便于实现不同用户并发，因此在处理put、get时，要先让服务器主线程把当前user信息发给client主线程，client主线程在把user信息发给下载上传线程，下载、上传线程重连服务器新线程后再发user信息，实现了当前下载或上传路径的传递
