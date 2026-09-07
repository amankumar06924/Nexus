#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

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


char server_response[256];
recv(network_socket, &server_response, sizeof(server_response), 0);
printf("The server sent the data : %s\n", server_response);
close(network_socket);
return 0;
}
