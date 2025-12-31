CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_GNU_SOURCE -g -I.
LDFLAGS = -pthread -lsqlite3

CLIENT_SRC = TCP_Client/client.c
SERVER_SRC = TCP_Server/server.c \
	         TCP_Server/ultilities.c \
	         TCP_Server/database.c \
	         TCP_Server/command_handlers.c

CLIENT_OBJS = $(CLIENT_SRC:.c=.o)
SERVER_OBJS = $(SERVER_SRC:.c=.o)

CLIENT_BIN = client
SERVER_BIN = server

.PHONY: all clean

all: $(CLIENT_BIN) $(SERVER_BIN)

# Compile object files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(CLIENT_BIN): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(SERVER_BIN): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(CLIENT_BIN) $(SERVER_BIN) $(CLIENT_OBJS) $(SERVER_OBJS)