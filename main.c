#include<signal.h>
#include<stdio.h>
#include<stdlib.h>
#include"my_thread.h"
volatile sig_atomic_t shutdown_requested = 0;
void handle_signal(int signal){
(void)signal;
shutdown_requested = 1;
//printf("\n server shutdown signal receives\n");
//client_manager_mark_all_closing();
//client_manager_disconnect_all();
//printf("servera all clients disconnected\n");
//exit(0);
}

int main(){
  if(client_manager_init()!=CLIENT_OK){
printf("manager init failed\n");
return 1;
  }
signal(SIGINT , handle_signal);
printf("server running... \n");
while(!shutdown_requested){
sleep(1);
}
printf("server shutdown ...");
int close = client_manager_mark_all_closing();
int dis = client_manager_disconnect_all();
printf("client marked clse %d\n",close);
printf("client dis %d\n",dis);
if(client_manager_destroy()!=CLIENT_OK){
printf("manager destroy failed\n");
}
return 0;
}
