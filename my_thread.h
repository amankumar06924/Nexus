#ifndef MY_THREAD_H
#define MY_THREAD_H

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_CLIENTS 10

#define CLIENT_FREE      0
#define CLIENT_CONNECTED 1
#define CLIENT_CLOSING   2

#define CLIENT_OK             0
#define CLIENT_ERR_FULL      -1
#define CLIENT_ERR_DUPLICATE -2
#define CLIENT_ERR_INVALID   -3
#define CLIENT_ERR_NOT_FOUND -4
#define CLIENT_ERR_INVALID_FD -5
#define CLIENT_ERR_INVALID_ID  -6

typedef struct {
int client_fd;
int client_id;
int active;
int client_state;
} client;

struct data {
int client_fd;
int client_id;
};

extern client clients[MAX_CLIENTS];
extern pthread_mutex_t clients_lock;

int add_client(int client_fd, int client_id);
int remove_client(int client_id);
void *worker(void *arg);
int count_active_clients(void);
int find_client(int client_id, client *out);
int find_connected_client(int client_id, client *out);
int update_client_fd(int client_id, int client_new_fd);
int get_client_fd(int client_id, int *out_fd);
int mark_client_closing(int client_id);
int disconnect_client(int client_id);
const char *client_state_name(int state);
int client_manager_init(void);
int client_manager_destroy(void);
void print_clients(void);
int get_client_snapshot(int index,client *out_client);
int connect_client(int client_fd,int client_id);
int reconnect_client(int client_fd,int client_id);
int count_connected_clients(void);
int count_closing_clients(void);
int count_free_slots(void);
int client_manager_reset(void);
int client_manager_validate(void);
int client_manager_mark_all_closing(void);
int client_manager_disconnect_all(void);
#endif
