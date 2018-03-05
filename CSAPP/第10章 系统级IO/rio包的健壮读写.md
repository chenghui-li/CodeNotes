# 代码1 带缓冲的输入输出函数
```
typedef struct {
  int rio_fd;   //缓冲区buf的描述符
  int rio_cnt;  //缓冲区buf中的未读字节数
  char *rio_bufptr; //缓冲区buf中下一个未读的字节
  char rio_buf[MAXN]; //缓冲区buf
}rio_t;
void rio_readinitb(rio_t *rp,int fd){
  rp->rio_fd = fd;
  rp->rio_cnt = 0;
  rp->rio_bufptr = rp->rio_buf;
}
ssize_t rio_read(rio_t *rp,void *usrbuf,size_t n){
  int cnt;
  while(rp->rio_cnt <= 0){   //如果读缓冲区为空，则调用read填充
    rp->rio_cnt = read(rp->rio_fd,rp->rio_buf,sizeof(rp->rio_buf));
    if(rp->rio_cnt < 0){
      if(errno != EINTR)
        return -1;
    }
    else if(rp->rio_cnt == 0) //EOF
      return 0
    else
      rp->rio_bufptr = rp->rio_buf;    //重置bufptr指针
  }
  cnt = n;
  //从rio缓冲区中拷贝n和rp->rio_cnt较小值到用户缓冲区
  if(rp->rio_cnt < n)
    cnt = rio_cnt;
  memcpy(usrbuf,rp->rio_bufptr,cnt);
  rp->rio_bufptr += cnt;   //后移读取了的字节数，指向第一个未读的字节数
  rp->rio_cnt -= cnt;
  return cnt;   //返回读取了的字节数
}
ssize_t rio_readlineb(rio_t *rp,void *usrbuf,size_t maxlen){
  int n,rc;
  char c,*bufp = usrbuf;
  for(n = 1;n<maxlen;n++){
    if((rc = rio_read(rp,&c,1)) == 1){
      *bufp++ = c;
      if(c == '\n'){
        n++;
        break;
      }
    }else if(rc == 0){
      if(n == 1)
        return 0;    //EOF,没有读到数据
      else
        break;     //EOF,有读到数据
    }else
      return -1;     //出错
  }
  *bufp = 0;
  return n-1;    //读到的数据的字节数
}
```
# 分析
- rio_read函数是linux中read函数的带缓冲区的版本。当调用rio_read要求读n个字节时，读缓冲区内有rp->rio_cnt个未读字节。如果缓冲区为空，那么会通过调用read再填满它。这个read收到一个不足值不是错误，它表示读缓冲区填充了一部分。一旦缓冲区非空，rio_read就从读缓冲区复制n和rp->rio_cnt中较小值个字节到用户缓冲区，并返回复制的字节数。

- rio_readlineb函数最大调用maxlen-1次rio_read函数。每次调用从读缓冲区读取一个字节，然后检查该字节是不是末尾的换行符。

# 代码2，rio的无缓冲输入输出函数
```
ssize_t rio_readn(int fd,void *usrbuf,size_t n){
	size_t nleft = n; //未读取字节数
	ssize_t nread;    //实际读取字节
	charr *bufp = usrbuf; //指向第一个未读字节
	while(nleft>0){
		if((nread = read(fd,bufp,nleft))<0){
			if(errno == EINTR)   //被信号中断
				nread = 0;
			else
				return -1;
		}
		else if(nread == 0)   //EOF
			return 0;
		nleft -= nread;
		bufp += nread;
	}
	return (n - nleft);   //目标读取字节数-未读取字节数=实际读取字节数
}
ssize_t rio_writen(int fd,void *usrbuf,size_t n){
	size_t nleft = n;    //未写字节数
	ssize_t nwriten;	 //已写字节数
	char *bufp = usrbuf;
	while(nleft > 0){
		if((nwriten = write(fd,bufp,nleft)) < 0){
			if(errno == EINTR)
				nwriten = 0;
			else
				return -1;
		}
		nleft -= nwriten;
		bufp += nwriten;
	}
	return n;   //最后肯定都写完了
}
```
# 分析
- 通过调用rio_readn和rio_writen函数，应用程序可以在内存和文件之间直接传送数据

- rio_readn函数从描述符fd的当前文件位置最多传送n个字节到内存位置usrbuf。相似的，rio_writen函数从位置usrbuf传送n个字节到描述符fd。

- rio_readn函数在遇到EOF时只能返回一个不足值；rio_w函数不会返回不足值。

- 对同一个描述符，可以任意交错的调用rio_writen和rio_readn