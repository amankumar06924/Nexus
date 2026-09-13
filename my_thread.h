#ifndef MY_THREAD_H
#define MY_THREAD_H

#include<stdio.h>
#include<unistd.h>
#include<pthread.h>
#define MAX_CLIENTS 10
typedef struct{
int client_fd;
int client_id;
int active;
}client;
struct data{
int client_fd;
int client_id;
};
client clients[MAX_CLIENTS];
pthread_mutex_t clients_lock = PTHREAD_MUTEX_INITIALIZER;
int add_client(int client_fd,int client_id);
int remove_client(int client_id);

#endif
