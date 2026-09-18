CC = gcc

CFLAGS = -Wall -Wextra -Wpedantic -pthread

SERVER = server
CLIENT = client

SERVER_OBJS = server.o my_thread.o binary_protocol.o
CLIENT_OBJS = client.o binary_protocol.o

all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_OBJS)
	$(CC) $(CFLAGS) $(SERVER_OBJS) -o $(SERVER)

$(CLIENT): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $(CLIENT_OBJS) -o $(CLIENT)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SERVER) $(CLIENT) *.o

.PHONY: all clean