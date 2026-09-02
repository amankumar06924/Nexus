#include<stdint.h>
#include<stdio.h>
#include"binary_protocol.h"

size_t serialize(const struct message *message,uint8_t *output_buffer,size_t buffer_capacity){
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
return 12;
}

bool deserialize(const uint8_t *input_buffer,size_t input_length,struct *message){


}

bool validate(const struct *message){


}

int main(){

return 0;
}
