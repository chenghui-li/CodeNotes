# 代码1
```
typedef struct{  //用于存储连接描述符的缓冲区
	int maxfd;    //read_set集合中的最大描述符
	fd_set read_set;   //所有已连接描述符的集合
	fd_set ready_set;  //准备好读的描述符的子集
	int nready;    //select中的准备好可以读的描述符的数目
	int maxi;      //client数组中的最高水平线
	int clientfd[FD_SETSIZE];   //存储已连接描述符的集合
	rio_t clientrio[FD_SETSIZE]; //读缓冲区的集合
}pool
int byte_cnt = 0;    //服务器接收到的总的字节数
void error_(const char *info){
	printf("error in %s\n",*info);
	exit(1);
}
int main(int argc,char *argv[]){
	int listenfd,connfd;
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	static pool pool;
	if(argc != 2){
		error_("input");
	}
	listenfd = open_listenfd(argv[1]);
	init_pool(listenfd,&pool);
	while(1){
		//等待listenfd或者connfd准备好可以读
		pool.ready_set = pool.read_set;
		if((pool.nready = select(pool.maxfd+1,&pool.ready_set,NULL,NULL,NULL))<=0){
			error("select");
		}
		//如果listenfd准备好可以读，则添加一个新的client到pool中
		if(FD_ISSET(listenfd,&pool.ready_set)){
			clientlen = sizeof(struct sockaddr_storage);
			if((connfd = accept(listenfd,(struct sockaddr *)&clientaddr,&clientlen))<0)
				error_("accept");
			add_client(connfd,&pool);
		}
		//向每一个connfd回显一个文本行
		check_clients(&pool);
	}
}

```

# 分析
- 一个pool结构维护着客户端的集合，调用init_pool可以初始化该池，之后服务器进入无限循环，服务器调用select判断哪个描述符准备好可以读。a.一个来自新客户端的连接请求到达，b.一个已连接客户端的connfd可以读了。当一个连接请求到达时，调用add_client将客户端添加到池里。最后服务器调用check_clients函数将向每一个客户端回显文本行。

# 代码2
```
void init_pool(int listenfd,pool *p){
	int i;
	p->maxi = -1;
	for(i = 0;i<FD_SETSIZE;i++){
		p->clientfd[i] = -1;   //没有客户端连接时，数组值为-1，即值为-1的槽可用
	}
	p->maxfd = listenfd;
	FD_ZERO(p->read_set);
	//初始状态下，listenfd是read_set唯一的成员
	FD_SET(listenfd,p->read_set);
}
```

# 分析
init_pool函数初始化池，clientfd数组表示已连接描述符，值为-1表示一个可用槽位。初始时已连接描述符的集合（clientfd数组）为空，并且listenfd是select读集合中的唯一成员。

# 代码3
```
void add_client(int connfd,pool *p){
	int i;
	p->nready --;
	for(i = 0;i<FD_SETSIZE;i++){
		if(p->client[i] < 0){   //找到空槽位
			p->client[i] = connfd;
			rio_readinitb(&p->clientrio[i],connfd);
			FD_SET(connfd,&p->read_set);   //将已连接描述符添加到准备好读的集合中
			if(connfd > maxfd)    //更新最大描述符
				maxfd = connfd;
			if(i > maxi)   //更新client数组的最高水平线
				maxi = i;
			break;
		}
	}
	if(i == FD_SETSIZE){    //没有空位表示数组已满
		error_("overflow");
	}
}
```

# 分析
add_client函数负责将已连接描述符添加到准备好读集合中，如果有空槽位，则加入，更新最高水平线和最大描述符（select函数所需），初始化对应的rio读缓冲区。如果没有空位，则抛出异常。

# 代码4
```
void check_client(pool *p){
	int i;
	int n;
	for(i = 0;i<maxi && p->nready > 0 ;i++){
		if(p->client[i] > 0 && FD_ISSET(p->client[i],&p->read_set)){
			//读取到数据
			if((n = rio_readlineb(&p->clientrio[i],buf,MAXN)) != 0){
				p->cnt += n;  //总的数据量累加
				rio_writen(p->client[i],buf,n);    //将数据回显
			}
			else{    //EOF
				if(close(p->client[i]) < 0)
					error_("close");
				FD_CLR(p->client[i],&p->read_set);
				p->client[i] = -1;   //释放槽位
			}
		}
	}
}
```

# 分析
遍历描述符数组，上限为maxi，并且准备好读描述符数大于0，