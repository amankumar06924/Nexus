#ifndef MY_THREAD_H
#define MY_THREAD_H

#include<stdio.h>
#include<unistd.h>
#include<pthread.h>
struct worker_arg{
int client_fd;
int client_id;
};
void *worker(void *arg);

#endif
