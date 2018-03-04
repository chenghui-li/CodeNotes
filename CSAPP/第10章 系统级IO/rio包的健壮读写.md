# 代码
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
  while(rp->rio_cnt <= 0){
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