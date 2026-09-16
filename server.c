#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include "binary_protocol.h"
#include "my_thread.h"
#include<string.h>
#define HEADER_SIZE 12

int main(){
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
while(1){
client_socket = accept(server_socket,NULL,NULL); // this accept() function does the 3-way handshake.
if (client_socket < 0){
perror("accept failed\n");
continue;
}
struct data *client_data = malloc(sizeof(struct data));
if(client_data==NULL){
perror("malloc failed!\n");
close(client_socket);
continue;
}
client_data->client_fd = client_socket;
client_data->client_id = next_client_id++;
int assigned_id = client_data->client_id;
int assigned_fd = client_data->client_fd;
pthread_t thread;
int result = pthread_create(&thread,NULL,worker,client_data);
if(result!=0){
fprintf(stderr,"pthread_create failed\n");
close(client_socket);
free(client_data);
continue;
}
pthread_detach(thread);
printf("[server] new clinet accepted id=%d fd=%d\n",assigned_id,assigned_fd);
}
close(server_socket); 
return 0; 
}
