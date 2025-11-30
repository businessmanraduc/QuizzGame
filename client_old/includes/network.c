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

int socket_fd;
bool logged_in = false;
char client_name[MAX_NAME_LEN];
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

void format_server_response(const char* response, tui_t* tui) {
    int color;
    if (strstr(response, "ERR_:"))
        color = COLOR_ERROR;
    else if (strstr(response, "WARN:"))
        color = COLOR_WARNING;
    else
        color = COLOR_SERVER;
    add_output_message(tui, response + 5, color);
}

void send_command_to_server(const char* command, tui_t* tui) {
    if (!get_server_status()) {
        add_output_message(tui, "[CLIENT] Error - Server is offline. Type 'reconnect' to attempt reconnection.\n", COLOR_ERROR);
        return;
    }
    if (send(socket_fd, command, strlen(command), 0) < 0) {
        add_output_message(tui, "[CLIENT] Error - Command sending failed!\n", COLOR_ERROR);
        set_server_status(false);
        add_output_message(tui, "[CLIENT] Server connection lost. Type 'reconnect' to attempt reconnection.\n", COLOR_WARNING);
        return;
    }
    usleep(50000);
}

void* receive_messages_thread(void* socket_ptr) {
    int socket = *(int*)socket_ptr;
    char buffer[BUFF_SIZE];

    while (is_program_running()) {
        memset(buffer, 0, BUFF_SIZE);
        int len = recv(socket, buffer, BUFF_SIZE - 1, 0);
        if (len <= 0) {
            if (global_tui_ref) {
                add_output_message(global_tui_ref, "[CLIENT] Warning - Disconnected from server/Server offline.\n", COLOR_WARNING);
                add_output_message(global_tui_ref, "[CLIENT] Type 'reconnect' to attempt reconnection.\n", COLOR_WARNING);
            }
            set_server_status(false);
            break;
        }

        if (strstr(buffer, "RESP:Welcome")) {
            logged_in = true;
            strcpy(client_name, buffer + 18);
            int i = 0;
            while (client_name[i] != '!' && i < MAX_NAME_LEN) i++;
            client_name[i] = '\0';
        } else if (strstr(buffer, "RESP:Registered")) {
            logged_in = true;
            strcpy(client_name, buffer + 27);
            int i = 0;
            while (client_name[i] != ' ') i++;
            client_name[i] = '\0';
        } else if (strstr(buffer, "RESP:Logged Out")) {
            logged_in = false;
        } else if (strstr(buffer, "RESP:bye-bye!")) {
            break;
        }

        if (global_tui_ref) {
            format_server_response(buffer, global_tui_ref);
        }
    }

    add_output_message(global_tui_ref, "[RECV_THREAD] Shutting down...", COLOR_INFO);
    return NULL;
}

bool connect_to_server(tui_t* tui) {
    global_tui_ref = tui;
    
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        add_output_message(tui, "[CLIENT] Error - Socket creation failed!\n", COLOR_ERROR);
        return false;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    
    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        add_output_message(tui, "[CLIENT] Error - Connection to server failed.\n", COLOR_ERROR);
        close(socket_fd);
        return false;
    }

    set_server_status(true);
    logged_in = false;
    memset(client_name, 0, MAX_NAME_LEN);
    add_output_message(tui, "[CLIENT] Successfully connected to server.\n", COLOR_SUCCESS);
    return true;
}

bool attempt_server_reconnect(tui_t* tui) {
    add_output_message(tui, "[CLIENT] Attempting to reconnect to server...\n", COLOR_CLIENT);
    if (socket_fd > 0) close(socket_fd);

    if (connect_to_server(tui)) {
        pthread_t recv_thread;
        pthread_create(&recv_thread, NULL, receive_messages_thread, &socket_fd);
        pthread_detach(recv_thread);
        return true;
    }
    
    return false;
}

void handle_sigint(int sig) {
    (void)sig;
    signal(SIGINT, SIG_IGN);
    set_program_running(false);
    if (global_tui_ref) {
        cleanup_tui(global_tui_ref);
    }
    if (socket_fd > 0) {
        close(socket_fd);
    }
    exit(0);
}