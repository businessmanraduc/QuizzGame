#ifndef TUI_H
#define TUI_H

#include <ncurses.h>
#include <stdbool.h>
#include "colors.h"
#include "input.h"

typedef struct {
    WINDOW *output_win;
    WINDOW *output_terminal;
    WINDOW *output_debug;
    WINDOW *input_win;
    int output_height;
    int input_height;
    int total_width;
    bool debug_mode;
} tui_t;

extern input_state_t current_input_state;

void enable_debug_mode(tui_t* tui);
void disable_debug_mode(tui_t* tui);
void update_debug_display(tui_t* tui);
void* debug_monitor_thread(void* arg);

void initialize_tui(tui_t *tui);
void cleanup_tui(tui_t *tui);
void clear_output_screen(tui_t* tui);
void add_output_message(tui_t *tui, const char *text, output_color_t color);
void handle_window_resize(tui_t *tui);
void* monitor_resize_thread(void* arg);
void terminal_shutdown_animation(void);

#endif