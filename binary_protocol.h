#ifndef BINARY_PROTOCOL_H
#define BINARY_PROTOCOL_H

#include<stdint.h>
#include<stddef.h>
#include<stdbool.h>
#define BINARY_PROTOCOL_VERSION 1
#define MAX_ALLOWED_PAYLOAD 4096
#define HEADER_SIZE 12
#define STATS_PAYLOAD_SIZE 32

enum message_type{
PING,
ECHO,
PONG,
GET_STATS,
STATS
};

struct Message_header{
uint8_t version;
uint8_t type;
uint16_t payload_length;
uint64_t sequence;
};

struct stats_payload {
uint64_t active_clients;
uint64_t connected_clients;
uint64_t closing_clients;
uint64_t free_slots;
};

static_assert(sizeof(struct stats_payload)==STATS_PAYLOAD_SIZE,"stats_payload size must be exaxtly 32 bytes");

struct message{
struct Message_header header;
uint8_t payload[MAX_ALLOWED_PAYLOAD];
};
bool message_init(struct message *message,uint8_t type,uint64_t sequence,const uint8_t *payload,size_t payload_length);
size_t serialize(const struct message*,uint8_t* output_buffer,size_t buffer_capacity);
bool deserialize(const uint8_t* input_buffer,size_t input_length,struct message*);
bool validate(const struct message*);
//bool Send_all(int socket,const uint8_t *buffer,size_t len);
bool write_to_8byte_buffer(uint8_t *out_buffer,const uint64_t *in_buffer,uint64_t len_of_in_buffer);
#endif
