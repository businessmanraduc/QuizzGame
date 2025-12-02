#include "network.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static tui_t* global_tui = NULL;

/* Format the response received from the server and print it to the output terminal */
/* @param tui Terminal User Interface struct*/
/* @param resp Response from server */
void print_resp(tui_t* tui, const char* resp) {
    int color;
    if (strstr(resp, "ERR_:"))
        color = COLOR_ERROR;
    else if (strstr(resp, "WARN:"))
        color = COLOR_WARNING;
    else
        color = COLOR_SERVER;
    
    add_output_msg(tui->output_terminal, resp + 5, color);
}

/* Send command to the server */
/* @param tui Terminal User Interface struct*/
/* @param cmd Command to be sent to the server */
void send_command(tui_t* tui, const char* cmd) {
    if (!get_server_status()) {
        add_output_msg(tui->output_terminal, "[CLIENT] Error - Server is offline. Type 'reconnect' to attempt reconnection.", COLOR_ERROR);
        return;
    }
    if (send(client_state.socket_fd, cmd, strlen(cmd), 0) < 0) {
        set_server_status(false);
        add_output_msg(tui->output_terminal, "[CLIENT] Error - Command sending failed!", COLOR_ERROR);
        add_output_msg(tui->output_terminal, "[CLIENT] Server connection lost. Type 'reconnect' to attempt reconnection.", COLOR_WARNING);
    }
    usleep(50000);
}

/* Receive Thread */
void* recv_thread(void* arg) {
    (void)arg;
    char buff[BUFF_SIZE];
    add_output_msg(global_tui->output_terminal, "[RECV_THREAD] Starting...", COLOR_INFO);

    while (is_program_running()) {
        memset(buff, 0, BUFF_SIZE);
        int len = recv(client_state.socket_fd, buff, BUFF_SIZE - 1, 0);
        if (len <= 0) {
            if (global_tui) {
                add_output_msg(global_tui->output_terminal, "[CLIENT] Warning - Disconnected from server/Server offline.\n", COLOR_WARNING);
                add_output_msg(global_tui->output_terminal, "[CLIENT] Type 'reconnect' to attempt reconnnection.", COLOR_WARNING);
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
            strcpy(client_state.client_name, buff + 26);
            int i = 0;
            while (client_state.client_name[i] != '!' && i < MAX_NAME_LEN)
                i++;
            client_state.client_name[i] = '\0';
        } else if (strstr(buff, "RESP:Logged Out")) {
            client_state.loggedIn = false;
        } else if (strstr(buff, "RESP:bye-bye!")) {
            break;
        }

        if (global_tui) {
            format_resp(buff, global_tui);
        }
    }
    
    add_output_msg(global_tui->output_terminal, "[RECV_THREAD] Shutting down...", COLOR_INFO);
    return NULL;
}

/* Connect to the server through socket() and by using TCP */
/* @param tui Terminal User Interface struct*/
bool connect_to_server(tui_t* tui) {
    global_tui = tui;
    
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

/* Attempt reconnection to the server */
/* @param tui Terminal User Interface struct*/
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