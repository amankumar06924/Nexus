#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include "binary_protocol.h"
#include "my_thread.h"
#include <string.h>
#include <signal.h>
#include <errno.h>
#define HEADER_SIZE 12
volatile sig_atomic_t shutdown_request = 0;
void handle_sigint(int sig){
(void)sig;
shutdown_request = 1;
}
int main(){
struct sigaction sa = {0};
sa.sa_handler = handle_sigint;
sigemptyset(&sa.sa_mask);
sa.sa_flags = 0;
if(sigaction(SIGINT,&sa,NULL)==-1){
perror("sigaction failed\n");
return EXIT_FAILURE;
}
if(client_manager_init()!=CLIENT_OK){
fprintf(stderr,"client manager initialization failed!\n");
return EXIT_FAILURE;
}
int server_socket;
server_socket = socket(AF_INET, SOCK_STREAM, 0);
if (server_socket < 0){
printf("Creating the socket failed (%d)\n\n", server_socket);
exit(EXIT_FAILURE);
}
struct sockaddr_in server_address = {0};
server_address.sin_family = AF_INET;
server_address.sin_port = htons(9003);
server_address.sin_addr.s_addr = INADDR_ANY; // inet_addr("127.0.01")
if (bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) < 0){
printf("Binding the socket failed!\n\n");
exit(EXIT_FAILURE);
}
if (listen(server_socket, 5) < 0){ // 5 mean it max 5 pending connection.
printf("Socket listening failed\n\n");
exit(EXIT_FAILURE);
}
int client_socket;
int next_client_id = 1;
while(!shutdown_request){
  int reaped = client_manager_reap_finished_workers();
  if(reaped>0){
printf("[manager] reaped %d finished workers\n",reaped);
  }
client_socket = accept(server_socket,NULL,NULL); // this accept() function does the 3-way handshake.
if (client_socket < 0){
  if(errno==EINTR){
    if(shutdown_request){break;}
    continue;
  }
perror("accept failed!");
continue;
}
struct data *client_data = malloc(sizeof(struct data));
if(client_data==NULL){
perror("malloc failed!"); // remove \n because perror() function khud newline add karte hai.
close(client_socket);
continue;
}
client_data->client_fd = client_socket;
client_data->client_id = next_client_id++;
pthread_mutex_init(&client_data->start_mutex, NULL);
pthread_cond_init(&client_data->start_cond, NULL);
client_data->start = 0;
int assigned_id = client_data->client_id;
int assigned_fd = client_data->client_fd;
pthread_t thread;
int check_reserve_client_slot = reserve_client_slot(assigned_fd,assigned_id);
if(check_reserve_client_slot!=CLIENT_OK){
printf("[server] reserve client slot failed\n");
close(client_socket);
free(client_data);
continue;
}
int result = pthread_create(&thread,NULL,worker,client_data);
if(result!=0){
fprintf(stderr,"pthread_create failed\n");
release_reserved_client_slot(assigned_id);
pthread_cond_destroy(&client_data->start_cond);
pthread_mutex_destroy(&client_data->start_mutex);
close(client_socket);
free(client_data);
continue;
}
int check_attach_client_thread =  attach_client_thread(assigned_id,thread,client_data);
if(check_attach_client_thread!=CLIENT_OK){
close(client_socket);
free(client_data);
continue;
}
//int add_client_check = add_client(assigned_fd,assigned_id,thread);
//if(add_client_check!=CLIENT_OK){
//fprintf(stderr,"[Server] failed to register client: fd=%d | id=%d | result=%d\n",assigned_fd,assigned_id,add_client_check);
//close(client_socket);
//free(client_data);
//continue;
//}
//pthread_detach(thread);
printf("[server] new clinet accepted id=%d fd=%d\n",assigned_id,assigned_fd);
}
printf("[server] shutdown requested!\n");
int disconnected_client = client_manager_disconnect_all();
printf("[server] shutdown signal sent to %d clients\n",disconnected_client);
int joined_workers = client_manager_join_all_workers();
printf("[server] %d worker threads joined\n",joined_workers);
//int marked_client = client_manager_mark_all_closing();
//printf("[server] %d client marked closing\n",marked_client);
close(server_socket); 
return 0; 
}
