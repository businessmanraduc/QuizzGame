#ifndef NETWORK_H
#define NETWORK_H

#include <stdbool.h>
#include "tui.h"

typedef struct {
    int socket_fd;
    bool loggedIn;
    char client_name[256];
} client_state_t;

extern client_state_t client_state;

void set_server_status(bool);
bool get_server_status();
void format_resp(const char*, tui_t*);
void send_command(const char*, tui_t*);
void* recv_thread(void*);
bool connect_to_server(tui_t*);
bool attempt_reconnection(tui_t*);

#endif