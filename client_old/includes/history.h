#ifndef HISTORY_H
#define HISTORY_H

#include "utils.h"

typedef struct {
    char commands[HISTORY_SIZE][BUFF_SIZE];
    int count; // number of saved commands
    int start_idx; // bottom of the history
    int curr_idx; // current end of history
    char temp_command[BUFF_SIZE];
    char buff_cmd[BUFF_SIZE];
    int buff_len; // buffered command length
    int buff_cursor_x; // buffered cursor x
    int buff_cursor_y; // buffered cursor y
} comm_history_t;

void init_history(comm_history_t*);
void push_history(comm_history_t*, const char*);
const char* get_history(comm_history_t*, int);

#endif