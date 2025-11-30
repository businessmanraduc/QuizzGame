#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <ncurses.h>

#include "includes/tui.h"
#include "includes/network.h"
#include "includes/input.h"
#include "includes/utils.h"
#include "includes/colors.h"

input_state_t current_input_state;

static tui_t tui;

void process_user_command(const char* input, tui_t* tui) {
    char output_line[1024];
    snprintf(output_line, sizeof(output_line), "> %s", input);
    add_output_message(tui, output_line, COLOR_CLIENT);
    
    char* command = (char*)input;
    while (*command == ' ' || *command == '\t') command++;
    char* end = command + strlen(command) - 1;
    while (end > command && (*end == ' ' || *end == '\t' || *end == '\n')) end--;
    *(end + 1) = '\0';

    if (strlen(command) > 0) {
        if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
            send_command_to_server("quit", tui);
            add_output_message(tui, "[CLIENT] Shutting down...\n", COLOR_CLIENT);
            set_program_running(false);
            sleep(1);
            terminal_shutdown_animation();
        } else if (strcmp(command, "reconnect") == 0) {
            if (get_server_status())
                add_output_message(tui, "[CLIENT] Warning - Already connected!\n", COLOR_WARNING);
            else 
                attempt_server_reconnect(tui);
        } else if (strcmp(command, "clear") == 0 || strcmp(command, "clr") == 0) {
            clear_output_screen(tui);
        } else if (strcmp(command, "debug-mode on") == 0) {
            enable_debug_mode(tui);
            add_output_message(tui, "[CLIENT] Debug mode enabled", COLOR_INFO);
        } else if (strcmp(command, "debug-mode off") == 0) {
            disable_debug_mode(tui);
            add_output_message(tui, "[CLIENT] Debug mode disabled", COLOR_INFO);
        } else {
            send_command_to_server(command, tui);
        }
    }
}

void run_input_loop(tui_t* tui) {
    char* input;
    while (is_program_running()) {
        input = get_user_input(tui->input_win, &current_input_state, tui->input_height, tui->total_width);
        if (strlen(input) == 0) {
            continue;
        }
        
        process_user_command(input, tui);

        usleep(10000);
    }
}

void display_welcome_messages(tui_t* tui) {
    clear_output_screen(tui);
    add_output_message(tui, "Client Version 0.1", COLOR_CLIENT);
    add_output_message(tui, "Note: localhost-mode only", COLOR_CLIENT);
    add_output_message(tui, "Note: quiz game unavailable, WIP.", COLOR_CLIENT);
    add_output_message(tui, "\nCLIENT: Entering anon-log mode. Type messages to send to server.", COLOR_CLIENT);
    add_output_message(tui, "For help, type 'help'\n", COLOR_CLIENT);
}

int main() {
    initialize_input_state(&current_input_state);
    initialize_tui(&tui);
    signal(SIGINT, handle_sigint);

    pthread_t resize_thread;
    pthread_create(&resize_thread, NULL, monitor_resize_thread, &tui);
    pthread_detach(resize_thread);

    if (!connect_to_server(&tui)) {
        add_output_message(&tui, "[CLIENT] Error - Failed to connect to server automatically.\n", COLOR_ERROR);
        add_output_message(&tui, "[CLIENT] Type 'reconnect' to attempt manual reconnection to server.\n", COLOR_WARNING);
    } else {
        pthread_t recv_thread;
        pthread_create(&recv_thread, NULL, receive_messages_thread, &socket_fd);
        pthread_detach(recv_thread);
    }
    
    display_welcome_messages(&tui);
    run_input_loop(&tui);
    
    cleanup_tui(&tui);
    close(socket_fd);
    return 0;
}