#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <fcntl.h>
#define SIGUSR3 SIGWINCH
FILE *b_log;
int customer[3]={0},count[3]={0},work[3]={0},on[3]={0};
pid_t child;
void sig_control(int signo){
	if(signo==SIGUSR1){
		customer[0]++;
		count[0]=20;
		work[0]=5;
		on[0]=1;
	}else if(signo==SIGUSR2){
		//fprintf(stderr, "get patient\n" );
		customer[1]++;
		count[1]=30;
		work[1]=10;
		on[1]=1;
	}else if(signo==SIGUSR3){
		customer[2]++;
		count[2]=3;
		work[2]=2;
		on[2]=1;
	}
}
int main(int argv,char **argc){
	FILE *test_data;
	test_data=fopen(argc[1],"rb");
	b_log=fopen("bidding_system_log","wb");
	int r_pipe[2];
	if(pipe(r_pipe)<0){
		perror("pipe error");
		exit(1);
	}
	if((child=fork())<0){
		perror("fork error");
		exit(1);
	}else if(child==0){
		dup2(r_pipe[1],STDOUT_FILENO);
		for(int i=0;i<2;i++){
			close(r_pipe[i]);
		}
		if(execl("customer_EDF","./customer_EDF",argc[1],NULL)<0){
			perror("execute customer_EDF error");
			exit(1);
		}
	}else{
		close(r_pipe[1]);
		struct sigaction act;
		act.sa_handler=sig_control;
		sigemptyset(&act.sa_mask);
		act.sa_flags=0;
		if(sigaction(SIGUSR1,&act,NULL)<0){
			perror("cannot catch SIGUSR1");
			exit(1);
		}
		if(sigaction(SIGUSR2,&act,NULL)<0){
			perror("cannot catch SIGUSR2");
			exit(1);
		}
		if(sigaction(SIGUSR3,&act,NULL)<0){
			perror("cannot catch SIGUSR2");
			exit(1);
		}
		struct timespec ts={0,100000000L};
		fcntl(r_pipe[0], F_SETFL, O_NONBLOCK);
		int countdown=-1;
		while(1){	
			countdown--;
			if(on[0]||on[1]||on[2]){
				int id,min=1000000;
				for(int i=0;i<3;i++){
					if(on[i]){
						if(min>count[i]){
							min=count[i];
							id=i;
						}
					}
				}
				if((id==0&&work[id]==5)||(id==1&&work[id]==10)||(id==2&&work[id]==2)){
					fprintf(b_log,"receive %d %d\n",id,customer[id]);
				}
				work[id]--;
				if(work[id]==0){
					count[id]=0;
					on[id]=0;
					fprintf(b_log, "finish %d %d\n",id,customer[id]);
					if(id==0)kill(child,SIGUSR1);
					else if(id==1)kill(child,SIGUSR2);
					else if(id==2)kill(child,SIGUSR3);
				}
				for(int i=0;i<3;i++)if(on[i])count[i]--;
			}
			
			char buf[15]={0};
			int n=read(r_pipe[0],buf,15);
			if(n>0&&strcmp(buf,"terminate\n")==0){
				countdown=10;
			}
			if(countdown==0){
				fprintf(b_log, "terminate\n");
				fclose(b_log);
				exit(0);
			}
			
			sigset_t new_mask;
			sigset_t origin_mask;
			sigemptyset(&new_mask);
			sigaddset(&new_mask,SIGUSR1);
			sigaddset(&new_mask,SIGUSR2);
			sigaddset(&new_mask,SIGUSR3);
			if(sigprocmask(SIG_BLOCK,&new_mask,&origin_mask)<0){
				perror("Blocking error\n");
			}
			nanosleep(&ts,NULL);
			if(sigprocmask(SIG_SETMASK,&origin_mask,NULL)<0){
				perror("restore error\n");
			}
		}
	}
}
