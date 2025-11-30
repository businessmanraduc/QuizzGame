#ifndef INPUT_H
#define INPUT_H

#include <ncurses.h>
#include <stdbool.h>

#define BUFF_SIZE 1024
#define HISTORY_SIZE 128

typedef struct {
    char commands[HISTORY_SIZE][BUFF_SIZE];
    int start;
    int count;
    int current_index;
    char temp_command[BUFF_SIZE];
    bool is_full;
} command_history_t;

typedef struct input_state {
    char buff[BUFF_SIZE];
    int cursor_pos;
    bool has_input;
    int cursor_y, cursor_x;
    int prev_cursor_x;
    command_history_t history;
} input_state_t;

void initialize_history(command_history_t* history);
void add_to_history(command_history_t* history, const char* command);
const char* get_history_command(const command_history_t* history, int logical_index);
void navigate_history(WINDOW* input_win, input_state_t* state, int direction, int input_height, int total_width);
void initialize_input_state(input_state_t* state);
char* get_user_input(WINDOW* input_win, input_state_t* state, int input_height, int total_width);
void restore_input_display(WINDOW* input_win, input_state_t* state, int total_width, int input_height);

#endif