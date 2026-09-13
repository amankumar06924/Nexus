#include"my_thread.h"

int add_client(int client_fd,int client_id){
pthread_mutex_lock(&clients_lock);
for(int i=0;i<MAX_CLIENTS;i++){
if(clients[i].active && clients[i].client_id==client_id){
pthread_mutex_unlock(&clients_lock);
return -1;
}
}
for(int i=0;i<MAX_CLIENTS;i++){
if(clients[i].active==0){
clients[i].client_fd = client_fd;
clients[i].client_id = client_id;
clients[i].active = 1;
pthread_mutex_unlock(&clients_lock);
return 0;
}
}
pthread_mutex_unlock(&clients_lock);
return -1;
}

int remove_client(int client_id){
pthread_mutex_lock(&clients_lock);
for(int i =0;i<MAX_CLIENTS;i++){
if(clients[i].client_id==client_id && clients[i].active==1){
  clients[i].active = 0;
  clients[i].client_id  = -1;
  clients[i].client_fd  = -1;
  pthread_mutex_unlock(&clients_lock);
  return 0;
}
}
pthread_mutex_unlock(&clients_lock);
return -1;
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
pthread_mutex_lock(&clients_lock);
for(int i=0;i<MAX_CLIENTS;i++){
if(clients[i].client_id==client_id && clients[i].active){
clients[i].client_fd = client_new_fd;
pthread_mutex_unlock(&clients_lock);
return 0;
}
}
pthread_mutex_unlock(&clients_lock);
return -1;
}

int main(){
pthread_t threads[6];
struct data data1[6] = {{100,1},{101,2},{102,3},{200,4},{201,5},{202,6}};
for(int i=0;i<6;i++){
int t1_thread = pthread_create(&threads[i],NULL,worker,&data1[i]);
if(t1_thread!=0){
printf("failed threaad is %d\n",i);
return 0;
}
}
add_client(500,10);
for(int i =0;i<6;i++){
int t1_join = pthread_join(threads[i],NULL);
if(t1_join!=0){
printf("failed join at %d\n",i);
return 0;
}
}
//int t2_join = pthread_join(t2,NULL);
//if(t1_thread!=0 || t2_thread!=0 || t1_join!=0 ||t2_join!=0){ printf("something worng in t thrad or join") ;return 1;}
pthread_mutex_lock(&clients_lock);
for(int i =0;i<MAX_CLIENTS;i++){
  if(clients[i].active){
printf("the fd , id,active is %d,%d,%d\n",clients[i].client_fd,clients[i].client_id,clients[i].active);
}
}
pthread_mutex_unlock(&clients_lock);

int total_active_client_is = count_active_clients();
printf("\ntotal active clients is %d\n",total_active_client_is);
int res = remove_client(3);
if(res==0) printf("res pass");
else printf("res failed");
printf("after remove client id 3 %d\n",count_active_clients());
client found;
int c = find_client(5,&found);
if(c==0){
printf("found client: fd=%d id=%d\n",found.client_fd,found.client_id);
}else{
printf("client not found\n");
}

int update_res = update_client_fd(5,9999);
if(update_res==0) printf("clients fd update\n");
else printf("clients not found\n");

client found1;
if(find_client(5,&found1)==0){
printf("update client: fd=%d id=%d\n",found1.client_fd,found1.client_id);
}

int duplicate = add_client(700,20);
duplicate = add_client(555,21);
duplicate = add_client(5555,22);
duplicate = add_client(55555,23);
if(duplicate==0){
printf("duplicate add\n");
}else{
printf("duplicate rejected\n");
}

pthread_mutex_destroy(&clients_lock);
return 0;
}
