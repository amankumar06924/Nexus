#include"binary_protocol.h"
#include<stdio.h>
#include<string.h>
int main(){
struct message ms;
bool flag1 = message_init(&ms,ECHO,1,(const uint8_t *)"Hello",5);
printf("flag1: %s\n",flag1?"pass":"fail");
//bool flag2 = message_init(&ms,PING,1,(const uint8_t *)"hello",5);
//printf("flag2: %s\n",flag2?"pass":"fail");
//bool flag3 = message_init(&ms,ECHO,1,(const uint8_t *)"hello",5);
//printf("flag3: %s\n",flag3?"p":"f");
//bool flag4 = message_init(&ms,PING,1,(const uint8_t *)"hello",5000);
//printf("flag4: %s\n",flag4?"p":"f");

//ms.header.version = 1;
//ms.header.type = PING;
//ms.header.payload_length = 5;
//ms.header.sequence = 0x1122334455667788;
//ms.header.sequence = 1;
//memcpy(ms.payload,"hello",5);
//printf("valid: %s\n",validate(&ms)?"pass":"fail");
//ms.header.version = 99;
//printf("wrong version : %s\n",!validate(&ms)?"pass":"fail");
//ms.header.version = 1;
//ms.header.type = 99;
//printf("wrong type :%s\n",!validate(&ms)?"pass":"fail");
//ms.header.type = PING;
//ms.header.payload_length = MAX_ALLOWED_PAYLOAD + 1;
//printf("oversized payload : %s\n",!validate(&ms)?"pass":"fail");

uint8_t buffer[17];
//uint8_t buffer[5];
size_t written = serialize(&ms,buffer,sizeof(buffer));
//printf("small buffer K %zu\n",written);
//printf("the written value is %zu\n",written);
if(written==0){ printf("serilization failed"); return 1;}

for(int i =0;i<17;i++){
printf("%02X ",buffer[i]);
}
printf("\n");
struct message decoder;
bool check_des = deserialize(buffer,17,&decoder);
//printf("truncated packet: %s\n", check_des?"fail":"pass");
if(!check_des){ printf("deserialization failde\n"); return 1;}
else {printf("deserialize pass\n");}

return 0;

} 
