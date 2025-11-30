#include "input.h"
#include "utils.h"
#include <string.h>
#include <unistd.h>

void initialize_history(command_history_t* history) {
    history->start = 0;
    history->count = 0;
    history->current_index = -1;
    history->temp_command[0] = '\0';
    history->is_full = false;
    for (int i = 0; i < HISTORY_SIZE; i++) {
        history->commands[i][0] = '\0';
    }
}

void add_to_history(command_history_t* history, const char* command) {
    if (strlen(command) == 0) return;

    if (history->count > 0) {
        int last_index = (history->start + history->count - 1) & 0b01111111;
        if (strcmp(history->commands[last_index], command) == 0) {
            return;
        }
    }

    int new_index = (history->start + history->count) & 0b01111111;
    strncpy(history->commands[new_index], command, BUFF_SIZE - 1);
    history->commands[new_index][BUFF_SIZE - 1] = '\0';
    
    if (history->count < HISTORY_SIZE) {
        history->count++;
    } else {
        history->start = (history->start + 1) & 0b01111111;
        history->is_full = true;
    }
    
    history->current_index = -1;
}

const char* get_history_command(const command_history_t* history, int logical_index) {
    if (logical_index < 0 || logical_index >= history->count) {
        return NULL;
    }
    
    if (!history->is_full) {
        return history->commands[logical_index];
    } else {
        int physical_index = (history->start + logical_index) & 0b01111111;
        return history->commands[physical_index];
    }
}

void navigate_history(WINDOW* input_win, input_state_t* state, int direction, int input_height, int total_width) {
    command_history_t* history = &state->history;
    
    if (history->count == 0) return;
    
    if (history->current_index == -1 && state->has_input) {
        strncpy(history->temp_command, state->buff, BUFF_SIZE - 1);
        history->temp_command[BUFF_SIZE - 1] = '\0';
    }

    if (direction == KEY_UP) {
        if (history->current_index == -1) {
            history->current_index = history->count - 1;
        } else if (history->current_index > 0) {
            history->current_index--;
        }
    } else if (direction == KEY_DOWN) {
        if (history->current_index != -1) {
            if (history->current_index < history->count - 1) {
                history->current_index++;
            } else {
                history->current_index = -1;
                if (history->temp_command[0] != '\0') {
                    strncpy(state->buff, history->temp_command, BUFF_SIZE - 1);
                    state->buff[BUFF_SIZE - 1] = '\0';
                } else {
                    state->buff[0] = '\0';
                }
                state->cursor_pos = strlen(state->buff);
                state->has_input = (state->cursor_pos > 0);
                
                pthread_mutex_lock(&resize_mutex);
                restore_input_display(input_win, state, total_width, input_height);
                pthread_mutex_unlock(&resize_mutex);
                return;
            }
        }
    }
    
    if (history->current_index >= 0 && history->current_index < history->count) {
        const char* history_cmd = get_history_command(history, history->current_index);
        if (history_cmd) {
            strncpy(state->buff, history_cmd, BUFF_SIZE - 1);
            state->buff[BUFF_SIZE - 1] = '\0';
            state->cursor_pos = strlen(state->buff);
            state->has_input = (state->cursor_pos > 0);
            
            pthread_mutex_lock(&resize_mutex);
            restore_input_display(input_win, state, total_width, input_height);
            pthread_mutex_unlock(&resize_mutex);
        }
    }
}

void initialize_input_state(input_state_t* state) {
    memset(state->buff, 0, BUFF_SIZE);
    state->cursor_pos = 0;
    state->has_input = false;
    state->cursor_y = 1;
    state->cursor_x = 3;
    state->prev_cursor_x = 0;
    initialize_history(&state->history);
}

void restore_input_display(WINDOW* input_win, input_state_t* state, int total_width, int input_height) {
    werase(input_win);
    box(input_win, 0, 0);
    mvwprintw(input_win, 0, 2, " Input ");
    mvwprintw(input_win, 1, 1, "> ");
    
    int y = 1, x = 3;
    if (state->has_input && state->cursor_pos > 0) {
        for (int i = 0; i < state->cursor_pos; i++) {
            wmove(input_win, y, x);
            if (state->buff[i] == '\\') {
                y++, x = 2;
            }
            waddch(input_win, state->buff[i]);
            x++;
            if (x >= total_width - 2) {
                x = 3, y++;
                if (y >= input_height - 1) break;
            }
        }
    }
    state->cursor_y = y;
    state->cursor_x = x;
    wrefresh(input_win);
}

char* get_user_input(WINDOW* input_win, input_state_t* state, int input_height, int total_width) {
    static char input_buffer[BUFF_SIZE];
    int ch;
    int pos = 0;
    
    pthread_mutex_lock(&input_state_mutex);
    state->has_input = false;
    state->cursor_pos = 0;
    state->buff[0] = '\0';
    state->cursor_y = 1;
    state->cursor_x = 3;
    state->history.current_index = -1;
    state->history.temp_command[0] = '\0';
    pthread_mutex_unlock(&input_state_mutex);

    pthread_mutex_lock(&resize_mutex);
    restore_input_display(input_win, state, total_width, input_height);
    pthread_mutex_unlock(&resize_mutex);

    while ((ch = wgetch(input_win)) != '\n' && ch != KEY_ENTER && is_program_running()) {
        total_width = COLS;

        if (ch == KEY_UP || ch == KEY_DOWN) {
            pthread_mutex_lock(&input_state_mutex);
            navigate_history(input_win, state, ch, input_height, total_width);
            pos = state->cursor_pos;
            strncpy(input_buffer, state->buff, BUFF_SIZE - 1);
            input_buffer[BUFF_SIZE - 1] = '\0';
            pthread_mutex_unlock(&input_state_mutex);
            continue;
        }
        
        pthread_mutex_lock(&input_state_mutex);
        if (state->history.current_index != -1 && ((ch >= 32 && ch <= 126) || ch == KEY_BACKSPACE || ch == 127)) {
            state->history.current_index = -1;
            state->history.temp_command[0] = '\0';
        }
        pthread_mutex_unlock(&input_state_mutex);

        if (ch == KEY_BACKSPACE || ch == 127) {
            pthread_mutex_lock(&input_state_mutex);
            if (pos > 0) {
                pos--;
                state->cursor_pos--;

                pthread_mutex_lock(&resize_mutex);
                if (state->cursor_x > 3) {
                    state->cursor_x--;
                } else if (state->cursor_y > 1) {
                    state->cursor_y--;
                    if (state->buff[state->cursor_pos] == '\\')
                        state->cursor_x = state->prev_cursor_x, pos++;
                    else
                        state->cursor_x = total_width - 3;
                }
                mvwaddch(input_win, state->cursor_y, state->cursor_x, ' ');
                wmove(input_win, state->cursor_y, state->cursor_x);
                state->buff[state->cursor_pos] = '\0';
                state->has_input = (state->cursor_pos > 0);
                pthread_mutex_unlock(&resize_mutex);
            }
            pthread_mutex_unlock(&input_state_mutex);
        } else if (ch == '\\') {
            pthread_mutex_lock(&input_state_mutex);
            state->buff[state->cursor_pos++] = ch;
            state->prev_cursor_x = state->cursor_x;
            state->buff[state->cursor_pos] = '\0';
            state->has_input = true;
            pthread_mutex_unlock(&input_state_mutex);

            pthread_mutex_lock(&resize_mutex);
            waddch(input_win, ch);
            if (state->cursor_y < input_height - 2) {
                state->cursor_x = 3;
                state->cursor_y++;
                wmove(input_win, state->cursor_y, state->cursor_x);
            }

            input_buffer[pos] = '\0';
            pthread_mutex_unlock(&resize_mutex);
        } else if (ch >= 32 && ch <= 126 && pos < (int)sizeof(input_buffer) - 1) {
            input_buffer[pos++] = ch;

            pthread_mutex_lock(&input_state_mutex);
            state->buff[state->cursor_pos++] = ch;
            state->buff[state->cursor_pos] = '\0';
            state->has_input = true;
            pthread_mutex_unlock(&input_state_mutex);

            pthread_mutex_lock(&resize_mutex);
            
            waddch(input_win, ch);
            state->cursor_x++;
            if (state->cursor_x > total_width - 3 && state->cursor_y < input_height - 2) {
                    state->cursor_x = 3;
                    state->cursor_y++;
                    wmove(input_win, state->cursor_y, state->cursor_x);
            }
            
            input_buffer[pos] = '\0';
            pthread_mutex_unlock(&resize_mutex);
        }

        pthread_mutex_lock(&resize_mutex);
        wrefresh(input_win);
        pthread_mutex_unlock(&resize_mutex);
        usleep(1000);
    }
    
    input_buffer[pos] = '\0';
    if (pos > 0) {
        pthread_mutex_lock(&input_state_mutex);
        add_to_history(&state->history, input_buffer);
        pthread_mutex_unlock(&input_state_mutex);
    }
    
    return input_buffer;
}