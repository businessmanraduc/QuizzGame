#ifndef TUI_H
#define TUI_H

#include <stdbool.h>
#include <ncurses.h>
#include "utils.h"
#include "history.h"

extern comm_history_t input_state;
extern comm_buff_t command_buff;

void init_tui(tui_t*, const char*);
void cleanup_tui(tui_t*);
void clear_screen(WINDOW*);
void redraw_input(WINDOW*);
void redraw_output(WINDOW*);
void redraw_debug(WINDOW*, WINDOW*);
void resize_client(tui_t*);
void update_debug_display(WINDOW*);
void enable_debug(tui_t*);
void disable_debug(tui_t*);
void shutdown_animation();

#endif
