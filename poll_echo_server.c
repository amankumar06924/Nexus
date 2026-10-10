#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <poll.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define PORT 9009
#define MAX_CLIENTS 10
#define BUFFER_SIZE 4096
#define OUTPUT_CAPACITY (64 * 1024)
#define IO_BUDGET (64 * 1024)

static volatile sig_atomic_t running = 1;
typedef struct{
int fd;
char output[OUTPUT_CAPACITY];
size_t out_start;
size_t out_len;
int peer_closed;
}Client;
static Client clients[MAX_CLIENTS];
static void handle_sigint(int signo){
(void)signo;
running = 0;
}
static int set_nonblocking(int fd){
int flags = fcntl(fd, F_GETFL, 0);
if(flags<0)
return -1;
return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
static int create_listener(void){
int fd = socket(AF_INET, SOCK_STREAM, 0);
if(fd<0){
perror("socket");
return -1;
}
int opt = 1;
if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,&opt, sizeof(opt)) < 0){
perror("setsockopt");
close(fd);
return -1;
}
struct sockaddr_in addr ={
.sin_family = AF_INET,
.sin_port = htons(PORT),
.sin_addr.s_addr = htonl(INADDR_ANY)
};
if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
perror("bind");
close(fd);
return -1;
}
if(listen(fd, 128)<0){
perror("listen");
close(fd);
return -1;
}
if(set_nonblocking(fd)<0){
perror("set_nonblocking listener");
close(fd);
return -1;
}
return fd;
}
static void close_client(Client *c){
if(c->fd >= 0){
printf("[server] closing fd=%d\n", c->fd);
close(c->fd);
}
c->fd = -1;
c->out_start = 0;
c->out_len = 0;
c->peer_closed = 0;
}
static size_t output_space(Client *c){
if(c->out_start + c->out_len + 1 > OUTPUT_CAPACITY &&c->out_start > 0){
memmove(c->output,c->output + c->out_start,c->out_len);
c->out_start = 0;
}
return OUTPUT_CAPACITY - (c->out_start + c->out_len);
}
static int queue_output(Client *c,const char *data,size_t length){
if(length > output_space(c))
return -1;
memcpy(c->output + c->out_start + c->out_len,data, length);
c->out_len += length;
return 0;
}

static void accept_clients(int listener){
while(running){
int fd = accept(listener, NULL, NULL);
if(fd<0){
if(errno == EINTR){
if(!running)
return;
continue;
}
if(errno == EAGAIN || errno == EWOULDBLOCK)
return;
perror("accept");
return;
}
if(set_nonblocking(fd)<0){
perror("set_nonblocking client");
close(fd);
continue;
}
int slot = -1;
for(int i = 0; i < MAX_CLIENTS; i++){
if(clients[i].fd < 0){
slot = i;
break;
}
}
if(slot<0){
printf("[server] client limit reached\n");
close(fd);
continue;
}
clients[slot].fd = fd;
clients[slot].out_start = 0;
clients[slot].out_len = 0;
clients[slot].peer_closed = 0;
printf("[server] client connected fd=%d slot=%d\n",fd, slot);
}
}

static int read_client(Client *c){
char buffer[BUFFER_SIZE];
size_t total = 0;
while(total < IO_BUDGET){
size_t space = output_space(c);
if(space == 0)
return 0;
size_t amount = sizeof(buffer);
if(amount > space)
amount = space;
if(amount > IO_BUDGET - total)
amount = IO_BUDGET - total;
ssize_t n = recv(c->fd, buffer, amount, 0);
if(n>0){
if(queue_output(c, buffer, (size_t)n) < 0)
return -1;
total += (size_t)n;
continue;
}
if(n == 0){
c->peer_closed = 1;
return 0;
}
if(errno == EINTR){
if(!running)
return -1;
continue;
}
if(errno == EAGAIN || errno == EWOULDBLOCK)
return 0;
perror("recv");
return -1;
}
return 0;
}

static int write_client(Client *c){
size_t total = 0;
while(c->out_len > 0 && total < IO_BUDGET) {
size_t amount = c->out_len;
if(amount > IO_BUDGET - total)
amount = IO_BUDGET - total;
ssize_t n = send(c->fd,c->output + c->out_start,amount,MSG_NOSIGNAL);
if(n>0){
c->out_start += (size_t)n;
c->out_len -= (size_t)n;
total += (size_t)n;
if(c->out_len == 0)
c->out_start = 0;
continue;
}
if(n < 0 && errno == EINTR){
if(!running)
return -1;
continue;
}
if(n<0&&(errno == EAGAIN || errno == EWOULDBLOCK)){
return 0;
}
if(n == 0){
return 0;
}
perror("send");
return -1;
}
return 0;
}

int main(void){
struct sigaction sa;
memset(&sa, 0, sizeof(sa));
sa.sa_handler = handle_sigint;
sigemptyset(&sa.sa_mask);
if(sigaction(SIGINT, &sa, NULL)<0){
perror("sigaction");
return 1;
}
for(int i = 0; i < MAX_CLIENTS; i++)
clients[i].fd = -1;
int listener = create_listener();
if(listener<0)
return 1;
printf("[server] listening on %d\n", PORT);
printf("[server] non-blocking echo server\n");
while(running){
struct pollfd pfds[MAX_CLIENTS + 1];
pfds[0].fd = listener;
pfds[0].events = POLLIN;
pfds[0].revents = 0;
for(int i = 0; i < MAX_CLIENTS; i++) {
Client *c = &clients[i];
pfds[i + 1].fd = c->fd;
pfds[i + 1].events = 0;
pfds[i + 1].revents = 0;
if(c->fd<0)
continue;
if(!c->peer_closed && output_space(c) > 0)
pfds[i + 1].events |= POLLIN;
if(c->out_len>0)
pfds[i + 1].events |= POLLOUT;
}
int result = poll(pfds, MAX_CLIENTS + 1, -1);
if(result<0){
if(errno == EINTR)
continue;
perror("poll");
break;
}
if(pfds[0].revents & POLLIN)
  accept_clients(listener);
for(int i = 0; i < MAX_CLIENTS; i++){
Client *c = &clients[i];
if(c->fd<0)
continue;
short events = pfds[i + 1].revents;
int failed = 0;
if (events & POLLNVAL) {
close_client(c);
continue;
}
if(events & POLLIN){
if(read_client(c) < 0)
failed = 1;
}
if(!failed && (events & POLLOUT)){
if(write_client(c) < 0)
failed = 1;
}
if(!failed && (events & POLLERR)) {
int error = 0;
socklen_t len = sizeof(error);
if(getsockopt(c->fd, SOL_SOCKET, SO_ERROR,&error, &len) < 0 || error != 0){
if(error != 0)
fprintf(stderr, "[server] socket error: %s\n",strerror(error));
failed = 1;
}
}
if(failed){
close_client(c);
continue;
}
if(events & POLLHUP)
c->peer_closed = 1;
if(c->peer_closed && c->out_len == 0){
close_client(c);
continue;
}
}
}
printf("[server] shutting down\n");
for(int i = 0; i < MAX_CLIENTS; i++)
close_client(&clients[i]);
close(listener);
printf("[server] cleanup complete\n");
return 0;
}
