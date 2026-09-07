#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include"binary_protocol.h"
#include<string.h>
bool Send_all(int socket,const uint8_t *buffer,size_t len){
size_t total_len = 0;
while(total_len<len){
ssize_t send_message = send(socket,buffer+total_len,len-total_len,0);
if(send_message<0){
return false;
}
if(send_message==0){
return false;
}
total_len += send_message;
}
return true;
}
int main(){
int network_socket;
network_socket = socket(AF_INET, SOCK_STREAM, 0);
if (network_socket < 0){
printf("Creating the socket failed (%d)\n\n", network_socket);
exit(EXIT_FAILURE);
}
struct sockaddr_in server_address={0};
server_address.sin_family = AF_INET;
server_address.sin_port = htons(9003);
server_address.sin_addr.s_addr = INADDR_ANY;
int connection_status = connect(network_socket, (struct sockaddr*) &server_address, sizeof(server_address));
if (connection_status < 0){
printf ("There was an error making a connection to the server (%d) \n\n", connection_status); 
exit(EXIT_FAILURE);
}
char payload[5] = "hello";
size_t max_buffer_size = 12 + MAX_ALLOWED_PAYLOAD;
uint8_t buffer[max_buffer_size];
struct message message;
bool message_init_check = message_init(&message,ECHO,1,(const uint8_t *)payload,sizeof(payload));
if(message_init_check){
printf("initializing message done!");
}else{ printf("initalizing message failed"); exit(EXIT_FAILURE);}
size_t serialization = serialize(&message,buffer,sizeof(buffer));
if(serialization<(message.header.payload_length + 12)){
printf("failed serialization!");
exit(EXIT_FAILURE);
}
bool send_all_message_flag = Send_all(network_socket,buffer,serialization);

if(!send_all_message_flag){
printf("send message failed!");
exit(EXIT_FAILURE);
}


//char server_response[256];
//recv(network_socket, &server_response, sizeof(server_response), 0);
//printf("The server sent the data : %s\n", server_response);
close(network_socket);
return 0;
}
