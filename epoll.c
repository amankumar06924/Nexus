#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 9009
#define MAX_CLIENTS 10
#define MAX_EVENTS 64
#define BUFFER_SIZE 4096

static volatile sig_atomic_t running = 1;
enum client_state{
CLIENT_FREE,
CLIENT_ACTIVE,
CLIENT_CLOSING
};
struct client{
int fd;
char output[BUFFER_SIZE];
size_t output_len;
size_t output_sent;
enum client_state state;
int peer_eof;
};

static void handle_sigint(int signal_number){
(void)signal_number;
running = 0;
}
static int set_nonblocking(int fd){
int flags = fcntl(fd, F_GETFL, 0);
if (flags == -1){
return -1;
}
if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1){
return -1;
}
return 0;
}
static int create_server(void){
int server_fd = socket(AF_INET, SOCK_STREAM, 0);
if (server_fd == -1){
perror("socket");
return -1;
}
int opt = 1;
if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,&opt, sizeof(opt)) == -1){
perror("setsockopt");
close(server_fd);
return -1;
}
struct sockaddr_in address = {
.sin_family = AF_INET,
.sin_port = htons(PORT),
.sin_addr.s_addr = htonl(INADDR_ANY)};
if(bind(server_fd, (struct sockaddr *)&address,sizeof(address)) == -1){
perror("bind");
close(server_fd);
return -1;
}
if(listen(server_fd, 128) == -1){
perror("listen");
close(server_fd);
return -1;
}
if(set_nonblocking(server_fd) == -1){
perror("set_nonblocking listener");
close(server_fd);
return -1;
}
return server_fd;
}
static int register_fd(int epoll_fd, int fd, uint32_t events){
struct epoll_event event = {0};
event.events = events;
event.data.fd = fd;
return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
}
static void remove_client(int epoll_fd, struct client *clients,int client_index){
if(clients[client_index].state==CLIENT_FREE){
return;
}
int fd = clients[client_index].fd;
if(fd == -1){
return;
}
clients[client_index].state = CLIENT_CLOSING;
(void)epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
close(fd);
printf("[server] client disconnected fd=%d\n", fd);
clients[client_index].fd = -1;
clients[client_index].output_len = 0;
clients[client_index].output_sent = 0;
clients[client_index].state = CLIENT_FREE;
}
static int find_client(struct client *clients, int fd){
for(int i = 0; i < MAX_CLIENTS; i++){
if(clients[i].fd == fd && clients[i].state==CLIENT_ACTIVE){
return i;
}
}
return -1;
}
static int find_free_slot(struct client *clients){
for(int i = 0; i < MAX_CLIENTS; i++){
if(clients[i].fd == -1){
return i;
}
}
return -1;
}

static void accept_new_clients(int epoll_fd, int server_fd,struct client *clients){
while (running){
int client_fd = accept(server_fd, NULL, NULL);
if(client_fd == -1){
  if(errno == EINTR){
if(!running){
return;
}
continue;
}
if(errno == EAGAIN || errno == EWOULDBLOCK){
return;
}
perror("accept");
return;
}
if(set_nonblocking(client_fd) == -1){
perror("set_nonblocking client");
close(client_fd);
continue;
}
int slot = find_free_slot(clients);
if(slot == -1){
printf("[server] client limit reached; rejecting fd=%d\n",client_fd);
close(client_fd);
continue;
}
if(register_fd(epoll_fd, client_fd, EPOLLIN | EPOLLRDHUP ) == -1){
perror("epoll_ctl ADD client");
close(client_fd);
continue;
}
clients[slot].fd = client_fd;
clients[slot].output_len = 0;
clients[slot].output_sent = 0;
clients[slot].state = CLIENT_ACTIVE;
clients[slot].peer_eof = 0;
printf("[server] client connected fd=%d\n", client_fd);
}
}
static int handle_client_read(int epoll_fd, struct client *clients,int slot){
struct client *client = &clients[slot];
char buffer[BUFFER_SIZE];
if(client->output_len != client->output_sent) return 0;
for (;;){
ssize_t bytes = recv(client->fd, buffer, sizeof(buffer), 0);
if(bytes > 0){
memcpy(client->output, buffer, (size_t)bytes);
client->output_len = (size_t)bytes;
client->output_sent = 0;
struct epoll_event event = {0};
event.events = EPOLLOUT;
event.data.fd = client->fd;
if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD,client->fd, &event) == -1){
perror("epoll_ctl MOD client");
return -1;
}
return 0;
}
if(bytes == 0){
client->peer_eof = 1;
if(client->output_len == client->output_sent) return -1;
return 0;
}
if(errno == EINTR){
continue;
}
if(errno == EAGAIN || errno == EWOULDBLOCK){
return 0;
}
perror("recv");
return -1;
}
}
static int handle_client_write(int epoll_fd, struct client *clients,int slot){
struct client *client = &clients[slot];
while(client->output_sent < client->output_len){
ssize_t bytes = send(client->fd,client->output + client->output_sent,client->output_len - client->output_sent,MSG_NOSIGNAL);
if (bytes > 0){
client->output_sent += (size_t)bytes;
continue;
}
if (bytes == -1 && errno == EINTR){
continue;
}
if(bytes == -1 &&(errno == EAGAIN || errno == EWOULDBLOCK)){
fprintf(stderr,"[backpressure] send would block: fd=%d pending=%zu\n",client->fd,client->output_len - client->output_sent);
return 0;
}
if(bytes == 0){
return -1;
}
perror("send");
return -1;
}
client->output_len = 0;
client->output_sent = 0;
if(client->peer_eof) return -1;
struct epoll_event event = {0};
event.events = EPOLLIN | EPOLLRDHUP;
event.data.fd = client->fd;
if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD,client->fd, &event) == -1){
perror("epoll_ctl MOD client");
return -1;
}
return 0;
}

int main(void){
struct sigaction sa = {0};
sa.sa_handler = handle_sigint;
sigemptyset(&sa.sa_mask);
if(sigaction(SIGINT, &sa, NULL) == -1){
perror("sigaction");
return 1;
}
int server_fd = create_server();
if(server_fd == -1){
return 1;
}
int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
if(epoll_fd == -1){
perror("epoll_create1");
close(server_fd);
return 1;
}
if(register_fd(epoll_fd, server_fd, EPOLLIN) == -1){
perror("epoll_ctl ADD listener");
close(epoll_fd);
close(server_fd);
return 1;
}
struct client clients[MAX_CLIENTS];
for (int i = 0; i < MAX_CLIENTS; i++){
clients[i].fd = -1;
clients[i].output_len = 0;
clients[i].output_sent = 0;
clients[i].state = CLIENT_FREE;
clients[i].peer_eof = 0;
}
struct epoll_event ready_events[MAX_EVENTS];
printf("[server] Nexus epoll server listening on port %d\n", PORT);
printf("[server] press Ctrl+C to stop\n");
while (running){
int n = epoll_wait(epoll_fd, ready_events, MAX_EVENTS, -1);
if (n == -1){
if (errno == EINTR){continue;}
perror("epoll_wait");
break;
}
for(int i = 0; i < n; i++){
int fd = ready_events[i].data.fd;
uint32_t flags = ready_events[i].events;
if(fd == server_fd){
if(flags & EPOLLIN){
accept_new_clients(epoll_fd, server_fd, clients);
}
continue;
}
int slot = find_client(clients, fd);
if(slot == -1){
continue;
}
int failed = 0;
if(flags & EPOLLERR){
int socket_error = 0;
socklen_t error_len = sizeof(socket_error);
if (getsockopt(fd, SOL_SOCKET, SO_ERROR,&socket_error, &error_len) == -1){
failed = 1;
perror("getsockopt SO_ERROR");
}
else if (socket_error != 0){
failed = 1;
fprintf(stderr, "[server] socket error fd=%d: %s\n",fd, strerror(socket_error));
}
}
//if(flags & EPOLLHUP){
//failed = 1;
//}
if(!failed && (flags & (EPOLLIN | EPOLLRDHUP | EPOLLHUP))){
if(handle_client_read(epoll_fd, clients, slot) == -1){
failed = 1;
}
}
if(!failed && (flags & EPOLLOUT)){
if(handle_client_write(epoll_fd, clients, slot) == -1){
failed = 1;
}
}
if(failed){
remove_client(epoll_fd, clients, slot);
}
}
}
for (int i = 0; i < MAX_CLIENTS; i++){
remove_client(epoll_fd, clients, i);
}
close(server_fd);
close(epoll_fd);
printf("[server] shutdown complete\n");
return 0;
}
