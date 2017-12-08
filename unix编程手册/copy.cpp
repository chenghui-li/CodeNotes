/*
功能：将一个文件的内容拷贝到另一个文件
知识点：open,close,read,write函数的使用
概述：
	open(pathname,flags,mode);
	close(fd);
	read(fd,buffer,size);
	write(fd,buffer,size);
*/
#include<unistd.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdio.h>
#define MAX_SIZE 1024
void _error(char *info){
	printf(" error %s\n", info);
	exit(1);
}
int main(int argc,char *argv[]){
	int numRead,numWrite;
	int inputFd,outputFd;
	char buffer[MAX_SIZE];
	if(argc != 3){
		_error("input");
	}
	if(inputFd = open(argv[1],O_RDONLY) == -1)
		_error("open input");
	if ((outputFd = open(argv[2],O_WRONLY|O_CREAT|O_TRUNC,S_IWUSR|S_IRUSR|S_IWGRP|S_IRGRP|S_IWOTH|S_IROTH)) == -1)
	{
		_error("open output");
	}
	while((numRead = read(inputFd,buffer,MAX_SIZE))>0){
		if(write(outputFd,buffer,numRead) != numRead)
			_error("can't write whole buffer");
	}
	if(numRead == -1)
		_error("read");
	if(close(inputFd) == -1)
		_error("close input");
	if(close(outputFd) == -1)
		_error("close output");
	exit(0);

}