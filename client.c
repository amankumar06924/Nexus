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
bool send_fragmented_message(int socket,size_t serialization,const uint8_t *buffer);
bool send_echo_message(int socket,uint64_t sequence,const uint8_t *payload,size_t payload_length){
size_t max_buffer_size = 12 + MAX_ALLOWED_PAYLOAD;
uint8_t buffer[max_buffer_size];
struct message message;
bool message_init_check =message_init(&message,ECHO,sequence,payload,payload_length);
if (!message_init_check) {
printf("message initialization failed!\n");
return false;
}
size_t serialization =
serialize(&message,buffer,sizeof(buffer));
if (serialization == 0) {
printf("message serialization failed!\n");
return false;
}
//bool send_all_message_flag =Send_all(socket,buffer,serialization);
bool send_all_message_flag = send_fragmented_message(socket,serialization,buffer);
if (!send_all_message_flag) {
printf("send_all failed!\n");
return false;
}
return true;
}
bool send_fragmented_message(int socket,size_t serialization,const uint8_t *buffer){
size_t offset = 0;
while(offset<serialization){
size_t chunk_size = 3;
if(serialization-offset < chunk_size){
chunk_size = serialization - offset;
}
ssize_t fragmented_send_message = send(socket,buffer+offset,chunk_size,0);
if(fragmented_send_message<0 || fragmented_send_message==0){
return false;
}
offset+=chunk_size;
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
server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
int connection_status = connect(network_socket, (struct sockaddr*) &server_address, sizeof(server_address));
if (connection_status < 0){
printf ("There was an error making a connection to the server (%d) \n\n", connection_status); 
exit(EXIT_FAILURE);
}
const uint8_t payload1[] = "hello";
const uint8_t payload2[] = "world";
const uint8_t payload3[] = "low latency";
const uint8_t payload4[] = "binary protocol";
send_echo_message(network_socket,1,payload1,sizeof(payload1) - 1);
send_echo_message(network_socket,2,payload2,sizeof(payload2) - 1);
send_echo_message(network_socket,3,payload3,sizeof(payload3) - 1);
//send_echo_message(network_socket,4,payload4,sizeof(payload4) - 1);

size_t max_buffer_size = 12+MAX_ALLOWED_PAYLOAD;
size_t header_size = 12;
uint8_t receive_buffer[max_buffer_size];
size_t byte_receive = 0;
struct message message;
uint8_t expected_sequence = 1;
while (1) {
ssize_t recv_byte = recv(network_socket,receive_buffer + byte_receive,sizeof(receive_buffer) - byte_receive,0);
if (recv_byte < 0) {
printf("error in recv byte!\n");
exit(EXIT_FAILURE);
}
if (recv_byte == 0) {
printf("server disconnected!\n");
break;
}
byte_receive += (size_t)recv_byte;
while (byte_receive >= header_size) {
uint16_t payload_length =((uint16_t)receive_buffer[2] << 8) | (uint16_t)receive_buffer[3];
if (payload_length > MAX_ALLOWED_PAYLOAD) {
printf("invalid payload length!\n");
close(network_socket);
exit(EXIT_FAILURE);
}
size_t frame_size = header_size + payload_length;
if (byte_receive < frame_size) {
break;
}
struct message decoder;
bool decoder_flag =deserialize(receive_buffer,frame_size,&decoder);
if (!decoder_flag) {
printf("deserialize failed!\n");
close(network_socket);
exit(EXIT_FAILURE);
}
printf("deserialize success!\n");
if (decoder.header.type == ECHO) {
printf("Server response: ");
for (size_t i = 0;i < decoder.header.payload_length;i++) {
printf("%c", decoder.payload[i]);
}
printf("\n");
if (decoder.header.sequence == expected_sequence) {
printf("sequence verified!\n");
expected_sequence++;
} else {
printf("sequence mismatch!\n");
}
}
size_t remaining_byte =byte_receive - frame_size;
memmove(receive_buffer,receive_buffer + frame_size,remaining_byte);byte_receive = remaining_byte;}
}
close(network_socket);
return 0;
}
