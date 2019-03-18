#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
void err_sys(const char* x){
	perror(x);
	exit(1);
}
int main(int argv,char **argc){
	if(argv!=4)err_sys("argument error");
	char r_fifo[20]={0},w_fifo[20]={0};
	sprintf(r_fifo,"host%s_%s.FIFO",argc[1],argc[2]);
	sprintf(w_fifo,"host%s.FIFO",argc[1]);
	//printf("%s %s\n",r_fifo,w_fifo);
	int all_in=0;
	if(argc[2][0]=='A')all_in=0;
	if(argc[2][0]=='B')all_in=1;
	if(argc[2][0]=='C')all_in=2;
	if(argc[2][0]=='D')all_in=3;
	int read_fd,write_fd;
	read_fd=open(r_fifo,O_RDONLY);
	write_fd=open(w_fifo,O_WRONLY);
	for(int i=0;i<10;i++){
		char buf[20],money[5][20];
		int c=0;
		read(read_fd,buf,20);
		char *start=buf;
		start=strtok(start," \n");
		while(start){
			strcpy(money[c++],start);
			start=strtok(NULL," \n");
		}
		char out[50];
		if(i%4==all_in){
			sprintf(out,"%s %s %s\n",argc[2],argc[3],money[all_in]);
		}else{
			sprintf(out,"%s %s 0\n",argc[2],argc[3]);
		}write(write_fd,out,strlen(out));
	}
	close(read_fd);
	close(write_fd);
}
