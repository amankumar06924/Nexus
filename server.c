#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

int main(){
char server_message[256] = "You have reached the server";
int server_socket;
server_socket = socket(AF_INET, SOCK_STREAM, 0);
if (server_socket < 0){
printf("Creating the socket failed (%d)\n\n", server_socket);
exit(EXIT_FAILURE);
}
struct sockaddr_in server_address;
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
send(client_socket, server_message, sizeof(server_message), 0); 
close(server_socket);
close(client_socket); 
return 0; 
}
