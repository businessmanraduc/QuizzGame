#include "tui.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

comm_history_t input_state = {0};
comm_buff_t command_buff = {0};

/* Initialize Terminal User Interface */
/* @param tui Pointer to the main TUI struct */
void init_tui(tui_t* tui) {
    initscr();
    timeout(50);
    noecho();
    keypad(stdscr, TRUE);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(COLOR_DEFAULT, COLOR_WHITE, -1);
        init_pair(COLOR_ERROR, COLOR_RED, -1);
        init_pair(COLOR_WARNING, COLOR_YELLOW, -1);
        init_pair(COLOR_SUCCESS, COLOR_GREEN, -1);
        init_pair(COLOR_INFO, COLOR_CYAN, -1);
        init_pair(COLOR_SERVER, COLOR_MAGENTA, -1);
        init_pair(COLOR_CLIENT, COLOR_BLUE, -1);
    }
    refresh();

    int total_height = LINES;
    tui->total_width = COLS;
    tui->output_height = total_height * 2 / 3;
    tui->input_height = total_height - tui->output_height;

    tui->output = newwin(tui->output_height, tui->total_width, 0, 0);
    tui->input = newwin(tui->input_height, tui->total_width, tui->output_height, 0);
    tui->output_terminal = newwin(tui->output_height - 2, tui->total_width - 2, 1, 1);
    tui->input_terminal = newwin(tui->input_height - 2, tui->total_width - 2, tui->output_height + 1, 1);

    scrollok(tui->output_terminal, TRUE);
    scrollok(tui->input_terminal, TRUE);
    keypad(tui->input_terminal, TRUE);

    box(tui->output, 0, 0);
    box(tui->input, 0, 0);
    mvwprintw(tui->output, 0, 2, " Output ");
    mvwprintw(tui->input, 0, 2, " Input ");

    wrefresh(tui->output);
    wrefresh(tui->input);
}