#include"my_thread.h"
client clients[MAX_CLIENTS];
pthread_mutex_t clients_lock;

int add_client(int client_fd,int client_id){
if(client_id<0 || client_fd<0){
return CLIENT_ERR_INVALID;
}
pthread_mutex_lock(&clients_lock);
for(int i=0;i<MAX_CLIENTS;i++){
if(clients[i].active && clients[i].client_id==client_id){
pthread_mutex_unlock(&clients_lock);
return CLIENT_ERR_DUPLICATE;
}
}
for(int i=0;i<MAX_CLIENTS;i++){
if(clients[i].active==0){
clients[i].client_fd = client_fd;
clients[i].client_id = client_id;
clients[i].active = 1;
clients[i].client_state = CLIENT_CONNECTED;
pthread_mutex_unlock(&clients_lock);
return 0;
}
}
pthread_mutex_unlock(&clients_lock);
return CLIENT_ERR_FULL;
}

int remove_client(int client_id){
if(client_id<0){return CLIENT_ERR_INVALID;}
pthread_mutex_lock(&clients_lock);
for(int i =0;i<MAX_CLIENTS;i++){
if(clients[i].client_id==client_id && clients[i].active==1){
  if(clients[i].client_state!=CLIENT_CLOSING){
pthread_mutex_unlock(&clients_lock);
return CLIENT_ERR_NOT_FOUND;
  }
  clients[i].active = 0;
  clients[i].client_id  = -1;
  clients[i].client_fd  = -1;
  clients[i].client_state = CLIENT_FREE;
  pthread_mutex_unlock(&clients_lock);
  return CLIENT_OK;
}
}
pthread_mutex_unlock(&clients_lock);
return CLIENT_ERR_NOT_FOUND;
}

void *worker(void *arg){
struct data *data = arg;
int add_client_check = add_client(data->client_fd,data->client_id);
if(add_client_check!=0){
printf("add client failed! for %d , %d",data->client_fd,data->client_id);
}
return NULL;
}

int count_active_clients(void){
int count = 0;
pthread_mutex_lock(&clients_lock);
for(int i =0;i<MAX_CLIENTS;i++){
if(clients[i].active){
count++;
}
}
pthread_mutex_unlock(&clients_lock);
return count;
}

int find_client(int client_id,client *out){
  if(out==NULL){
return -1;
  }
pthread_mutex_lock(&clients_lock);
for(int i =0;i<MAX_CLIENTS;i++){
if(clients[i].client_id==client_id && clients[i].active){
*out = clients[i];
pthread_mutex_unlock(&clients_lock);
return 0;
}
}
pthread_mutex_unlock(&clients_lock);
return -1;
}

int update_client_fd(int client_id,int client_new_fd){
  if(client_id<0 || client_new_fd<0){return CLIENT_ERR_INVALID;}
pthread_mutex_lock(&clients_lock);
for(int i=0;i<MAX_CLIENTS;i++){
if(clients[i].client_id==client_id && clients[i].active){
  if(clients[i].client_state!=CLIENT_CONNECTED){
pthread_mutex_unlock(&clients_lock);
return CLIENT_ERR_NOT_FOUND;
  }
clients[i].client_fd = client_new_fd;
pthread_mutex_unlock(&clients_lock);
return CLIENT_OK;
}
}
pthread_mutex_unlock(&clients_lock);
return CLIENT_ERR_NOT_FOUND;
}

const char *client_state_name(int state){
switch(state){
  case CLIENT_FREE:
    return "FREE";
  case CLIENT_CONNECTED:
    return "CONNECTED";
  case CLIENT_CLOSING:
    return "CLOSING";
  default:
    return "UNKNOWN";
}
}

int mark_client_closing(int client_id){
pthread_mutex_lock(&clients_lock);
for(int i =0;i<MAX_CLIENTS;i++){
if(clients[i].active && clients[i].client_id==client_id){
clients[i].client_state = CLIENT_CLOSING;
pthread_mutex_unlock(&clients_lock);
return 0;
}
}
pthread_mutex_unlock(&clients_lock);
return -1;
}

int get_client_fd(int client_id,int *out_fd){
if(out_fd==NULL){return -1;}
pthread_mutex_lock(&clients_lock);
for(int i =0;i<MAX_CLIENTS;i++){
if(clients[i].active && clients[i].client_id == client_id){
*out_fd = clients[i].client_fd;
pthread_mutex_unlock(&clients_lock);
return 0;
}
}
pthread_mutex_unlock(&clients_lock);
return -1;
}

int find_connected_client(int client_id,client *out){
pthread_mutex_lock(&clients_lock);
for(int i =0;i<MAX_CLIENTS;i++){
if(clients[i].active && clients[i].client_id==client_id && clients[i].client_state == CLIENT_CONNECTED){
*out = clients[i];
pthread_mutex_unlock(&clients_lock);
return 0;
}
}
pthread_mutex_unlock(&clients_lock);
return -1;
}

int disconnect_client(int client_id) {
if(client_id<0){return CLIENT_ERR_INVALID;}
pthread_mutex_lock(&clients_lock);
for (int i = 0; i < MAX_CLIENTS; i++) {
if (clients[i].active == 1 &&
clients[i].client_id == client_id) {
clients[i].client_state = CLIENT_CLOSING;
clients[i].active = 0;
clients[i].client_id = -1;
clients[i].client_fd = -1;
clients[i].client_state = CLIENT_FREE;
pthread_mutex_unlock(&clients_lock);
return CLIENT_OK;
}
}
pthread_mutex_unlock(&clients_lock);
return CLIENT_ERR_NOT_FOUND;
}

int client_manager_init(void) {
int result = pthread_mutex_init(&clients_lock, NULL);
if(result != 0) {
return -1;
}
for (int i = 0; i < MAX_CLIENTS; i++) {
clients[i].active = 0;
clients[i].client_id = -1;
clients[i].client_fd = -1;
clients[i].client_state = CLIENT_FREE;
}
return 0;
}

int client_manager_destroy(void) {
int result = pthread_mutex_destroy(&clients_lock);
if (result != 0) {
return -1;
}
return 0;
}

void print_clients(void){
pthread_mutex_lock(&clients_lock);
printf("\n--- client table ----\n");
for(int i =0;i<MAX_CLIENTS;i++){
if(clients[i].active){
printf("slot=%d | id=%d | fd=%d | state=%s\n",i,clients[i].client_id,clients[i].client_fd,client_state_name(clients[i].client_state));
}
}
pthread_mutex_unlock(&clients_lock);
}
int get_client_snapshot(int index,client *out_client){
if(out_client==NULL){return CLIENT_ERR_INVALID;}
if(index<0 || index>=MAX_CLIENTS){
return CLIENT_ERR_INVALID;
}
pthread_mutex_lock(&clients_lock);
*out_client = clients[index];
pthread_mutex_unlock(&clients_lock);
return CLIENT_OK;
}
int connect_client(int client_fd,int client_id){
if(client_fd<0){return CLIENT_ERR_INVALID_FD;}
if(client_id<0){return CLIENT_ERR_INVALID_ID;}
return add_client(client_fd,client_id);
}
int reconnect_client(int client_fd, int client_id){
if (client_fd < 0) {
return CLIENT_ERR_INVALID_FD;
}
if (client_id < 0) {
return CLIENT_ERR_INVALID_ID;
}
pthread_mutex_lock(&clients_lock);
int client_index = -1;
for (int i = 0; i < MAX_CLIENTS; i++) {
if (clients[i].active &&clients[i].client_id == client_id) {
client_index = i;
break;
}
}
if (client_index == -1) {
pthread_mutex_unlock(&clients_lock);
return CLIENT_ERR_NOT_FOUND;
}
for (int i = 0; i < MAX_CLIENTS; i++) {
if (i == client_index) {
continue;
}
if (clients[i].active &&clients[i].client_fd == client_fd) {
pthread_mutex_unlock(&clients_lock);
return CLIENT_ERR_DUPLICATE;
}
}
clients[client_index].client_fd = client_fd;
clients[client_index].client_state = CLIENT_CONNECTED;
pthread_mutex_unlock(&clients_lock);
return CLIENT_OK;
}

int count_connected_clients(void){
int count = 0;
pthread_mutex_lock(&clients_lock);
for (int i = 0; i < MAX_CLIENTS; i++) {
if (clients[i].active &&
clients[i].client_state == CLIENT_CONNECTED) {
count++;
}
}
pthread_mutex_unlock(&clients_lock);
return count;
}

int count_closing_clients(void){
int count = 0;
pthread_mutex_lock(&clients_lock);
for (int i = 0; i < MAX_CLIENTS; i++) {
if (clients[i].active &&
clients[i].client_state == CLIENT_CLOSING) {
count++;
}
}
pthread_mutex_unlock(&clients_lock);
return count;
}

int count_free_slots(void){
int count = 0;
pthread_mutex_lock(&clients_lock);
for (int i = 0; i < MAX_CLIENTS; i++) {
if (!clients[i].active) {
count++;
}
}
pthread_mutex_unlock(&clients_lock);
return count;
}

int client_manager_reset(void){
pthread_mutex_lock(&clients_lock);
for(int i =0;i<MAX_CLIENTS;i++){
clients[i].client_fd = -1;
clients[i].client_id = -1;
clients[i].active    =  0;
clients[i].client_state =CLIENT_FREE;
}
pthread_mutex_unlock(&clients_lock);
return CLIENT_OK;
}
int client_manager_validate(void){
pthread_mutex_lock(&clients_lock);
for (int i = 0; i < MAX_CLIENTS; i++) {
client *current = &clients[i];
if (!current->active) {
if (current->client_fd != -1 ||
current->client_id != -1 ||
current->client_state != CLIENT_FREE) {
pthread_mutex_unlock(&clients_lock);
return CLIENT_ERR_INVALID;
}
continue;
}
if(current->client_fd < 0 ||current->client_id < 0) {
pthread_mutex_unlock(&clients_lock);
return CLIENT_ERR_INVALID;
}
if(current->client_state != CLIENT_CONNECTED &&current->client_state != CLIENT_CLOSING){
pthread_mutex_unlock(&clients_lock);
return CLIENT_ERR_INVALID;
}
for(int j = i + 1; j < MAX_CLIENTS; j++){
client *other = &clients[j];
if(!other->active){
continue;
}
if(current->client_id == other->client_id ||current->client_fd == other->client_fd) {
pthread_mutex_unlock(&clients_lock);
return CLIENT_ERR_DUPLICATE;
}
}
}
pthread_mutex_unlock(&clients_lock);
return CLIENT_OK;
}

int client_manager_mark_all_closing(void){
int changed = 0;
pthread_mutex_lock(&clients_lock);
for (int i = 0; i < MAX_CLIENTS; i++) {
if(clients[i].active &&clients[i].client_state == CLIENT_CONNECTED) {
clients[i].client_state = CLIENT_CLOSING;
changed++;
}
}
pthread_mutex_unlock(&clients_lock);
return changed;
}

int client_manager_disconnect_all(void){
int disconnected = 0;
pthread_mutex_lock(&clients_lock);
for(int i = 0; i < MAX_CLIENTS; i++) {
if(clients[i].active){
disconnected++;
}
clients[i].active = 0;
clients[i].client_id = -1;
clients[i].client_fd = -1;
clients[i].client_state = CLIENT_FREE;
}
pthread_mutex_unlock(&clients_lock);
return disconnected;
}
