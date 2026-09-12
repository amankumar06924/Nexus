## C 
today is 1sep and i work on implementing binary protocol in c. 

|--today i write some header , make struct in binary_protocol.h file

2 sep 

today i work on binary_protocol.c file , define the serilize function

1) i learn how to split bytes link:- (https://stackoverflow.com/questions/42896154/python-split-byte-into-high-low-nibbles)
2) learn about what is serilization and Deserilization , link:- (https://www.geeksforgeeks.org/java/serialization-and-deserialization-in-java/)

3 sep

today i define deserilization and validation function

4 sep 

today i test my deserilize and serilize function and define new function
bool message_init(struct message *message,uint8_t type,uint64_t sequence,const uint8_t *payload,size_t payload_length); because it help me to define message_header directly

5 sep 

today i learn about how to implement TCP server and client in c link:-(https://medium.com/@shivambhadani_/understanding-tcp-and-building-our-own-tcp-server-in-c-language-8de9d9de78ef)  

and familer with new error called error: lvalue required as an unary '&' operand. solution link:-(https://stackoverflow.com/questions/22788026/error-lvalue-required-as-unary-operand).

8sep

today i learn about memmove() function in c you can read about this function using (man memmove).

implement tcp fragment message to test server working good or not send_fragmented_message(socket,serilization,buffer) and it rutern true false.

add GET_STATS and STATS to check the status of server 

tyr to implement multi request/response