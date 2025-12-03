#include "input.h"
#include <string.h>
#include <unistd.h>

/* Initialize the Input and the History for the client */
/* @param input_state The main input state struct*/
void init_input(comm_history_t* input_state) {
    init_history(input_state);
    input_state->buff_len = 0;
    input_state->buff_cursor_x = 2;
    input_state->buff_cursor_y = 0;
    memset(input_state->buff_cmd, 0, BUFF_SIZE);
    memset(input_state->temp_command, 0, BUFF_SIZE);
}

/* Search through the command history */
/* @param input_window Pointer to the input window from the terminal */
/* @param direction The character from the input that will guide the history navigation */
void search_history(WINDOW* input_window, int direction) {
    if (input_state.count == 0)
        return;

    if (input_state.curr_idx == -1 && input_state.buff_len > 0) {
        strncpy(input_state.temp_command, input_state.buff_cmd, BUFF_SIZE - 1);
        input_state.temp_command[BUFF_SIZE - 1] = '\0';
    }

    if (direction == KEY_UP) {
        if (input_state.curr_idx == -1) {
            input_state.curr_idx = input_state.count - 1;
        } else if (input_state.curr_idx > 0) {
            input_state.curr_idx--;
        }
    } else if (direction == KEY_DOWN) {
        if (input_state.curr_idx != -1) {
            if (input_state.curr_idx < input_state.count - 1) {
                input_state.curr_idx++;
            } else {
                input_state.curr_idx = -1;
                if (input_state.temp_command[0] != '\0') {
                    strncpy(input_state.buff_cmd, input_state.temp_command, BUFF_SIZE - 1);
                    input_state.buff_cmd[BUFF_SIZE - 1] = '\0';
                    input_state.buff_len = strlen(input_state.buff_cmd);
                } else {
                    input_state.buff_cmd[0] = '\0';
                    input_state.buff_len = 0;
                }
                
                pthread_mutex_lock(&resize_mutex);
                redraw_input(input_window);
                pthread_mutex_unlock(&resize_mutex);
                return;
            }
        }
    }

    if (input_state.curr_idx >= 0 && input_state.curr_idx < input_state.count) {
        const char* history_cmd = get_history(&input_state, input_state.curr_idx);
        if (history_cmd) {
            strncpy(input_state.buff_cmd, history_cmd, BUFF_SIZE - 1);
            input_state.buff_cmd[BUFF_SIZE - 1] = '\0';
            input_state.buff_len = strlen(input_state.buff_cmd);
            
            pthread_mutex_lock(&resize_mutex);
            redraw_input(input_window);
            pthread_mutex_unlock(&resize_mutex);
        }
    }
}

/* Get input from the Input window */
/* @param input_window Window pointer from where we take the input */
char* get_input(WINDOW* input_window, tui_t* tui) {
    static char input_buff[BUFF_SIZE];
    int ch;

    input_state.buff_len = 0;
    input_state.buff_cmd[0] = '\0';
    input_state.buff_cursor_x = 2;
    input_state.buff_cursor_y = 0;
    input_state.curr_idx = -1;
    input_state.temp_command[0] = '\0';

    int actual_height, actual_width;
    getmaxyx(input_window, actual_height, actual_width);


    wtimeout(input_window, 50);
    
    pthread_mutex_lock(&resize_mutex);
    redraw_input(input_window);
    pthread_mutex_unlock(&resize_mutex);

    int last_lines = LINES, last_cols = COLS;

    while ((ch = wgetch(input_window)) != '\n' && ch != KEY_ENTER && is_program_running()) {
        if (ch == ERR) {
            if (LINES != last_lines || COLS != last_cols) {
                last_lines = LINES;
                last_cols = COLS;
                handle_resize(tui);
            }

            if (tui->debug_mode)
                update_debug_display(tui);
        }
        
        getmaxyx(input_window, actual_height, actual_width);

        if (ch == KEY_UP || ch == KEY_DOWN) {
            search_history(input_window, ch);
            continue;
        }

        if (input_state.curr_idx != -1 && ((ch >= 32 && ch <= 126) || ch == KEY_BACKSPACE || ch == 127)) {
            input_state.curr_idx = -1;
            input_state.temp_command[0] = '\0';
        }

        if (ch == KEY_BACKSPACE || ch == 127) {
            if (input_state.buff_len > 0) {
                input_state.buff_len--;

                pthread_mutex_lock(&resize_mutex);
                if (input_state.buff_cursor_x > 1)
                    input_state.buff_cursor_x--;
                else if (input_state.buff_cursor_y > 0) {
                    input_state.buff_cursor_y--;
                    input_state.buff_cursor_x = actual_width - 2;
                }
                mvwaddch(input_window, input_state.buff_cursor_y, input_state.buff_cursor_x, ' ');
                wmove(input_window, input_state.buff_cursor_y, input_state.buff_cursor_x);
                input_state.buff_cmd[input_state.buff_len] = '\0';
                pthread_mutex_unlock(&resize_mutex);
            }
        } else if (ch >= 32 && ch <= 126 && input_state.buff_len < BUFF_SIZE - 1) {
            input_state.buff_cmd[input_state.buff_len++] = ch;
            input_state.buff_cmd[input_state.buff_len] = '\0';

            pthread_mutex_lock(&resize_mutex);
            waddch(input_window, ch);
            input_state.buff_cursor_x++;
            if (input_state.buff_cursor_x > actual_width - 2 && input_state.buff_cursor_y < actual_height - 2) {
                input_state.buff_cursor_x = 1;
                input_state.buff_cursor_y++;
                wmove(input_window, input_state.buff_cursor_y, input_state.buff_cursor_x);
            }
            pthread_mutex_unlock(&resize_mutex);
        }

        pthread_mutex_lock(&resize_mutex);
        wrefresh(input_window);
        pthread_mutex_unlock(&resize_mutex);
    }

    if (!is_program_running()) {
        strncpy(input_buff, "quit", 5);
        return input_buff;
    }

    strncpy(input_buff, input_state.buff_cmd, BUFF_SIZE - 1);
    input_buff[BUFF_SIZE - 1] = '\0';
    if (input_state.buff_len > 0) {
        push_history(&input_state, input_buff);
    }

    return input_buff;
}