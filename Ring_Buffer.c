#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
uint8_t *buffer;
size_t head;       
size_t tail;      
size_t capacity;
size_t count;
} RingBuffer;
bool rb_init(RingBuffer *rb, size_t capacity) {
if (!rb || capacity == 0) return false;
rb->buffer = (uint8_t *)malloc(capacity * sizeof(uint8_t));
if (!rb->buffer) return false;
rb->head = 0;
rb->tail = 0;
rb->capacity = capacity;
rb->count = 0;
return true;
}
void rb_free(RingBuffer *rb) {
if(rb && rb->buffer){
free(rb->buffer);
rb->buffer = NULL;
}
}
bool rb_is_empty(const RingBuffer *rb) {
return rb->count == 0;
}
bool rb_is_full(const RingBuffer *rb) {
return rb->count == rb->capacity;
}
bool rb_push(RingBuffer *rb, uint8_t data) {
if(rb_is_full(rb)){
return false; 
}
rb->buffer[rb->head] = data;
rb->head = (rb->head + 1) % rb->capacity;
rb->count++;
return true;
}
bool rb_pop(RingBuffer *rb, uint8_t *data) {
if(rb_is_empty(rb)){
return false;
}
*data = rb->buffer[rb->tail];
rb->tail = (rb->tail + 1) % rb->capacity;
rb->count--;
return true;
}
bool rb_peek(const RingBuffer *rb, uint8_t *data){
if(rb_is_empty(rb)){
return false;
}
*data = rb->buffer[rb->tail];
return true;
}
int main(void) {
RingBuffer rb;
rb_init(&rb, 4);
for(uint8_t i = 10; i <= 40; i += 10){
rb_push(&rb, i);
}
uint8_t val;
rb_pop(&rb, &val);
printf("Popped: %u\n", val);
rb_pop(&rb, &val);
printf("Popped: %u\n", val);
rb_push(&rb, 50);
rb_push(&rb, 60);
while (rb_pop(&rb, &val)) {
printf("Popped: %u\n", val);
}
rb_free(&rb);
return 0;
}
