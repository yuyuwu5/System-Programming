#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
void err_sys(const char* x){ 
	perror(x);
	exit(1); 
}
typedef struct{
	int index,rank,money;
}form;
int cmp(const void *p1,const void *p2){
	form *s1=(form*)p1,*s2=(form*)p2;
	if(s1->money>s2->money)return -1;
	if(s1->money<s2->money)return 1;
	return 0;
}
int main(int argc,char **argv){
	if(argc!=2)err_sys("argument error");
	int id=atoi(argv[1]);
	char tmp[5][20];
	sprintf(tmp[0],"host%d_A.FIFO",id);
	sprintf(tmp[1],"host%d_B.FIFO",id);
	sprintf(tmp[2],"host%d_C.FIFO",id);
	sprintf(tmp[3],"host%d_D.FIFO",id);
	sprintf(tmp[4],"host%d.FIFO"  ,id);
	for(int i=0;i<5;i++)mkfifo(tmp[i],0666);
	while(1){
		int player[4];
		fscanf(stdin,"%d %d %d %d",&player[0],&player[1],&player[2],&player[3]);
		if(player[0]==-1||player[1]==-1||player[2]==-1||player[3]==-1){
			for(int i=0;i<=4;i++)unlink(tmp[i]);
			exit(0);
		}
		//printf("%d %d %d %d\n",player[0],player[1],player[2],player[3]);
		int fd_write[4],fd_read;
		int money[4]={0},key[4];
		char index[4][2]={{'A'},{'B'},{'C'},{'D'}};
		pid_t child[4];
		for(int i=0;i<4;i++){
			key[i]=(rand()%65536);
			char Key[8];
			sprintf(Key,"%d%c",key[i],'\0');
			child[i]=fork();
			if(child[i]==0){
				if(execl("player","./player",argv[1],index[i],Key,(char*)NULL)==-1)
					err_sys("player error");
			}else if(child[i]<0){
					err_sys("fork error");
			}
			else{
				fd_write[i]=open(tmp[i],O_WRONLY);
			}
		}
		fd_read=open(tmp[4],O_RDONLY);
		int winner[4]={0};
		for(int i=0;i<10;i++){
			for(int j=0;j<4;j++){
				money[j]+=1000;
			}
			char M[50]={0},buf[100]={0},ind[4][2];
			int Money[4],rand_key[4],inde[4];
			sprintf(M,"%d %d %d %d\n",money[0],money[1],money[2],money[3]);
			for(int j=0;j<4;j++){
				write(fd_write[j],M,strlen(M));
				read(fd_read,buf,100);
				sscanf(buf,"%s%d%d",ind[j],&rand_key[j],&Money[j]);
				if(ind[j][0]=='A')inde[j]=0;
				if(ind[j][0]=='B')inde[j]=1;
				if(ind[j][0]=='C')inde[j]=2;
				if(ind[j][0]=='D')inde[j]=3;
			}int max_money=-1,max_id;
			for(int j=0;j<4;j++){
				if(rand_key[j]==key[inde[j]]){
					if(max_money<Money[j]){
						max_money=Money[j];
						max_id=inde[j];
					}
				}
			}
			money[max_id]-=max_money;
			winner[max_id]++;
		}
		form r[4];
		for(int i=0;i<4;i++){
			r[i].index=player[i];
			r[i].money=winner[i];
		}
		qsort(r,4,sizeof(form),cmp);
		r[0].rank=1;
		for(int i=1;i<4;i++){
			if(r[i].money==r[i-1].money){
				r[i].rank=r[i-1].rank;
			}else r[i].rank=i+1;
		}
		char buff[200];
		int Index[5];
		for(int i=0;i<4;i++){
			for(int j=0;j<4;j++){
				if(r[j].index==player[i]){
					Index[i]=j;
				}
			}
		}
	sprintf(buff,"%d %d\n%d %d\n%d %d\n%d %d\n",player[Index[0]],r[Index[0]].rank,player[Index[1]],r[Index[1]].rank,player[Index[2]],r[Index[2]].rank,player[Index[3]],r[Index[3]].rank);
		fprintf(stdout,"%s",buff);
		fflush(stdout);
		close(fd_write[0]);
		close(fd_write[1]);
		close(fd_write[2]);
		close(fd_write[3]);
		close(fd_read);
		wait(0);
		wait(0);
		wait(0);
		wait(0);
	}
}
