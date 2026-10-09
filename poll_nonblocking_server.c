#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>

#define PORT 9008
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
static volatile sig_atomic_t running = 1;
static void handle_sigint(int signal_number){
(void)signal_number;
running = 0;
}
static int set_nonblocking(int fd){
int flags = fcntl(fd, F_GETFL, 0);
if(flags < 0){
return -1;
}
if(fcntl(fd, F_SETFL, flags | O_NONBLOCK)<0){
return -1;
}
return 0;
}
static int create_server_socket(void){
int server_fd = socket(AF_INET, SOCK_STREAM, 0);
if(server_fd<0){
perror("socket");
return -1;
}
int opt = 1;
if(setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))<0){
perror("setsockopt");
close(server_fd);
return -1;
}
struct sockaddr_in address = {
.sin_family = AF_INET,
.sin_port = htons(PORT),
.sin_addr.s_addr = htonl(INADDR_ANY)
};
if(bind(server_fd,(struct sockaddr *)&address,sizeof(address))<0){
perror("bind");
close(server_fd);
return -1;
}
if(listen(server_fd, 128)<0){
perror("listen");
close(server_fd);
return -1;
}
if(set_nonblocking(server_fd)<0){
perror("set_nonblocking server_fd");
close(server_fd);
return -1;
}
return server_fd;
}
static void remove_client(struct pollfd pfds[],nfds_t *nfds,nfds_t index){
int client_fd = pfds[index].fd;
printf("[server] closing client fd=%d\n", client_fd);
close(client_fd);
pfds[index] = pfds[*nfds - 1];
(*nfds)--;
}
static void accept_new_clients(int server_fd,struct pollfd pfds[],nfds_t *nfds){
while(running){
int client_fd = accept(server_fd, NULL, NULL);
if(client_fd<0){
if(errno == EINTR) {
if(!running){
return;
}
continue;
}
if(errno==EAGAIN||errno==EWOULDBLOCK) {
return;
}
perror("accept");
return;
}
if(*nfds>=MAX_CLIENTS+1){
printf("[server] maximum clients reached; rejecting fd=%d\n",client_fd);
close(client_fd);
continue;
}
if(set_nonblocking(client_fd)<0){
perror("set_nonblocking client_fd");
close(client_fd);
continue;
}
pfds[*nfds].fd = client_fd;
pfds[*nfds].events = POLLIN;
pfds[*nfds].revents = 0;
(*nfds)++;
printf("[server] client connected fd=%d; active=%lu\n",client_fd,(unsigned long)(*nfds - 1));
}
}
static int handle_client_read(int client_fd){
char buffer[BUFFER_SIZE];
size_t bytes_processed = 0;
const size_t READ_BUDGET = 64 * 1024;
while(running && bytes_processed < READ_BUDGET) {
size_t remaining_budget = READ_BUDGET - bytes_processed;
size_t read_size = sizeof(buffer);
if(read_size > remaining_budget) {
read_size = remaining_budget;
}
ssize_t n = recv(client_fd,buffer,read_size,0);
if(n>0){
bytes_processed += (size_t)n;
printf("[server] fd=%d received %zd bytes: ",client_fd, n);
if(fwrite(buffer, 1, (size_t)n, stdout) != (size_t)n) {
fprintf(stderr, "[server] stdout write failed\n");
}
if(fflush(stdout) == EOF){
fprintf(stderr, "[server] stdout flush failed\n");
}
continue;
}
if(n==0){
printf("[server] fd=%d disconnected (EOF)\n",client_fd);
return -1;
}
if(errno==EINTR){
if(!running){
return -1;
}
continue;
}
if(errno==EAGAIN || errno==EWOULDBLOCK){
return 0;
}
perror("recv");
return -1;
}
return running ? 0 : -1;
}
int main(void){
struct sigaction sa;
memset(&sa, 0, sizeof(sa));
sa.sa_handler = handle_sigint;
sigemptyset(&sa.sa_mask);
if(sigaction(SIGINT, &sa, NULL) < 0) {
perror("sigaction");
return 1;
}
int server_fd = create_server_socket();
if(server_fd<0){
return 1;
}
printf("[server] listening on port %d\n", PORT);
printf("[server] non-blocking + poll() enabled\n");
printf("[server] press Ctrl+C to stop\n");
struct pollfd pfds[MAX_CLIENTS + 1];
memset(pfds, 0, sizeof(pfds));
pfds[0].fd = server_fd;
pfds[0].events = POLLIN;
nfds_t nfds = 1;
while(running){
int result = poll(pfds, nfds, -1);
if(result<0){
if(errno==EINTR){
continue;
}
perror("poll");
break;
}
if(pfds[0].revents & POLLIN){
accept_new_clients(server_fd, pfds, &nfds);
}
if(!running){
break;
}
for(nfds_t i = 1; i < nfds; ) {
short revents = pfds[i].revents;
if(revents & POLLNVAL){
fprintf(stderr,"[server] invalid FD=%d (POLLNVAL)\n",pfds[i].fd);
remove_client(pfds, &nfds, i);
continue;
}
if(revents & POLLIN){
int read_result = handle_client_read(pfds[i].fd);
if(read_result < 0){
remove_client(pfds, &nfds, i);
continue;                }
}
if(revents & POLLERR){
int socket_error = 0;
socklen_t error_length = sizeof(socket_error);
if(getsockopt(pfds[i].fd,SOL_SOCKET,SO_ERROR,&socket_error,&error_length)<0){
perror("getsockopt SO_ERROR");
remove_client(pfds, &nfds, i);
continue;
}
if(socket_error != 0){
fprintf(stderr,"[server] fd=%d socket error: %s\n",pfds[i].fd,strerror(socket_error));
remove_client(pfds, &nfds, i);
continue;
}
}
if(revents & POLLHUP){
printf("[server] fd=%d hangup detected\n",pfds[i].fd);
remove_client(pfds, &nfds, i);
continue;
}
i++;
}
}
printf("[server] shutting down\n");
for(nfds_t i = 1; i < nfds; i++) {
close(pfds[i].fd);
}
close(server_fd);
printf("[server] cleanup complete\n");
return 0;
}
