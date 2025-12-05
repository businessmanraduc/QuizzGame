#include "input.h"
#include "network.h"
#include <string.h>
#include <unistd.h>

/* Initialize the Input and the History for the client */
/* @param command_buffer The main buffer for non-completed commands */
void init_input(comm_buff_t* command_buffer) {
    command_buffer->len = 0;
    command_buffer->cursor_x = 2;
    command_buffer->cursor_y = 0;
    memset(command_buffer->command, 0, BUFF_SIZE);
}

/* Get input from the Input window */
/* @param input_window Window pointer from where we take the input */
char* get_input(WINDOW* input_window) {
    static char input_buff[BUFF_SIZE];
    
    int height, width;
    getmaxyx(input_window, height, width);

    wtimeout(input_window, 50);
    //redraw_input(input_window);

    int ch = wgetch(input_window);

    // command ending posibilities
    if (ch == ERR && !is_program_running()) {
        strncpy(input_buff, "quit", 5);
        return input_buff; // client is shutting down
    } else if (ch == '\n' || ch == KEY_ENTER) {
        strncpy(input_buff, command_buff.command, BUFF_SIZE - 1);
        input_buff[BUFF_SIZE - 1] = '\0';
        if (command_buff.len > 0)
            push_history(&input_state, input_buff);
        return input_buff; // command is ready for sending to the server
    }
    
    if (ch == KEY_UP || ch == KEY_DOWN) { // history navigation handling
        search_history(&input_state, &command_buff, ch);
        redraw_input(input_window);
    }
    
    if (input_state.curr_idx != -1 && ((ch >= 32 && ch <= 126) || ch == KEY_BACKSPACE || ch == 127)) {
        // resetting history navigation in case of editing current input buffer
        input_state.curr_idx = -1;
        input_state.temp_command[0] = '\0';
    }

    if (ch == KEY_BACKSPACE || ch == 127) {
        // current input buffer character erase handling
        if (command_buff.len > 0) {
            command_buff.len--;

            if (command_buff.cursor_x > 1)
                command_buff.cursor_x--;
            else if (command_buff.cursor_y > 0) {
                command_buff.cursor_y--;
                command_buff.cursor_x = width - 2;
            }

            NCURSES_LOCK();
            mvwaddch(input_window, command_buff.cursor_y, command_buff.cursor_x, ' ');
            wmove(input_window, command_buff.cursor_y, command_buff.cursor_x);
            NCURSES_UNLOCK();
            
            command_buff.command[command_buff.len] = '\0';
        }
    } else if (ch >= 32 && ch <= 126 && command_buff.len < BUFF_SIZE - 1) {
        // normal character insertion handling
        command_buff.command[command_buff.len++] = ch;
        command_buff.command[command_buff.len] = '\0';

        NCURSES_LOCK();
        waddch(input_window, ch);
    
        command_buff.cursor_x++;
        if (command_buff.cursor_x > width - 2 && command_buff.cursor_y < height - 2) {
            command_buff.cursor_x = 1;
            command_buff.cursor_y++;
            wmove(input_window, command_buff.cursor_y, command_buff.cursor_x);
        }
        NCURSES_UNLOCK();
    }

    NCURSES_LOCK();
    wrefresh(input_window);
    NCURSES_UNLOCK();

    // if we reach this point, then the input buffer doesn't have a complete command yet
    return NULL;
}

/* Process, Trim & Parse user command */
/* @param input Pointer to the command to be sent to the server */
/* @param tui Pointer to the main TUI struct */
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
            if (get_server_status())
                send_command(tui, "quit");
            add_output_msg(tui->output_terminal, "[CLIENT] Shutting down...", COLOR_MAGENTA);
            set_program_running(false);
            sleep(1);
            shutdown_animation();
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
            send_command(tui, command);
        }
    }
}