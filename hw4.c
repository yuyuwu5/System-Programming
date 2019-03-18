#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<pthread.h>
#include<time.h>
#define R_data 601
int queue=0,total_thread,total_tree;
FILE *out,*train,*test;
pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
typedef struct{
	int type,sort_by;
	float data[33];
}Data;
typedef struct Node{
	struct Node *left,*right;
	int dimension;
	float threshold;
}Node;
Node *Root[2000];
void invalid(){
	puts("invalid argument\n");
	exit(1);
}
int cmp_int(const void *p1,const void *p2){
	int *s1=(int*)p1,*s2=(int*)p2;
	if(*s1<*s2)return -1;
	if(*s1>*s2)return 1;
	return 0;
}
void read_data(int tmp[],Data input[]){
	char buf[3000];
	int now=0,y;
	for(int i=0;i<25150;i++){
		fgets(buf,3000,train);
		if(i==tmp[now]){
			sscanf(buf,"%d%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%d",&y,&input[now].data[0],&input[now].data[1],&input[now].data[2],&input[now].data[3],&input[now].data[4],&input[now].data[5],&input[now].data[6],&input[now].data[7],&input[now].data[8],&input[now].data[9],&input[now].data[10],&input[now].data[11],&input[now].data[12],&input[now].data[13],&input[now].data[14],&input[now].data[15],&input[now].data[16],&input[now].data[17],&input[now].data[18],&input[now].data[19],&input[now].data[20],&input[now].data[21],&input[now].data[22],&input[now].data[23],&input[now].data[24],&input[now].data[25],&input[now].data[26],&input[now].data[27],&input[now].data[28],&input[now].data[29],&input[now].data[30],&input[now].data[31],&input[now].data[32],&input[now].type);
			//printf("%d %f %d\n",decision[t][now].id,decision[t][now].data[0],decision[t][now].type);
			now++;
		}
	}
}
int cmp(const void *p1,const void *p2){
	Data *s1=(Data*)p1,*s2=(Data*)p2;
	if(s1->data[s1->sort_by]<s2->data[s2->sort_by])return -1;
	if(s1->data[s1->sort_by]>s2->data[s2->sort_by])return 1;
	return 0;
}
Node *make_decision(Data input[5000],int left,int right,int length){
	Node *root=(Node*)malloc(sizeof(Node));
	root->left=NULL; root->right=NULL;
	int zero=0,one=0;
	for(int i=left;i<right;i++){
		if(input[i].type==0)zero++;
		if(input[i].type==1)one++;
	}
	if(zero==length||one==length){
		root->threshold=-1;
		if(zero==length)root->dimension=0;
		else if(one==length)root->dimension=1;
		return root;
	}
	int best_cut,best_dim;
	float best_gini=10000;
	for(int i=0;i<33;i++){
		for(int j=left;j<right;j++)input[j].sort_by=i;
		qsort(&input[left],length,sizeof(Data),cmp);
		for(int j=left;j<right;j++){
			int l_zero=0,l_one=0,r_zero=0,r_one=0;
			for(int l=left;l<=j;l++){
				if(input[l].type==0)l_zero++;
				else if(input[l].type==1)l_one++;
			}
			for(int r=j+1;r<right;r++){
				if(input[r].type==0)r_zero++;
				else if(input[r].type==1)r_one++;
			}
			float lll=(float)l_zero/(float)(l_zero+l_one),rrr=(float)r_zero/(float)(r_zero+r_one);
			float a=(float)(j-left+1)/(float)length;
			float gini=a*lll*(1-lll)+(1-a)*rrr*(1-rrr);
			if(best_gini>gini){
				best_gini=gini;
				best_cut=j;
				best_dim=i;
			}
		}
	}
	for(int i=left;i<right;i++)input[i].sort_by=best_dim;
	qsort(&input[left],length,sizeof(Data),cmp);
	root->threshold=(input[best_cut].data[best_dim]+input[best_cut+1].data[best_dim])/2;
	root->dimension=best_dim;
	root->left=make_decision(input,left,best_cut+1,best_cut-left+1);
	root->right=make_decision(input,best_cut+1,right,right-best_cut-1);
	return root;
}
int build_tree(int t){
	int tmp[3000];					
	for(int i=0;i<R_data;i++){
		tmp[i]=rand()%25150;
	}
	//for(int i=0;i<R_data;i++)printf("%d ",tmp[i]);
	//puts("");
	qsort(tmp,R_data,sizeof(int),cmp_int);
	Data input[5000];
	read_data(tmp,input);
	Root[t]=make_decision(input,0,R_data,R_data);
}
int traverse(Node *root,Data t){
	if(!root->left&&!root->right){
		if(root->dimension==0)return -1;
		if(root->dimension==1)return 1;
	}else if(t.data[root->dimension]<=root->threshold){
		return traverse(root->left,t);
	}else{
		return traverse(root->right,t);
	}
}
void testing(){
	Data t;
	int id;
	while(fscanf(test,"%d",&id)!=EOF){
		for(int i=0;i<33;i++)fscanf(test,"%f",&t.data[i]);
		int win=0;
		for(int i=0;i<total_tree;i++){
			win+=traverse(Root[i],t);
		}
		fprintf(out,"%d,%d\n",id,win>0?1:0);
	}
}
void *func(void *arg){
	//puts("thread start\n");
	while(1){
		pthread_mutex_lock(&lock);
		int tmp=queue;
	//	fprintf(stderr,"%d tmp= %d",(int*)arg,tmp);
		if(tmp<total_tree){
			queue++;
			pthread_mutex_unlock(&lock);
			build_tree(tmp);
	//		fprintf(stderr,"%d done\n",(int*)arg);
		}else{
			pthread_mutex_unlock(&lock);
			pthread_exit(NULL);
		}
	}
}
int main(int argc,char **argv){
	if(argc!=9)invalid();
	if(strcmp(argv[1],"-data")==0){
		char path[50];
		strcat(path,argv[2]);
		strcat(path,"/training_data");
	//	printf("1 %s\n",path);
		train=fopen(path,"rb");
		strcpy(path,argv[2]);
		strcat(path,"/testing_data");
	//	printf("2 %s\n",path);
		test=fopen(path,"rb");
	}else invalid();
	if(strcmp(argv[3],"-output")==0){
		out=fopen(argv[4],"wb");
	}else invalid();
	if(strcmp(argv[5],"-tree")==0){
		total_tree=atoi(argv[6]);
	}else invalid();
	if(strcmp(argv[7],"-thread")==0){
		total_thread=atoi(argv[8]);
	}else invalid();
	srand(time(NULL));
	pthread_t tid[500];
	for(int i=0;i<total_thread;i++){
		if(pthread_create(&tid[i],NULL,func,NULL)!=0){
			puts("cannot create thread\n");
			exit(1);
		}
	}
	for(int i=0;i<total_thread;i++){
		pthread_join(tid[i],NULL);
	}
	testing();
	fclose(train);
	fclose(test);
	fclose(out);
}
