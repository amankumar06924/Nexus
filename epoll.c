#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>
#include<signal.h>
#include<fcntl.h>
#include<sys/epoll.h>

#include<arpa/inet.h>
#include<sys/socket.h>

#define PORT 9009
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define MAX_EVENTS 64

static volatile sig_atomic_t running =1; 
static void handle_sigint(int signal_number){
(void)signal_number;
running = 0;
}

static int set_nonblocking(int fd){
int flag = fcntl(fd,F_GETFL,0);
if(flag<0){
return -1;
}
int flag1 = fcntl(fd,F_SETFL,flag|O_NONBLOCK);
if(flag1<0){
return -1;
}
return 0;
}
static int create_server(void){
int socket_fd  = socket(AF_INET,SOCK_STREAM,0);
if(socket_fd<0){
perror("socket");
return -1;
}
if(set_nonblocking(socket_fd)<0){
perror("set_nonblocking listener");
close(socket_fd);
return -1;
}
int opt = 1;
if(setsockopt(socket_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))<0){
close(socket_fd);
return -1;
}
struct sockaddr_in address = {
.sin_family = AF_INET,
.sin_port = htons(PORT),
.sin_addr.s_addr = htonl(INADDR_ANY)
};
if(bind(socket_fd,(struct sockaddr *)&address,sizeof(address))<0){
perror("bind");
close(socket_fd);
return -1;
}
if(listen(socket_fd,128)<0){
perror("listen");
close(socket_fd);
return -1;
}
return socket_fd;
}

static void remove_client(struct epoll_event event[],nfds_t *nfds,nfds_t index){
int client_fd = event[index].data.fd;
printf("[server] closing client fd=%d\n",client_fd);
close(client_fd);
free(client_fd);
event[client_fd] = NULL;
}

static void accept_new_client(int socket_fd,struct epoll_event event[],nfds_t *nfds){
while(running){
int client_fd = accept(socket_fd,NULL,NULL);
if(client_fd<0){
if(errno==EINTR){
if(!running){
return;
}
continue;
}
if(errno==EAGAIN || errno== EWOULDBLOCK){
return ;
}
perror("accept");
return;
}
if(*nfds>=MAX_CLIENTS+1){
printf("[server] max client reached, registered fd=%d\n",client_fd);
close(client_fd);
continue;
}
if(set_nonblocking(client_fd)<0){
perror("set_nonblocking");
close(client_fd);
continue;
}
event[*nfds].events = EPOLLIN;
event[*nfds].data.fd = client_fd;
(*nfds)++;
printf("[server] client connected fd=%d\n ",client_fd);
}
}

int main(){
struct sigaction sa;
memset(&sa,0,sizeof(sa));
sa.sa_handler = handle_sigint;
sigemptyset(&sa.sa_mask);
if(sigaction(SIGINT,&sa,NULL)<0){
perror("sigaction");
return 1;
}
int server_fd = create_server_socket();
if(server_fd<0){
return 1;
}
struct epoll_event listener_event = {0};
memset(event,0,sizeof(event));
listener_event.data.fd = server_fd;
listener_event.events = EPOLLIN;
nfds_t nfds = 1;
int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
if(epoll_fd==-1){
perror("failed to create epoll instance");
return 1;
}
int epoll_ctl_fd = epoll_ctl(epoll_fd,EPOLL_CTL_ADD,server_fd,&listener_event);
if(epoll_ctl_fd<0){
perror("epoll_ctl ADD listener");
close(epoll_fd);
close(server_fd);
return 1;
}

struct epoll_event ready_events[MAX_EVENTS];
while(running){
int n = epoll_wait(epoll_fd,ready_events,MAX_EVENTS,-1);
if(n==-1){
printf("[server] error in epoll_wait\n");
}
for(int i = 0;i<n;i++){
int fd = ready_events[i].data.fd;
uint32_t flag = ready_events[i].events;
if(fd==server_fd && (flag&EPOLLIN)){

}
}

}


close(epoll_fd);
return 0;

}
