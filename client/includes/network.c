#include "network.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

static tui_t* global_tui_ref = NULL;
client_state_t client_state = {0, false, {0}};
bool server_online = true;
pthread_mutex_t server_status_mutex = PTHREAD_MUTEX_INITIALIZER;

void set_server_status(bool status) {
    pthread_mutex_lock(&server_status_mutex);
    server_online = status;
    pthread_mutex_unlock(&server_status_mutex);
}

bool get_server_status() {
    pthread_mutex_lock(&server_status_mutex);
    bool status = server_online;
    pthread_mutex_unlock(&server_status_mutex);
    return status;
}

void format_resp(const char* resp, tui_t* tui) {
    int color;
    if (strstr(resp, "ERR_:"))
        color = COLOR_ERROR;
    else if (strstr(resp, "WARN:"))
        color = COLOR_WARNING;
    else
        color = COLOR_SERVER;
    add_output_msg(tui->output_terminal, resp + 5, color);
}

void send_command(const char* cmd, tui_t* tui) {
    if (!get_server_status()) {
        add_output_msg(tui->output_terminal, "[CLIENT] Error - Server is offline. Type 'reconnect' to attempt reconnection.", COLOR_ERROR);
        return;
    }
    if (send(client_state.socket_fd, cmd, strlen(cmd), 0) < 0) {
        set_server_status(false);
        add_output_msg(tui->output_terminal, "[CLIENT] Error - Command sending failed!", COLOR_ERROR);
        add_output_msg(tui->output_terminal, "[CLIENT] Server connection lost. Type 'reconnect' to attempt reconnection.", COLOR_WARNING);
        return;
    }
    usleep(50000);
}

void* recv_thread(void* arg) {
    (void)arg;
    char buff[BUFF_SIZE];
    add_output_msg(global_tui_ref->output_terminal, "[RECV_THREAD] Booting up...", COLOR_INFO);

    while (is_program_running()) {
        memset(buff, 0, BUFF_SIZE);
        int len = recv(client_state.socket_fd, buff, BUFF_SIZE - 1, 0);
        if (len <= 0) {
            if (global_tui_ref) {
                add_output_msg(global_tui_ref->output_terminal, "[CLIENT] Warning - Disconnected from server/Server offline.\n", COLOR_WARNING);
                add_output_msg(global_tui_ref->output_terminal, "[CLIENT] Type 'reconnect' to attempt reconnnection.", COLOR_WARNING);
            }
            set_server_status(false);
            break;
        }

        if (strstr(buff, "RESP:Welcome")) {
            client_state.loggedIn = true;
            strcpy(client_state.client_name, buff + 18);
            int i = 0;
            while (client_state.client_name[i] != '!' && i < MAX_NAME_LEN)
                i++;
            client_state.client_name[i] = '\0';
        } else if (strstr(buff, "RESP:Registered")) {
            client_state.loggedIn = true;
            strcpy(client_state.client_name, buff + 27);
            int i = 0;
            while (client_state.client_name[i] != '!' && i < MAX_NAME_LEN)
                i++;
            client_state.client_name[i] = '\0';
        } else if (strstr(buff, "RESP:Logged Out")) {
            client_state.loggedIn = false;
        } else if (strstr(buff, "RESP:bye-bye!")) {
            break;
        }

        if (global_tui_ref) {
            format_resp(buff, global_tui_ref);
        }
    }
    
    add_output_msg(global_tui_ref->output_terminal, "[RECV_THREAD] Shutting down...", COLOR_INFO);
    return NULL;
}

bool connect_to_server(tui_t* tui) {
    global_tui_ref = tui;
    
    client_state.socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_state.socket_fd < 0) {
        add_output_msg(tui->output_terminal, "[CLIENT] Error - Socket creation failed!\n", COLOR_ERROR);
        return false;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    
    if (connect(client_state.socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        add_output_msg(tui->output_terminal, "[CLIENT] Error - Connection to server failed.\n", COLOR_ERROR);
        set_server_status(false);
        close(client_state.socket_fd);
        return false;
    }

    set_server_status(true);
    client_state.loggedIn = false;
    memset(client_state.client_name, 0, MAX_NAME_LEN);
    add_output_msg(tui->output_terminal, "[CLIENT] Successfully connected to server.\n", COLOR_SUCCESS);
    return true;
}

bool attempt_reconnection(tui_t* tui) {
    add_output_msg(tui->output_terminal, "[CLIENT] Attempting to reconnect to server...\n", COLOR_CLIENT);
    if (client_state.socket_fd > 0) close(client_state.socket_fd);

    if (connect_to_server(tui)) {
        pthread_t recv_thread_id;
        pthread_create(&recv_thread_id, NULL, recv_thread, &client_state.socket_fd);
        pthread_detach(recv_thread_id);
        return true;
    }
    
    return false;
}