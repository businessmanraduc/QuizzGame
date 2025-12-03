#ifndef INPUT_H
#define INPUT_H

#include <ncurses.h>
#include <stdbool.h>
#include "utils.h"
#include "tui.h"

void init_input(comm_buff_t*);
char* get_input(WINDOW*);
void process_user_command(const char*, tui_t*);

#endif