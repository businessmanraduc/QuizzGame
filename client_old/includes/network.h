#ifndef NETWORK_H
#define NETWORK_H

#include <stdbool.h>
#include "colors.h"
#include "tui.h"

extern int socket_fd;
extern bool logged_in;
extern char client_name[256];

void set_server_status(bool status);
bool get_server_status();
void format_server_response(const char* response, tui_t* tui);
void send_command_to_server(const char* command, tui_t* tui);
void* receive_messages_thread(void* socket_ptr);
bool connect_to_server(tui_t* tui);
bool attempt_server_reconnect(tui_t* tui);
void handle_sigint(int sig);

#endif