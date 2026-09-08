#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include "binary_protocol.h"
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
//char server_message[256] = {"you have connected to server "};
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
client_socket = accept(server_socket,NULL,NULL); // this accept() function does the 3-way handshake.
if (client_socket < 0){
printf("Client socket is negative, couldn't accept");
exit(EXIT_FAILURE);
}
size_t header_size = 12;
size_t max_buffer_size = header_size + MAX_ALLOWED_PAYLOAD;
uint8_t receive_buffer[max_buffer_size];
uint8_t response_buffer[max_buffer_size];
size_t byte_receive = 0;
//bool byte_receive_complete = false;
uint16_t payload_length = 0;
size_t frame_size = 0;
//size_t byte_consumed = 0;
while(1){
ssize_t recv_byte = recv(client_socket,receive_buffer+byte_receive,sizeof(receive_buffer)-byte_receive,0);
if(recv_byte<0){
printf("error in recv byte!");
exit(EXIT_FAILURE);
}else if(recv_byte==0){printf("client disconnected!"); break;}
byte_receive += (size_t)recv_byte;

while(byte_receive>=(size_t)header_size){
payload_length = ((receive_buffer[2]<<8)|receive_buffer[3]);
if(payload_length>MAX_ALLOWED_PAYLOAD){
printf("invalid payload length");
close(client_socket);
close(server_socket);
exit(EXIT_FAILURE);
}
frame_size = header_size + payload_length;
if(byte_receive<frame_size){
break;
}
struct message decoder;
bool decoder_flag;
decoder_flag = deserialize(receive_buffer,frame_size,&decoder);
if(decoder_flag)printf("deserilize succes!");
else{ printf("deserilize failed!"); exit(EXIT_FAILURE);}
size_t remaining_byte = byte_receive - frame_size;
if(decoder.header.type==ECHO){
struct message response;
bool response_message_init = message_init(&response,ECHO,decoder.header.sequence,(uint8_t*)decoder.payload,decoder.header.payload_length);
if(!response_message_init){
printf("response message init fail!");
exit(EXIT_FAILURE);
}
size_t response_serialization = serialize(&response,response_buffer,sizeof(response_buffer));
if(response_serialization<(response.header.payload_length+12)){
printf("response serialization failed !");
exit(EXIT_FAILURE);
}
bool response_send_all = Send_all(client_socket,response_buffer,response_serialization);
if(!response_send_all){
printf("response send_all message failed!");
exit(EXIT_FAILURE);
}
}
memmove(receive_buffer,receive_buffer+frame_size,remaining_byte);
byte_receive = remaining_byte;
}
}
close(server_socket);
close(client_socket); 
return 0; 
}
