#ifndef INPUT_H
#define INPUT_H

#include <ncurses.h>
#include <stdbool.h>
#include "utils.h"
#include "tui.h"

void init_input(comm_history_t*);
char* get_input(WINDOW*);
#endif
