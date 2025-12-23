#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <ncurses.h>

#include "includes/utils.h"
#include "includes/history.h"
#include "includes/tui.h"
#include "includes/network.h"
#include "includes/input.h"

void display_welcome(tui_t* tui) {
    add_output_msg(tui->output_terminal, "Client Version 0.1", COLOR_CLIENT);
    add_output_msg(tui->output_terminal, "Note: localhost-mode only", COLOR_CLIENT);
    add_output_msg(tui->output_terminal, "Note: quiz game unavailable, WIP.", COLOR_CLIENT);
    add_output_msg(tui->output_terminal, "\nCLIENT: Entering anon-log mode. Type messages to send to server.", COLOR_CLIENT);
    add_output_msg(tui->output_terminal, "For help, type 'help'\n", COLOR_CLIENT);
}

void main_thread(tui_t* tui) {
    add_output_msg(tui->output_terminal, "[MAIN_THREAD] Starting...", COLOR_CLIENT);

    char* input;
    input_state.curr_idx = -1;
    input_state.temp_command[0] = '\0';
    init_input(&command_buff);
    redraw_input(tui->input_terminal);

    while (is_program_running()) {
        input = get_input(tui->input_terminal);

        if (input != NULL && strlen(input) != 0) {
            input_state.curr_idx = -1;
            input_state.temp_command[0] = '\0';
            init_input(&command_buff);
            process_user_command(input, tui);
            redraw_input(tui->input_terminal);
        }
        if (is_resize_needed())
            resize_client(tui);
        if (tui->debug_mode)
            redraw_debug(tui->debug_terminal, tui->input_terminal);
    }

    add_output_msg(tui->output_terminal, "[MAIN_THREAD] Shutting down...", COLOR_CLIENT);
}

int main(int argc, char* argv[]) {
    tui_t client_tui;
    char sv_addr[256] = "";
    if (argc == 1)
        strcpy(sv_addr, "127.0.0.1");
    else
        strcpy(sv_addr, argv[1]);

    signal(SIGINT, handle_sigint);
    signal(SIGWINCH, handle_sigwinch);
    printf("[CLIENT_DEBUG] Signal handlers set. Initiating TUI...\n");
    init_tui(&client_tui, sv_addr);
    //printf("[CLIENT_DEBUG] TUI Initialized. Starting history...\n");
    init_history(&input_state);
    init_input(&command_buff);
    //printf("[CLIENT_DEBUG] History Initialized. Engaging connection with server...\n");

    if (!connect_to_server(&client_tui)) {
        add_output_msg(client_tui.output_terminal, "[CLIENT] Error - Failed to connect to server automatically.", COLOR_ERROR);
        add_output_msg(client_tui.output_terminal, "[CLIENT] Type 'reconnect' to attempt manual reconnection to server.", COLOR_WARNING);
    } else {
        pthread_t recv_thread_id;
        pthread_create(&recv_thread_id, NULL, recv_thread, NULL);
        pthread_detach(recv_thread_id);
    }

    display_welcome(&client_tui);
    main_thread(&client_tui);

    cleanup_tui(&client_tui);
    return 0;
}
