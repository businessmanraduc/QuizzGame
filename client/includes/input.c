#include "input.h"
#include <string.h>
#include <unistd.h>

void init_input(comm_buff_t* command_buffer) {
    command_buffer->len = 0;
    command_buffer->cursor_x = 2;
    command_buffer->cursor_y = 0;
    memset(command_buffer->command, 0, BUFF_SIZE);
}

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