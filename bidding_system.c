#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/wait.h>
void err_sys(const char* x){ 
	perror(x);
	exit(1); 
}
int combination[5000][4];
void working_combination(int l){
	int c=0;
	for(int i=1;i<=l;i++){
		for(int j=i+1;j<=l;j++){
			for(int k=j+1;k<=l;k++){
				for(int d=k+1;d<=l;d++){
					combination[c][0]=i;
					combination[c][1]=j;
					combination[c][2]=k;
					combination[c++][3]=d;
				}
			}
		}
	}
}
typedef struct{
	int index,score,rank;
}form;
int cmp(const void *p1,const void *p2){
	form *s1=(form*)p1,*s2=(form*)p2;
	if(s1->score<s2->score)return -1;
	if(s1->score>s2->score)return 1;
	return 0;
}
int main(int argc,char **argv){
	if(argc!=3){
		err_sys("argument error");
	}
	fd_set read_fd;
	fd_set write_fd;
	fd_set read_master;
	fd_set write_master;
	FD_ZERO (&read_fd);
	FD_ZERO (&read_master);
	FD_ZERO (&write_fd);
	FD_ZERO (&write_master);
	int maxfd = getdtablesize();
	int host=atoi(argv[1]),player=atoi(argv[2]);
	pid_t pid[host+1];
	int fd_server_read[host+1][2];
	int fd_server_write[host+1][2]; 
	int transform[1024];
	for(int i=1;i<=host;i++){
		if(pipe(fd_server_read[i])<0)err_sys("pipe error");
		if(pipe(fd_server_write[i])<0)err_sys("pipe error");
		if((pid[i]=fork())<0){
			err_sys("fork error");
		}else if(pid[i]==0){ //child host
			dup2(fd_server_read[i][1],STDOUT_FILENO); //host to write
			dup2(fd_server_write[i][0],STDIN_FILENO); //host to read
			close(fd_server_read[i][1]);
			close(fd_server_write[i][0]);
			close(fd_server_read[i][0]); 
			close(fd_server_write[i][1]); 
			char arg[3]={0};
			if(i>=10){
				arg[0]='1';
				arg[1]=(i%10)+'0';
			}else{
				arg[0]=(i%10)+'0';
			}if(execl("host","./host",arg,(char*)NULL)==-1){
				err_sys("execl error");
			}
		}else{ //bidding server
			FD_SET(fd_server_read[i][0],&read_master);
			FD_SET(fd_server_write[i][1],&write_master);
			transform[fd_server_read[i][0]]=fd_server_write[i][1];
			close(fd_server_read[i][1]); 
			close(fd_server_write[i][0]); 
		}
	}
	working_combination(player);
	int total=player*(player-1)*(player-2)*(player-3)/24,now=0;
	for(int i=total;i<5000;i++)for(int j=0;j<4;j++)combination[i][j]=-1;
	write_fd=write_master;
	int k=select(maxfd+1,NULL,&write_fd,NULL,NULL);
	for(int i=0;i<maxfd+1;i++){
		if(FD_ISSET(i,&write_fd)){
			char buf[20];
			sprintf(buf,"%d %d %d %d\n",combination[now][0],combination[now][1],combination[now][2],combination[now][3]);
			now++;
			write(i,buf,strlen(buf));	
		}
	}
	form score[21]={0};
	for(int i=0;i<21;i++){
		score[i].index=i;
		if(i>player)score[i].score=1000000000;
	}
	while(now<total+host){
		read_fd=read_master;
		int activity=select(maxfd+1,&read_fd,NULL,NULL,NULL);
				//puts("get");
		for(int i=0;i<maxfd+1;i++){
			if(FD_ISSET(i,&read_fd)){
				char rank[50],target[8][3];
				read(i,rank,50);
				int k[8],c=0;
				char *start=rank;
				start=strtok(start," \n");
				while(start){
					strcpy(target[c++],start);
					start=strtok(NULL," \n");
				}for(int i=0;i<8;i++){
					k[i]=atoi(target[i]);
				}for(int i=0;i<4;i++){
					score[k[2*i]].score+=k[2*i+1];
				}
				char buf[20];
				sprintf(buf,"%d %d %d %d\n",combination[now][0],combination[now][1],combination[now][2],combination[now][3]);
				write(transform[i],buf,strlen(buf));
				if(now>=total)FD_CLR(i,&read_master);
				now++;
			}
		}
	}
	qsort(score,21,sizeof(form),cmp);
	score[0].rank=1;
	for(int i=1;i<21;i++){
		if(score[i].score==score[i-1].score)score[i].rank=score[i-1].rank;
		else score[i].rank=i;
	}
	for(int i=1;i<=player;i++){
		for(int j=0;j<21;j++){
			if(score[j].index==i){
				printf("%d %d\n",i,score[j].rank);
				break;
			}
		}
	}for(int i=0;i<host;i++)wait(0);
}
