#include"my_thread.h"
#include"binary_protocol.h"
client clients[MAX_CLIENTS];
pthread_mutex_t clients_lock;

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

int add_client(int client_fd,int client_id,pthread_t thread_id){
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
if(clients[i].active==0 && clients[i].thread_joinable==0){
clients[i].client_fd = client_fd;
clients[i].client_id = client_id;
clients[i].active = 1;
clients[i].client_state = CLIENT_CONNECTED;
clients[i].thread_id = thread_id;
clients[i].thread_joinable = 1;
clients[i].thread_finished = 0;
pthread_mutex_unlock(&clients_lock);
return 0;
}
}
pthread_mutex_unlock(&clients_lock);
return CLIENT_ERR_FULL;
}

int mark_thread_finished(int client_id){
pthread_mutex_lock(&clients_lock);
for(int i = 0;i<MAX_CLIENTS;i++){
if(clients[i].active==1 && clients[i].client_id == client_id){
clients[i].thread_finished = 1;
pthread_mutex_unlock(&clients_lock);
return 1;
}
}
pthread_mutex_unlock(&clients_lock);
return 0;
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
  //clients[i].thread_id = 0;
  clients[i].thread_joinable = 1;
  pthread_mutex_unlock(&clients_lock);
  return CLIENT_OK;
}
}
pthread_mutex_unlock(&clients_lock);
return CLIENT_ERR_NOT_FOUND;
}

bool send_stats_response(int client_fd, uint64_t sequence){
struct stats_payload stats = {
.active_clients = count_active_clients(),
.connected_clients = count_connected_clients(),
.closing_clients = count_closing_clients(),
.free_slots = count_free_slots()
};
uint64_t values[4] = {
stats.active_clients,
stats.connected_clients,
stats.closing_clients,
stats.free_slots
};
uint8_t stats_buffer[STATS_PAYLOAD_SIZE];
write_to_8byte_buffer(stats_buffer,values,4);
struct message response;
bool initialized = message_init(&response,STATS,sequence,stats_buffer,STATS_PAYLOAD_SIZE);
if(!initialized){
printf("[worker] STATS message initialization failed\n");
return false;
}
uint8_t response_buffer[HEADER_SIZE + STATS_PAYLOAD_SIZE];
size_t serialized_size = serialize(&response,response_buffer,sizeof(response_buffer));
if(serialized_size == 0){
printf("[worker] STATS serialization failed\n");
return false;
}
if(!Send_all(client_fd, response_buffer, serialized_size)){
printf("[worker] STATS send failed\n");
return false;
}
printf("[worker] STATS sent: active=%lu connected=%lu closing=%lu free=%lu\n",stats.active_clients,stats.connected_clients,stats.closing_clients,stats.free_slots);
return true;
}



void *worker(void *arg){
struct data *data = arg;
if(data==NULL){return NULL;}
int client_fd = data->client_fd;
int client_id = data->client_id;
free(data);
pthread_t thread_id = pthread_self();
int add_client_check = add_client(client_fd,client_id,thread_id);
if(add_client_check!=CLIENT_OK){
printf("[WORKER] failed to add client: fd=%d | id=%d | result=%d\n",client_fd,client_id,add_client_check);
close(client_fd);
return NULL;
}
printf("[worker] client register: id=%d, fd=%d\n",client_id,client_fd);
uint8_t receive_buffer[8192];
size_t buffered_bytes = 0;
bool protocol_error = false;
while(1){
if(buffered_bytes==sizeof(receive_buffer)){
fprintf(stderr,"[workder] receive buffer full\n");
break;
}
ssize_t bytes_read = recv(client_fd,receive_buffer+buffered_bytes,sizeof(receive_buffer)-buffered_bytes,0);
if(bytes_read>0){
buffered_bytes += bytes_read;
printf("[worker] client id=%d receive: %zd bytes\n",client_id,bytes_read);
if(buffered_bytes<HEADER_SIZE){
printf("[worker] waiting for complete header...\n");
continue;
}
printf("[worker] complete header received\n");
while(buffered_bytes>=HEADER_SIZE){
uint16_t payload_length = ((receive_buffer[2]<<8)|receive_buffer[3]);
if(payload_length>MAX_ALLOWED_PAYLOAD){
printf("invalid payload length : %u\n",payload_length);
protocol_error = true;
break;
}
size_t frame_size = HEADER_SIZE + payload_length;
if(buffered_bytes<frame_size){  
printf("worker waiting for complete frame...\n");
break;
}
struct message incoming_message;
bool decoded = deserialize(receive_buffer,frame_size,&incoming_message);
if(!decoded){
printf("[worker] invalid frame received\n");
protocol_error = true;
break;
}
printf("[worker] frame decoded successfully\n");
printf("worker message type : %u\n",incoming_message.header.type);
printf("worker sequence %lu\n",incoming_message.header.sequence);
switch(incoming_message.header.type){
  case PING: {
    uint8_t response_buffer[HEADER_SIZE];
    printf("worker receive PING\n");
    struct message response;
    bool response_message_init = message_init(&response,PONG,incoming_message.header.sequence,NULL,0);
    if(!response_message_init){
    printf("response message init fail!");
    close(client_fd);
    exit(EXIT_FAILURE);
    }
    size_t response_serialization = serialize(&response,response_buffer,sizeof(response_buffer));
    if(response_serialization==0){
    printf("PONG serialization failed!\n");
    close(client_fd);
    exit(EXIT_FAILURE);
    }
    bool response_send_all = Send_all(client_fd,response_buffer,response_serialization);
    if(!response_send_all){printf("PONG send failed!\n");protocol_error=true;break;}

    printf("pong sent successfuly\n");
    break;
             }
  case ECHO: {
    printf("worker receive ECHO\n");
    struct message response;
    bool response_message_init =message_init(&response,ECHO,incoming_message.header.sequence,incoming_message.payload,incoming_message.header.payload_length);
    if (!response_message_init) {
        printf("ECHO response message init failed!\n");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    uint8_t response_buffer[HEADER_SIZE + incoming_message.header.payload_length];
    size_t response_serialization =serialize(&response,response_buffer,sizeof(response_buffer));
    if (response_serialization == 0) {
        printf("ECHO serialization failed!\n");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    bool response_send_all =
        Send_all(client_fd,response_buffer,response_serialization);
    if (!response_send_all) {
        printf("ECHO response send failed!\n");
        protocol_error = true;
        break;
    }
    printf("ECHO response sent successfully\n");
    break;
                 }         
  case GET_STATS: {
    printf("[worker] GET_STATS received\n");
    bool stats_sent = send_stats_response(client_fd,incoming_message.header.sequence);
    if (!stats_sent) {
      protocol_error = true;
      break;
    }
    break;
                  }
  default:{
    printf("unknown message type :%u\n",incoming_message.header.type);
    break;
          }
}
buffered_bytes -= frame_size;
if(buffered_bytes>0){
memmove(receive_buffer,receive_buffer+frame_size,buffered_bytes);
}
printf("[worker] remaining buffered bytes: %zu\n",buffered_bytes);
}
if (protocol_error) {
    printf("[worker] protocol error, closing connection: id=%d\n", client_id);
    break;
}
}else if(bytes_read==0){
printf("[worker] client disconnected: id=%d\n",client_id);
break;
}else{
if(errno==EINTR){
continue;
}
perror("[worker] recv failed!\n");
break;
}
}
printf("[worker] client id=%d worker ending\n",client_id);
int result = mark_client_closing(client_id);
if(result!=CLIENT_OK){
printf("[worker] failed to mark client closing: id=%d error=%d\n",client_id,result);
}
close(client_fd);
int mrk_thread_finish = mark_thread_finished(client_id);
if(mrk_thread_finish){
printf("[worker] mark_thread_finished\n");
}else{
printf("[worker] mark_thread_finished failed\n");
}
result = remove_client(client_id);
if(result!=CLIENT_OK){
printf("[worker] failed to remove client: id=%d error=%d\n",client_id,result);
}
printf("[worker] client cleaned up: id=%d fd=%d\n",client_id,client_fd);
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
//clients[i].thread_id = 0;
clients[i].thread_joinable = 0;
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
return add_client(client_fd,client_id,pthread_self());
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
//clients[i].thread_id = 0;
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
printf("[manager] client %d marked closing\n",i+1);
}
}
pthread_mutex_unlock(&clients_lock);
return changed;
}

int client_manager_disconnect_all(void){
int disconnected = 0;
pthread_mutex_lock(&clients_lock);
for(int i = 0; i < MAX_CLIENTS; i++) {
if(!clients[i].active){
continue;
}
int client_fd = clients[i].client_fd;
int client_id = clients[i].client_id;
clients[i].client_state = CLIENT_CLOSING;
if(client_fd>=0){
if(shutdown(client_fd,SHUT_RDWR)==-1){
if(errno!=ENOTCONN){
fprintf(stderr,"[manager] shutdown failed for client id=%d fd=%d: %s\n",client_id,client_fd,strerror(errno));
}
}else {
printf("[manager] client id=%d fd=%d disconnected\n",client_id,client_fd);
}
}
disconnected++;
}
pthread_mutex_unlock(&clients_lock);
return disconnected;
}

int client_manager_join_all_workers(void){
pthread_t threads[MAX_CLIENTS];
int thread_count = 0;
pthread_mutex_lock(&clients_lock);
for(int i = 0; i < MAX_CLIENTS; i++) {
if(clients[i].thread_joinable) {threads[thread_count] = clients[i].thread_id;
thread_count++;
}
}
pthread_mutex_unlock(&clients_lock);
int joined_count = 0;
for(int i = 0; i < thread_count; i++) {
int result = pthread_join(threads[i], NULL);
if(result == 0) {
joined_count++;
}else{
fprintf(stderr,"[manager] pthread_join failed: %s\n",
strerror(result));
}
}
return joined_count;
}


int client_manager_reap_finished_workers(void){
pthread_t threads_ids[MAX_CLIENTS];
int slot_ids[MAX_CLIENTS];
int thread_count = 0;
pthread_mutex_lock(&clients_lock);
for(int i = 0; i < MAX_CLIENTS; i++){
if(clients[i].thread_finished == 1 && clients[i].thread_joinable == 1){
threads_ids[thread_count] = clients[i].thread_id;
slot_ids[thread_count] = i;
thread_count++;
}
}
pthread_mutex_unlock(&clients_lock);
int joined_count = 0;
for(int i = 0; i < thread_count; i++){
int res = pthread_join(threads_ids[i], NULL);
if(res == 0){
joined_count++;
pthread_mutex_lock(&clients_lock);
clients[slot_ids[i]].thread_finished = 0;
clients[slot_ids[i]].thread_joinable = 0;
pthread_mutex_unlock(&clients_lock);
}else{
fprintf(stderr,"[manager] pthread_join failed: %s\n",strerror(res));
}
}
return joined_count;
}