CC = gcc
CFLAGS = -Wall -Wextra -g
XML_CFLAGS = $(shell pkg-config --cflags libxml-2.0)
XML_LIBS = $(shell pkg-config --libs libxml-2.0)

TARGET_DIR = build

SERVER_SRCS = server/server.c \
              server/includes/xml.c

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
	$(CC) $(CFLAGS) $(XML_CFLAGS) $(SERVER_SRCS) -o $@ $(XML_LIBS) -lpthread

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