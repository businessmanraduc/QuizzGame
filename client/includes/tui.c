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
    int total_width = COLS;
    tui->upper_height = total_height * 2 / 3;
    tui->lower_height = total_height - tui->upper_height;
    tui->left_section_width = total_width;
    tui->right_section_width = total_width / 3;

    tui->output = newwin(tui->upper_height, total_width, 0, 0);
    tui->input = newwin(tui->lower_height, total_width, tui->upper_height, 0);
    tui->output_terminal = newwin(tui->upper_height - 2, total_width - 2, 1, 1);
    tui->input_terminal = newwin(tui->lower_height - 2, total_width - 2, tui->upper_height + 1, 1);
    tui->debug = NULL;
    tui->debug_terminal = NULL;
    tui->debug_mode = false;

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

/* Cleanup/Quit Terminal User Interface */
/* @param tui Pointer to the main TUI struct */
void cleanup_tui(tui_t* tui) {
    if (tui->output_terminal) delwin(tui->output_terminal);
    if (tui->output) delwin(tui->output);
    if (tui->debug_terminal) delwin(tui->debug_terminal);
    if (tui->debug) delwin(tui->debug);
    if (tui->input_terminal) delwin(tui->input_terminal);
    if (tui->input) delwin(tui->input);
    endwin();
}

/* Clear screen buffer */
/* @param window Pointer to the window to be cleared */
void clear_screen(WINDOW* window) {
    NCURSES_LOCK();
    werase(window);
    wrefresh(window);
    output_state.count = 0;
    output_state.start = 0;
    NCURSES_UNLOCK();
}

/* Redraw Input Window */
/* @param input_window Pointer to the window which needs a redraw */
void redraw_input(WINDOW* input_window) {
    NCURSES_LOCK();
    werase(input_window);

    int height, width;
    getmaxyx(input_window, height, width);
    if (height <= 0 || width <= 0) return;
    
    mvwprintw(input_window, 0, 1, "> ");
    int x = 3, y = 0;
    if (command_buff.len > 0) {
        int max_chars = (command_buff.len < BUFF_SIZE - 1) ? command_buff.len : BUFF_SIZE - 1;

        for (int i = 0; i < max_chars; i++) {
            if (x >= width - 1 || x < 0) {
                x = 1;
                y++;
                if (y >= height) break;
            }

            if (command_buff.command[i] != '\0')
                mvwaddch(input_window, y, x, command_buff.command[i]);
            x++;
        }
    }

    command_buff.cursor_x = (x < width && x >= 0) ? x : (width > 1 ? width - 1 : 0);
    command_buff.cursor_y = (y < height && y >= 0) ? y : (height > 1 ? height - 1 : 0);

    if (command_buff.cursor_x >= width)
        command_buff.cursor_x = width - 1;
    if (command_buff.cursor_y >= height)
        command_buff.cursor_y = height - 1;

    wmove(input_window, command_buff.cursor_y, command_buff.cursor_x);
    wrefresh(input_window);

    NCURSES_UNLOCK();
}

/* Redraw Output Window */
/* @param output_window Pointer to the window which needs a redraw */
void redraw_output(WINDOW* output_window) {
    NCURSES_LOCK();

    werase(output_window);
    for (int i = 0; i < output_state.count; i++) {
        int idx = (output_state.start + i) & 0x1F;
        if (has_colors())wattron(output_window, COLOR_PAIR(output_state.colors[idx]));
        wprintw(output_window, "%s\n", output_state.messages[idx]);
        if (has_colors())
            wattroff(output_window, COLOR_PAIR(output_state.colors[idx]));
    }
    wrefresh(output_window);

    NCURSES_UNLOCK();
}

/* Redraw Debug Window */
/* @param debug_window Pointer to the window which needs a redraw */
/* @param input_window Pointer to the input window, where we redirect the cursor when done */
void redraw_debug(WINDOW* debug_window, WINDOW* input_window) {
    NCURSES_LOCK();

    werase(debug_window);
    int line = 0;
    int debug_width = getmaxx(debug_window);

    mvwprintw(debug_window, line++, 1, "Server Status:");
    mvwprintw(debug_window, line++, 1, " - Online: %s", get_server_status() ? "YES" : "NO");
    mvwprintw(debug_window, line++, 1, " - Logged In: %s", client_state.loggedIn ? "YES" : "NO");
    if (client_state.loggedIn)
        mvwprintw(debug_window, line++, 1, " - Username: %s", client_state.client_name);
    
    line++;
    mvwprintw(debug_window, line++, 1, "Input State:");
    mvwprintw(debug_window, line++, 1, " - Has Input: %s", command_buff.len ? "YES" : "NO");
    mvwprintw(debug_window, line++, 1, " - Cursor Pos: (%d, %d)", command_buff.cursor_x, command_buff.cursor_y);
    if (command_buff.len)
        mvwprintw(debug_window, line++, 1, " - Buff Len: %d", command_buff.len);

    line++;
    refresh();
    mvwprintw(debug_window, line++, 1, "Terminal Sizes:");
    mvwprintw(debug_window, line++, 1, " - Total Width: %d", COLS);
    mvwprintw(debug_window, line++, 1, " - Total Height: %d", LINES);
    
    line++;
    mvwprintw(debug_window, line++, 1, "Command History:");
    comm_history_t* history = &input_state;
    int show_count = (history->count < 8) ? history->count : 8;
    for (int i = 0; i < show_count; i++) {
        int hist_index = history->count - 1 - i;
        const char* cmd = get_history(history, hist_index);
        if (cmd) {
            char cmd_preview[debug_width - 4];
            strncpy(cmd_preview, cmd, debug_width - 5);
            cmd_preview[debug_width - 5] = '\0';
            mvwprintw(debug_window, line++, 2, "%d: %s", i + 1, cmd_preview);
        }
    }

    wrefresh(debug_window);
    wmove(input_window, command_buff.cursor_y, command_buff.cursor_x);
    wrefresh(input_window); // move cursor back to Input Window after Debug redraw

    NCURSES_UNLOCK();
}

/* Main Resize Handler and re-Structuring function */
/* @param tui Pointer to the main TUI struct */
void resize_client(tui_t* tui) {
    NCURSES_LOCK();

    def_prog_mode();
    endwin();
    refresh();
    reset_prog_mode();
    refresh();

    int total_height = LINES;
    int total_width = COLS;
    tui->upper_height = total_height * 2 / 3;
    tui->lower_height = total_height - tui->upper_height;
    tui->left_section_width = total_width; // will change if debug-mode is on
    tui->right_section_width = total_width / 3;

    if (tui->debug_mode) {
        tui->left_section_width = total_width * 2 / 3; // make space for Debug Window on the right
        werase(tui->debug);
        werase(tui->debug_terminal);
        wresize(tui->debug, total_height, tui->right_section_width - 1);
        wresize(tui->debug_terminal, total_height - 2, tui->right_section_width - 3);

        mvwin(tui->debug, 0, tui->left_section_width + 1);
        mvwin(tui->debug_terminal, 1, tui->left_section_width + 2);
        box(tui->debug, 0, 0);
        mvwprintw(tui->debug, 0, 2, " Debug ");
        for (int i = 0; i < total_height; i++)
            mvaddch(i, tui->left_section_width, ' ');

        refresh();
        wrefresh(tui->debug);
        wrefresh(tui->debug_terminal);
    }

    wresize(tui->output, tui->upper_height, tui->left_section_width);
    wresize(tui->input, tui->lower_height, tui->left_section_width);
    mvwin(tui->input, tui->upper_height, 0);

    wresize(tui->output_terminal, tui->upper_height - 2, tui->left_section_width - 2);
    mvwin(tui->output_terminal, 1, 1);
    wresize(tui->input_terminal, tui->lower_height - 2, tui->left_section_width - 2);
    mvwin(tui->input_terminal, tui->upper_height + 1, 1);

    werase(tui->output);
    box(tui->output, 0, 0);
    mvwprintw(tui->output, 0, 2, " Output ");

    werase(tui->input);
    box(tui->input, 0, 0);
    mvwprintw(tui->input, 0, 2, " Input ");

    touchwin(tui->output);
    touchwin(tui->input);
    touchwin(tui->output_terminal);
    touchwin(tui->input_terminal);

    wrefresh(tui->output);
    wrefresh(tui->input);

    NCURSES_UNLOCK(); // the following functions already have their own locking mechanism on ncurses

    redraw_input(tui->input_terminal);
    redraw_output(tui->output_terminal);

    set_resize_req(false);
}

/* Enable Debug-Mode */
/* @param tui Pointer to the main TUI struct */
void enable_debug(tui_t* tui) {
    add_output_msg(tui->output_terminal, "[CLIENT] Debug Mode activated.", COLOR_CLIENT);

    NCURSES_LOCK();
    tui->debug_mode = true;
    tui->debug = newwin(LINES, COLS / 3, COLS * 2 / 3, 0);
    tui->debug_terminal = newwin(LINES - 2, COLS / 3 - 2, COLS * 2 / 3 + 1, 1);
    werase(stdscr);
    NCURSES_UNLOCK();

    resize_client(tui);
}

/* Disable Debug-Mode */
/* @param tui Pointer to the main TUI struct */
void disable_debug(tui_t* tui) {
    add_output_msg(tui->output_terminal, "[CLIENT] Debug Mode deactivated.", COLOR_CLIENT);

    NCURSES_LOCK();
    tui->debug_mode = false;
    wclear(tui->debug_terminal);
    wclear(tui->debug);
    delwin(tui->debug_terminal);
    delwin(tui->debug);
    NCURSES_UNLOCK();

    resize_client(tui);
}

/* Terminal Shutdown Animation - to be played whenever the client is shutting down */
void shutdown_animation() {
    NCURSES_LOCK();

    def_prog_mode();
    endwin();
    
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    reset_prog_mode();
    refresh();
    
    for (int wave = 0; wave < rows + cols; wave += 2) {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (i + j < wave) {
                    if (i + j > wave - 5) {
                        char tail_chars[] = { '.', '~', '*' };
                        mvaddch(i, j, tail_chars[(i + j) % 3]);
                    } else {
                        mvaddch(i, j, ' ');
                    }
                }
            }
        }
        refresh();
        usleep(40000);
    }
    
    clear();
    refresh();

    NCURSES_UNLOCK();
}