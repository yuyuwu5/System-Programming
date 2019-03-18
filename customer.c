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
#include <limits.h>
#include <fcntl.h>
int ordinary=0,ord=0,member=0,vip=0,vip_on=0,member_on=0;
struct timeval vip_came,member_came,started;
FILE *c_log;
static void sig_control(int signo){
	if(signo==SIGUSR1){
		fprintf(c_log,"finish 1 %d\n",member);
		member_on=0;
	}
	else if(signo==SIGUSR2){
		fprintf(c_log,"finish 2 %d\n",vip);
		vip_on=0;
	}
	else if(signo==SIGINT){
		ord++;
		fprintf(c_log,"finish 0 %d\n",ord); 
	}
}
int get_time(struct timeval a,struct timeval b){
	return (a.tv_sec-b.tv_sec)*1000000+(a.tv_usec-b.tv_usec);
}
int main(int argv,char **argc){
	FILE *test_data;
	test_data=fopen(argc[1],"rb");
	c_log=fopen("customer_log","wb");
	int type,real=0,pre=0;
	float start;
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
	if(sigaction(SIGINT,&act,NULL)<0){
		perror("cannot catch SIGUSR2");
		exit(1);
	}
	pid_t parent=getppid();
	struct timeval now;
	struct timespec ts={0,100000000L};
	while(1){
		if(fscanf(test_data,"%d%f",&type,&start)!=EOF){
			real=(int)(start*10)-pre;
			pre=(int)(start*10);
//			fprintf(stderr, "%d\n",real);
			for(int i=0;i<real;i++){
				gettimeofday(&now,NULL);
			//	fprintf(stderr,"%ld %ld\n",now.tv_sec,now.tv_usec);
				if(member_on==1&&get_time(now,member_came)>1000000){
					fprintf(c_log, "timeout 1 %d\n",member);
					fclose(test_data);
					fclose(c_log);
					exit(0);
				}
				if(vip_on==1&&get_time(now,vip_came)>300000){
					fprintf(c_log, "timeout 2 %d\n",vip);
					fclose(test_data);
					fclose(c_log);
					exit(0);
				}
				if(nanosleep(&ts,NULL)<0)i--;
			}
			if(type==0){
				printf("ordinary\n");
				fflush(stdout);
				ordinary++;
				fprintf(c_log,"send 0 %d\n",ordinary); 
			}else if(type==1){
				kill(parent,SIGUSR1);
				gettimeofday(&member_came,NULL);
			//	fprintf(stderr,"member %ld %ld\n",member_came.tv_sec,member_came.tv_usec);
				member++;
				member_on=1;
				fprintf(c_log,"send 1 %d\n",member); 
			}else if(type==2){
				kill(parent,SIGUSR2);
				gettimeofday(&vip_came,NULL);
			//	fprintf(stderr,"vip %ld %ld\n",vip_came.tv_sec,vip_came.tv_usec);
				vip++;
				vip_on=1;
				fprintf(c_log,"send 2 %d\n",vip); 
			}
		}else{
			for(int i=0;i<20;i++){
				gettimeofday(&now,NULL);
				if(member_on==1&&get_time(now,member_came)>1000000){
					fprintf(c_log, "timeout 1 %d\n",member);
					fclose(test_data);
					fclose(c_log);
					exit(0);
				}
				if(vip_on==1&&get_time(now,vip_came)>300000){
					fprintf(c_log, "timeout 2 %d\n",vip);
					fclose(test_data);
					fclose(c_log);
					exit(0);
				}
				nanosleep(&ts,NULL);
			}
			fclose(test_data);
			fclose(c_log);
			exit(0);
		}
	}
}
