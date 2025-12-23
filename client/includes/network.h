#ifndef NETWORK_H
#define NETWORK_H

#include <stdbool.h>
#include "utils.h"

void print_resp(tui_t*, const char*);
void send_command(tui_t*, const char*);
void* recv_thread(void*);
bool connect_to_server(tui_t*);
bool attempt_reconnection(tui_t*);

#endif
