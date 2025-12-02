#ifndef HISTORY_H
#define HISTORY_H

#include "utils.h"

typedef struct {
    char commands[HISTORY_SIZE][BUFF_SIZE]; // commands history table
    int count; // number of saved commands
    int start_idx; // bottom of the history
    int curr_idx; // current end of history
    char temp_command[BUFF_SIZE]; // temporary buffer, used by search_history when temp. input is found
} comm_history_t;

typedef struct {
    int len; // length of the current (writeable) command buffer
    int cursor_x, cursor_y; // cursor positions (x and y) inside the Input Window
    char command[BUFF_SIZE]; // (writeable) command buffer
} comm_buff_t;

void init_history(comm_history_t*);
void push_history(comm_history_t*, const char*);
const char* get_history(comm_history_t*, int);
void search_history(comm_history_t*, comm_buff_t*, int);

#endif