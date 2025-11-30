#ifndef TUI_H
#define TUI_H

#include <ncurses.h>
#include <stdbool.h>
#include "utils.h"
#include "history.h"

#define MAX_MESSAGES 32

typedef struct {
    char messages[MAX_MESSAGES][BUFF_SIZE];
    output_color_t colors[MAX_MESSAGES];
    int count;
    int start;
} output_state_t;

typedef struct {
    WINDOW* output;
    WINDOW* output_terminal;
    WINDOW* debug;
    WINDOW* debug_terminal;
    WINDOW* input;
    WINDOW* input_terminal;
    int output_height;
    int input_height;
    int total_width;
    bool debug_mode;
} tui_t;

extern comm_history_t input_state;

void init_tui(tui_t*);
void cleanup_tui(tui_t*);
void clear_screen(WINDOW*);
void add_output_msg(WINDOW*, const char*, output_color_t);
void redraw_input(WINDOW*);
void handle_resize(tui_t*);
void* resize_thread(void*);
void update_debug_display(tui_t*);
void enable_debug(tui_t*);
void disable_debug(tui_t*);
void terminal_shutdown_animation();

#endif