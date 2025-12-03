#include "history.h"
#include <string.h>
#include <unistd.h>
#include <ncurses.h>

/* Initialize history of commands */
/* @param history pointer to the global history table */
void init_history(comm_history_t* history) {
    history->start_idx = 0;
    history->curr_idx = 0;
    history->count = 0;
    memset(history->temp_command, 0, BUFF_SIZE);
    for (int i = 0; i < HISTORY_SIZE; i++)
        history->commands[i][0] = '\0';
}

/* Push a new command into history table */
/* @param history pointer to the global history table */
/* @param new_command pointer to the command to be pushed */
void push_history(comm_history_t* history, const char* new_command) {
    if (strlen(new_command) == 0)
        return;

    if (history->count > 0) {
        int last_index = (history->start_idx + history->count - 1) & 0x7F;
        if (strcmp(history->commands[last_index], new_command) == 0)
            return;
    }

    int new_index = (history->start_idx + history->count) & 0x7F;
    strncpy(history->commands[new_index], new_command, BUFF_SIZE - 1);
    history->commands[new_index][BUFF_SIZE - 1] = '\0';

    if (history->count < HISTORY_SIZE)
        history->count++;
    else {
        history->start_idx = (history->start_idx + 1) & 0x7F;
    }

    history->curr_idx = -1;
}

/* Get a command from history at a certain index*/
/* @param history pointer to the global history table*/
/* @param index logical index inside the history table*/
const char* get_history(comm_history_t* history, int index) {
    if (index < 0 || index >= history->count)
        return NULL;

    if (history->count < HISTORY_SIZE)
        return history->commands[index];
    else {
        int phys_index = (history->start_idx + index) & 0x7F;
        return history->commands[phys_index];
    }
}

/* Search through the command history */
/* @param history pointer to the global history table */
/* @param command_buff pointer to the command buffer, where the result of the history search will be put */
/* @param direction The character from the input that will guide the history search direction(UP/DOWN) */
void search_history(comm_history_t* history, comm_buff_t* command_buff, int direction) {
    if (history->count == 0)
        return;

    if (history->curr_idx == -1 && command_buff->len > 0) {
        strncpy(history->temp_command, command_buff->command, BUFF_SIZE - 1);
        history->temp_command[BUFF_SIZE - 1] = '\0';
    }

    if (direction == KEY_UP) {
        if (history->curr_idx == -1) {
            history->curr_idx = history->count - 1;
        } else if (history->curr_idx > 0) {
            history->curr_idx--;
        }
    } else if (direction == KEY_DOWN) {
        if (history->curr_idx != -1) {
            if (history->curr_idx < history->count - 1) {
                history->curr_idx++;
            } else {
                history->curr_idx = -1;
                if (history->temp_command[0] != '\0') {
                    strncpy(command_buff->command, history->temp_command, BUFF_SIZE - 1);
                    command_buff->command[BUFF_SIZE - 1] = '\0';
                    command_buff->len = strlen(command_buff->command);
                } else {
                    command_buff->command[0] = '\0';
                    command_buff->len = 0;
                }
                
                // pthread_mutex_lock(&resize_mutex);
                // redraw_input(input_window);
                // pthread_mutex_unlock(&resize_mutex);
                return;
            }
        }
    }

    if (history->curr_idx >= 0 && history->curr_idx < history->count) {
        const char* history_cmd = get_history(history, history->curr_idx);
        if (history_cmd) {
            strncpy(command_buff->command, history_cmd, BUFF_SIZE - 1);
            command_buff->command[BUFF_SIZE - 1] = '\0';
            command_buff->len = strlen(command_buff->command);
            
            // pthread_mutex_lock(&resize_mutex);
            // redraw_input(input_window);
            // pthread_mutex_unlock(&resize_mutex);
        }
    }
}

