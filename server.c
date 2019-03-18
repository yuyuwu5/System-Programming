#define _GNU_SOURCE
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>

#define ERR_EXIT(a) { perror(a); exit(1); }

typedef struct {
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;

typedef struct {
	int id,amount,price;
}Item;
typedef struct {
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[512];  // data sent by/to client
    size_t buf_len;  // bytes used by buf
    // you don't need to change this.
	int item;
    int wait_for_write;  // used by handle_read to know if the header is read or not.
} request;

server svr;  // server
request* requestP = NULL;  // point to a list of requests
int maxfd;  // size of open file descriptor table, size of request list

const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";

// Forwards

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance

static int handle_read(request* reqP);
// return 0: socket ended, request done.
// return 1: success, message (without header) got this time is in reqP->buf with reqP->buf_len bytes. read more until got <= 0.
// It's guaranteed that the header would be correctly set after the first read.
// error code:
// -1: client connection error

int lock_init(int fd,int cmd,int type,off_t offset,int whence,off_t len){
	struct flock lock;
	lock.l_type=type;
	lock.l_start=offset;
	lock.l_whence=whence;
	lock.l_len=len;
	return fcntl(fd,cmd,&lock);
}


int main(int argc, char** argv) {
    int i, ret;

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen=0;

    int conn_fd;  // fd for a new connection with client
    int file_fd;  // fd for file that we open for reading
    char buf[516];
    int buf_len;

    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    // Initialize server
    init_server((unsigned short) atoi(argv[1]));
    // Get file descripter table size and initize request table
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);
    fd_set read_fd;
    fd_set master;
    FD_ZERO(&read_fd);
    FD_ZERO(&master);
    FD_SET(svr.listen_fd,&master);
    int inside[2048]={0},fid[2048]={0};
    while (1) {
	    // TODO: Add IO multiplexing
	    read_fd=master;
	    int activity;
	    activity=select(maxfd+1,&read_fd,NULL,NULL,NULL);
	    for(int i=0;i<=maxfd;i++){
		    //int fd=requestP[i].conn_fd;
		    int fd=i;
		    if(FD_ISSET(fd,&read_fd)){
			    if(fd==svr.listen_fd){
				    // Check new connection
				    clilen = sizeof(cliaddr);
				    conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
				    if (conn_fd < 0) {
					    if (errno == EINTR || errno == EAGAIN) continue;  // try again
					    if (errno == ENFILE) {
						    (void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
						    continue;
					    }ERR_EXIT("accept")
				    }else{
					    FD_SET(conn_fd,&master);
				    }
				    requestP[conn_fd].conn_fd = conn_fd;
				    strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
				    fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);
			    }else{
#ifdef READ_SERVER
				    ret = handle_read(&requestP[fd]); // parse data from client to requestP[conn_fd].buf
				    if (ret < 0) {
					    fprintf(stderr, "bad request from %s\n", requestP[fd].host);
					    continue;
				    }
				    int query=atoi(requestP[fd].buf);
				    if(query<=20&&query>0){
				    	fid[fd]=open("item_list",O_RDONLY,0);
				    	if(fid[fd]>=0){
					    	int rt=lock_init(fid[fd],F_OFD_SETLK,F_RDLCK,sizeof(Item)*(query-1),SEEK_SET,sizeof(Item));
						if(rt<0){
				    			sprintf(buf,"This item is locked.\n");
				    			write(requestP[fd].conn_fd, buf, strlen(buf));
				    			close(requestP[fd].conn_fd);
				    			free_request(&requestP[fd]);
				   			FD_CLR(fd,&master);
							continue;
					    	}lock_init(fid[fd],F_OFD_SETLK,F_UNLCK,sizeof(Item)*(query-1),SEEK_SET,sizeof(Item));
				    		close(fid[fd]);
				    	}
				   		FILE *fp;
				  		fp=fopen("item_list","rb");
						fseek(fp,(query-1)*sizeof(Item),SEEK_SET);
						Item t;
				    	fread(&t,sizeof(Item),1,fp);
				    	sprintf(buf,"item%d $%d remain: %d\n",query,t.price,t.amount);
				    	write(requestP[fd].conn_fd, buf, strlen(buf));
				    	fclose(fp);
				    }else{
				    	sprintf(buf,"Out Of Range\n");
				    	write(requestP[fd].conn_fd, buf, strlen(buf));
				    }
				    close(requestP[fd].conn_fd);
				    free_request(&requestP[fd]);
				    FD_CLR(fd,&master);
#else
				    if(inside[fd]==0){
				    	ret = handle_read(&requestP[fd]); // parse data from client to requestP[conn_fd].buf
				    	if (ret < 0) {
						    fprintf(stderr, "bad request from %s\n", requestP[fd].host);
						    continue;
						}inside[fd]=atoi(requestP[fd].buf);
				         fid[fd]=open("item_list",O_WRONLY,0);
				         
					 int rt=lock_init(fid[fd],F_OFD_SETLK,F_WRLCK,sizeof(Item)*(inside[fd]-1),SEEK_SET,sizeof(Item));
					 if(rt<0){
				    		sprintf(buf,"This item is locked.\n");
				    		write(requestP[fd].conn_fd, buf, strlen(buf));
				    		close(requestP[fd].conn_fd);
				    		free_request(&requestP[fd]);
				   		FD_CLR(fd,&master);
				   		inside[fd]=0;
						continue;
					    }
					    sprintf(buf,"This item is modifiable.\n");
				        write(requestP[fd].conn_fd, buf, strlen(buf));
				        continue;
				    }else if(inside[fd]>0){
				   	 ret=handle_read(&requestP[fd]);
				   	 if(ret<0){
						    fprintf(stderr, "bad request from %s\n", requestP[fd].host);
						    continue;
					    }
					    char action[20];
					    int now=0,num;
					    for(int i=0;i<strlen(requestP[fd].buf);i++){
						    if(isalpha(requestP[fd].buf[i])){
							    action[now++]=requestP[fd].buf[i];
						    }else{
							    action[now++]='\0';
							    num=atoi(&requestP[fd].buf[i+1]);
							    break;
						    }
					    }
					    if(num>=0){
						    FILE *fp;
						    fp=fopen("item_list","rb");
						    fseek(fp,(inside[fd]-1)*sizeof(Item),SEEK_SET);
						    Item t;
						    fread(&t,sizeof(Item),1,fp);
						    fclose(fp);
						    if(strcmp(action,"buy")==0){
							    if(t.amount-num>=0){
								    t.amount-=num;
								    FILE *fp;
								    fp=fopen("item_list","r+");
								    fseek(fp,(inside[fd]-1)*sizeof(Item),SEEK_SET);
								    fwrite(&t,sizeof(Item),1,fp);
								    fclose(fp);
							    }else{
								    sprintf(buf,"Operation failed.\n");
								    write(requestP[fd].conn_fd, buf, strlen(buf));
							    }
						    }else if(strcmp(action,"sell")==0){
							    t.amount+=num;
							    FILE *fp;
							    fp=fopen("item_list","r+");
							    fseek(fp,(inside[fd]-1)*sizeof(Item),SEEK_SET);
							    fwrite(&t,sizeof(Item),1,fp);
							    fclose(fp);
						    }else if(strcmp(action,"price")==0){
							    t.price=num;
							    FILE *fp;
							    fp=fopen("item_list","r+");
							    fseek(fp,(inside[fd]-1)*sizeof(Item),SEEK_SET);
							    fwrite(&t,sizeof(Item),1,fp);
							    fclose(fp);
						    }else{
							    sprintf(buf,"Operation failed.\n");
							    write(requestP[fd].conn_fd, buf, strlen(buf));
						    }
					    }else{
						    sprintf(buf,"Operation failed.\n");
						    write(requestP[fd].conn_fd, buf, strlen(buf));
					    }
					    lock_init(fid[fd],F_OFD_SETLK,F_UNLCK,sizeof(Item)*(inside[fd]-1),SEEK_SET,sizeof(Item));
				    	close(fid[fd]);
					    close(requestP[fd].conn_fd);
					    free_request(&requestP[fd]);
					    inside[fd]=0;
					    FD_CLR(fd,&master);
				    }
#endif
			    }
		    }
	    }
    }free(requestP);
    return 0;
}


// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void* e_malloc(size_t size);


static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->item = 0;
    reqP->wait_for_write = 0;
}

static void free_request(request* reqP) {
    /*if (reqP->filename != NULL) {
        free(reqP->filename);
        reqP->filename = NULL;
    }*/
    init_request(reqP);
}

// return 0: socket ended, request done.
// return 1: success, message (without header) got this time is in reqP->buf with reqP->buf_len bytes. read more until got <= 0.
// It's guaranteed that the header would be correctly set after the first read.
// error code:
// -1: client connection error
static int handle_read(request* reqP) {
    int r;
    char buf[512];

    // Read in request from client
    r = read(reqP->conn_fd, buf, sizeof(buf));
    if (r < 0) return -1;
    if (r == 0) return 0;
	char* p1 = strstr(buf, "\015\012");
	int newline_len = 2;
	// be careful that in Windows, line ends with \015\012
	if (p1 == NULL) {
		p1 = strstr(buf, "\012");
		newline_len = 1;
		if (p1 == NULL) {
			ERR_EXIT("this really should not happen...");
		}
	}
	size_t len = p1 - buf + 1;
	memmove(reqP->buf, buf, len);
	reqP->buf[len - 1] = '\0';
	reqP->buf_len = len-1;
    return 1;
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }
}

static void* e_malloc(size_t size) {
    void* ptr;

    ptr = malloc(size);
    if (ptr == NULL) ERR_EXIT("out of memory");
    return ptr;
}
