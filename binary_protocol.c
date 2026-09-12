#include<stdint.h>
#include<stdio.h>
#include"binary_protocol.h"
#include<string.h>
bool message_init(struct message *message,uint8_t type,uint64_t sequence,const uint8_t *payload,size_t payload_length){
if(message==NULL || (type!=PING && type!=ECHO &&type !=PONG && type!=GET_STATS && type!=STATS) || payload_length>MAX_ALLOWED_PAYLOAD || (payload ==NULL && payload_length>0)) return false;
message->header.type = type;
message->header.sequence = sequence;
message->header.payload_length = payload_length;
message->header.version = BINARY_PROTOCOL_VERSION;
if(payload_length>0){
memcpy(message->payload,payload,payload_length);
}
return validate(message);
}
size_t serialize(const struct message *message,uint8_t *output_buffer,size_t buffer_capacity){
  if(message==NULL || output_buffer==NULL){
return 0;
  }
if(buffer_capacity==0){return 0;}
output_buffer[0] = message->header.version;
if(buffer_capacity<2) return 1;
output_buffer[1] = message->header.type;
if(buffer_capacity<4) return 2;
output_buffer[2] = ((message->header.payload_length)>>8) & 0xFF;
output_buffer[3] = (message->header.payload_length)&0xFF;
if(buffer_capacity<12) return 4;
int k = 7;
for(int i=4;i<12;i++){
output_buffer[i] = ((message->header.sequence)>>(8*k))&0xFF;
k--;
}
size_t required = 12 + message->header.payload_length;
if(buffer_capacity<required) return 0;
memcpy(output_buffer+12,message->payload,message->header.payload_length);
return 12 + message->header.payload_length;
}

bool deserialize(const uint8_t *input_buffer,size_t input_length,struct message *message){
  if(input_buffer==NULL || message==NULL){
return false;
  }
if(input_length<12) return false;
message->header.version = input_buffer[0];
message->header.type = input_buffer[1];
message->header.payload_length = ((input_buffer[2]<<8) | input_buffer[3]);
int k = 7;
message->header.sequence = 0;
for(int i=4;i<12;i++){
message->header.sequence |= (((uint64_t)input_buffer[i])<<(8*k));
  k--;
}
size_t required = 12 + message->header.payload_length;
if(input_length<required || message->header.payload_length>MAX_ALLOWED_PAYLOAD) return false;

memcpy(message->payload,input_buffer+12,message->header.payload_length);
if(!validate(message)) return false;
return true;
}

bool validate(const struct message *message){
  if(message==NULL){
return false;
  }
if(message->header.version!= BINARY_PROTOCOL_VERSION || (message->header.type!=PING && message->header.type!=ECHO &&message->header.type!=PONG && message->header.type!=GET_STATS && message->header.type!=STATS) || message->header.payload_length>MAX_ALLOWED_PAYLOAD || (message->header.type==PING && message->header.payload_length!=0)||(message->header.type==PONG && message->header.payload_length!=0)||(message->header.type==GET_STATS && message->header.payload_length!=0)||(message->header.type==STATS && message->header.payload_length!=32)){return false;}
return true;
}
bool write_to_8byte_buffer(uint8_t *out_buffer,const uint64_t *in_buffer,uint64_t len_of_in_buffer){
  if(out_buffer==NULL || in_buffer==NULL){return false;}
uint64_t k = 0;
for(int i =0;i<len_of_in_buffer;i++){
for(int j =7;j>=0;j--){
out_buffer[k] = (in_buffer[i]>>(8*j)&0xFF);
k++;
}
}
if(k!=(len_of_in_buffer*8)){
return false;
}
return true;
}
