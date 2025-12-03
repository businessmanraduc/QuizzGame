#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <ncurses.h>

#include "includes/utils.h"
#include "includes/tui.h"
#include "includes/network.h"
#include "includes/input.h"

static tui_t tui;

void display_welcome(tui_t* tui) {
    add_output_msg(tui->output_terminal, "Client Version 0.1", COLOR_CLIENT);
    add_output_msg(tui->output_terminal, "Note: localhost-mode only", COLOR_CLIENT);
    add_output_msg(tui->output_terminal, "Note: quiz game unavailable, WIP.", COLOR_CLIENT);
    add_output_msg(tui->output_terminal, "\nCLIENT: Entering anon-log mode. Type messages to send to server.", COLOR_CLIENT);
    add_output_msg(tui->output_terminal, "For help, type 'help'\n", COLOR_CLIENT);
}

void process_user_command(const char* input, tui_t* tui) {
    char output_line[1024];
    snprintf(output_line, sizeof(output_line), "> %s", input);
    add_output_msg(tui->output_terminal, output_line, COLOR_CLIENT);
    
    char* command = (char*)input;
    while (*command == ' ' || *command == '\t') command++;
    char* end = command + strlen(command) - 1;
    while (end > command && (*end == ' ' || *end == '\t' || *end == '\n')) end--;
    *(end + 1) = '\0';

    if (strlen(command) > 0) {
        if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
            send_command("quit", tui);
            add_output_msg(tui->output_terminal, "[CLIENT] Shutting down...", COLOR_MAGENTA);
            set_program_running(false);
            sleep(1);
            terminal_shutdown_animation();
        } else if (strcmp(command, "reconnect") == 0) {
            if (get_server_status())
                add_output_msg(tui->output_terminal, "[CLIENT] Warning - Already connected!\n", COLOR_WARNING);
            else 
                attempt_reconnection(tui);
        } else if (strcmp(command, "clear") == 0 || strcmp(command, "clr") == 0) {
            clear_screen(tui->output_terminal);
        } else if (strcmp(command, "debug-mode on") == 0) {
            enable_debug(tui);
        } else if (strcmp(command, "debug-mode off") == 0) {
            disable_debug(tui);
        } else {
            send_command(command, tui);
        }
    }
}

void main_thread(tui_t* tui) {
    char* input;
    while (is_program_running()) {
        input = get_input(tui->input_terminal, tui);
        if (strlen(input) == 0) {
            continue;
        }
        
        process_user_command(input, tui);

        usleep(10000);
    }
}

int main() {
    signal(SIGINT, handle_sigint);
    init_tui(&tui);

    //pthread_t resize_thread_id;
    //pthread_create(&resize_thread_id, NULL, resize_thread, &tui);
    //pthread_detach(resize_thread_id);

    if (!connect_to_server(&tui)) {
        add_output_msg(tui.output_terminal, "[CLIENT] Error - Failed to connect to server automatically.", COLOR_ERROR);
        add_output_msg(tui.output_terminal, "[CLIENT] Type 'reconnect' to attempt manual reconnection to server.", COLOR_WARNING);
    } else {
        pthread_t recv_thread_id;
        pthread_create(&recv_thread_id, NULL, recv_thread, NULL);
        pthread_detach(recv_thread_id);
    }

    display_welcome(&tui);
    main_thread(&tui);

    cleanup_tui(&tui);
    return 0; 
}