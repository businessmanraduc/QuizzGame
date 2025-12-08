CC = gcc
CFLAGS = -Wall -Wextra -g

TARGET_DIR = build

SERVER_SRCS = server/server.c \
              server/includes/xml.c \
			  server/includes/libxml.c

CLIENT_SRCS = client/client.c \
			  client/includes/tui.c \
			  client/includes/history.c \
			  client/includes/network.c \
			  client/includes/input.c \
			  client/includes/utils.c

SERVER_TARGET = $(TARGET_DIR)/server
CLIENT_TARGET = $(TARGET_DIR)/client

all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(SERVER_TARGET): $(SERVER_SRCS) | $(TARGET_DIR)
	$(CC) $(CFLAGS) $(SERVER_SRCS) -o $@ -lpthread

$(CLIENT_TARGET): $(CLIENT_SRCS) | $(TARGET_DIR)
	$(CC) $(CFLAGS) $(CLIENT_SRCS) -o $@ -lpthread -lm -lncurses

$(TARGET_DIR):
	mkdir -p $(TARGET_DIR)
	mkdir -p $(TARGET_DIR)/logs

clean:
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)

distclean: clean
	rm -rf $(TARGET_DIR)

.PHONY: all clean distclean