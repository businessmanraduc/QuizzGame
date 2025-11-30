#include "tui.h"
#include "network.h"
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

static output_state_t output_state = {0};
comm_history_t input_state = {0};

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

/* Cleanup/Quit Terminal User Interface */
/* @param tui Pointer to the main TUI struct */
void cleanup_tui(tui_t *tui) {
    if (tui->output_terminal) delwin(tui->output_terminal);
    if (tui->output) delwin(tui->output);
    if (tui->input_terminal) delwin(tui->input_terminal);
    if (tui->input) delwin(tui->input);
    endwin();
}

/* Clear screen buffer */
/* @param window Pointer to the window to be cleared */
void clear_screen(WINDOW* window) {
    pthread_mutex_lock(&resize_mutex);
    werase(window);
    wrefresh(window);
    output_state.count = 0;
    output_state.start = 0;
    pthread_mutex_unlock(&resize_mutex);
}

/* Add output message to the screen */
/* @param output_window Pointer to the output window */
/* @param msg Message to be printed */
/* @param color Color to be used for the message */
void add_output_msg(WINDOW* output_window, const char* msg, output_color_t color) {
    pthread_mutex_lock(&resize_mutex);

    if (output_state.count < MAX_MESSAGES) {
        strncpy(output_state.messages[output_state.count], msg, BUFF_SIZE - 1);
        output_state.colors[output_state.count] = color;
        output_state.count++;
    } else {
        strncpy(output_state.messages[output_state.start], msg, BUFF_SIZE - 1);
        output_state.colors[output_state.start] = color;
        output_state.start = (output_state.start + 1) & 0x1F;
    }

    if (has_colors())
        wattron(output_window, COLOR_PAIR(color));
    wprintw(output_window, "%s\n", msg);
    if (has_colors())
        wattroff(output_window, COLOR_PAIR(color));
    wrefresh(output_window);
    pthread_mutex_unlock(&resize_mutex);
}

/* Redraw Input Window */
/* @param input_window Pointer to the window which needs a redraw */
void redraw_input(WINDOW* input_window) {
    werase(input_window);
    
    int actual_height, actual_width;
    getmaxyx(input_window, actual_height, actual_width);
    if (actual_height <= 0 || actual_width <= 0) {
        return;
    }
    
    mvwprintw(input_window, 0, 1, "> ");
    int x = 3, y = 0;
    
    if (input_state.buff_len > 0) {
        int max_chars = input_state.buff_len;
        if (max_chars > BUFF_SIZE - 1) {
            max_chars = BUFF_SIZE - 1;
        }
        
        for (int i = 0; i < max_chars; i++) {
            if (y >= actual_height || y < 0) {
                break;
            }
            if (x >= actual_width - 1 || x < 0) {
                x = 1;
                y++;
                if (y >= actual_height) break;
            }
            
            if (input_state.buff_cmd[i] != '\0') {
                mvwaddch(input_window, y, x, input_state.buff_cmd[i]);
            }
            x++;
        }
    }
    
    input_state.buff_cursor_x = (x < actual_width && x >= 0) ? x : (actual_width > 1 ? actual_width - 1 : 0);
    input_state.buff_cursor_y = (y < actual_height && y >= 0) ? y : (actual_height > 1 ? actual_height - 1 : 0);
    
    if (input_state.buff_cursor_y >= actual_height) {
        input_state.buff_cursor_y = actual_height - 1;
    }
    if (input_state.buff_cursor_x >= actual_width) {
        input_state.buff_cursor_x = actual_width - 1;
    }
    
    wmove(input_window, input_state.buff_cursor_y, input_state.buff_cursor_x);
    wrefresh(input_window);
}

/* Redraw Output Window */
/* @param output_window Pointer to the window which needs a redraw */
void redraw_output(WINDOW* output_window) {
    werase(output_window);
    for (int i = 0; i < output_state.count; i++) {
        int idx = (output_state.start + i) & 0x1F;
        if (has_colors())
            wattron(output_window, COLOR_PAIR(output_state.colors[idx]));
        wprintw(output_window, "%s\n", output_state.messages[idx]);
        if (has_colors())
            wattroff(output_window, COLOR_PAIR(output_state.colors[idx]));
    }
    wrefresh(output_window);
}

/* Main Handle Resize and re-Structuring function */
/* @param tui Pointer to the main TUI struct */
void handle_resize(tui_t* tui) {
    pthread_mutex_lock(&resize_mutex);

    reset_prog_mode();
    refresh();

    int total_height = LINES;
    tui->total_width = COLS;
    tui->output_height = total_height * 2 / 3;
    tui->input_height = total_height - tui->output_height;

    if (tui->debug_mode) {
        tui->total_width = COLS * 2 / 3;
        werase(tui->debug);
        werase(tui->debug_terminal);
        for (int i = 0; i < total_height; i++)
            mvaddch(i, tui->total_width, 'm');
        wresize(tui->debug, LINES, COLS / 3 - 1);
        wresize(tui->debug_terminal, LINES - 2, COLS / 3 - 3);
        mvwin(tui->debug, 0, tui->total_width + 1);
        mvwin(tui->debug_terminal, 1, tui->total_width + 2);
        box(tui->debug, 0, 0);
        mvwprintw(tui->debug, 0, 2, " Debug ");
        for (int i = 0; i < total_height; i++)
            mvaddch(i, tui->total_width, ' ');
        refresh();
        wrefresh(tui->debug);
        wrefresh(tui->debug_terminal);
    }

    wresize(tui->output, tui->output_height, tui->total_width);
    wresize(tui->input, tui->input_height, tui->total_width);
    mvwin(tui->input, tui->output_height, 0);

    wresize(tui->output_terminal, tui->output_height - 2, tui->total_width - 2);
    mvwin(tui->output_terminal, 1, 1);
    wresize(tui->input_terminal, tui->input_height - 2, tui->total_width - 2);
    mvwin(tui->input_terminal, tui->output_height + 1, 1);

    werase(tui->output);
    box(tui->output, 0, 0);
    mvwprintw(tui->output, 0, 2, " Output ");

    werase(tui->input);
    box(tui->input, 0, 0);
    mvwprintw(tui->input, 0, 2, " Input ");

    touchwin(tui->output);
    touchwin(tui->input);
    if (tui->output_terminal) touchwin(tui->output_terminal);
    if (tui->input_terminal) touchwin(tui->input_terminal);

    wrefresh(tui->output);
    wrefresh(tui->input);
    if (tui->output_terminal) wrefresh(tui->output_terminal);
    if (tui->input_terminal) wrefresh(tui->input_terminal);

    redraw_output(tui->output_terminal);
    redraw_input(tui->input_terminal);

    pthread_mutex_unlock(&resize_mutex);
}

/* Resize Thread */
/* @param arg Pointer which should point towards a tui_t struct */
void* resize_thread(void* arg) {
    tui_t* tui = (tui_t*)arg;
    int last_lines = LINES;
    int last_cols = COLS;
    add_output_msg(tui->output_terminal, "[RESIZE_THREAD] Booting up...", COLOR_INFO);
    while (is_program_running()) {
        if (LINES != last_lines || COLS != last_cols) {
            last_lines = LINES;
            last_cols = COLS;
            handle_resize(tui);
        }
        usleep(100000);
    }

    add_output_msg(tui->output_terminal, "[RESIZE_THREAD] Shutting down...", COLOR_INFO);
    return NULL;
}

void update_debug_display(tui_t* tui) {
    if (!tui->debug_mode || !tui->debug || !tui->debug_terminal)
        return;

    werase(tui->debug_terminal);
    int line = 0;
    int debug_width = getmaxx(tui->debug_terminal);

    mvwprintw(tui->debug_terminal, line++, 1, "Server Status:");
    mvwprintw(tui->debug_terminal, line++, 1, "-- Online: %s", get_server_status() ? "YES" : "NO");
    mvwprintw(tui->debug_terminal, line++, 1, "-- Logged in: %s", client_state.loggedIn ? "YES" : "NO");
    if (client_state.loggedIn) {
        mvwprintw(tui->debug_terminal, line++, 1, "-- User: %s", client_state.client_name);
    }
    
    line++;

    mvwprintw(tui->debug_terminal, line++, 1, "Input State:");
    mvwprintw(tui->debug_terminal, line++, 1, "-- Has input: %s", input_state.buff_len ? "YES" : "NO");
    mvwprintw(tui->debug_terminal, line++, 1, "-- Cursor: (%d, %d)", input_state.buff_cursor_x, input_state.buff_cursor_y);
    mvwprintw(tui->debug_terminal, line++, 1, "-- Buff len: %d", input_state.buff_len);
    char buff_preview[21];
    strncpy(buff_preview, input_state.buff_cmd, 20);
    buff_preview[20] = '\0';
    mvwprintw(tui->debug_terminal, line++, 1, "-- Buffer: '%s'", buff_preview);
    
    line++;
    
    mvwprintw(tui->debug_terminal, line++, 1, "Command History:");
    comm_history_t* history = &input_state;
    int show_count = (history->count < 8) ? history->count : 8;
    for (int i = 0; i < show_count; i++) {
        int hist_index = history->count - 1 - i;
        const char* cmd = get_history(history, hist_index);
        if (cmd) {
            char cmd_preview[debug_width - 4];
            strncpy(cmd_preview, cmd, debug_width - 5);
            cmd_preview[debug_width - 5] = '\0';
            mvwprintw(tui->debug_terminal, line++, 2, "%d: %s", i + 1, cmd_preview);
        }
    }

    wrefresh(tui->debug_terminal);
}

void* debug_thread(void* arg) {
    tui_t* tui = (tui_t*)arg;
    
    while (is_program_running() && tui->debug_mode) {
        update_debug_display(tui);
        usleep(100000);
    }
    
    add_output_msg(tui->output_terminal, "[DEBUG_THREAD] Shutting down...", COLOR_INFO);
    return NULL;
}

/* Enable Debug-Mode */
/* @param tui Pointer to the main TUI struct */
void enable_debug(tui_t* tui) {
    add_output_msg(tui->output_terminal, "[CLIENT] Debug Mode activated.", COLOR_CLIENT);
    tui->debug_mode = true;
    tui->debug = newwin(LINES, COLS / 3, COLS * 2 / 3, 0);
    tui->debug_terminal = newwin(LINES - 2, COLS / 3 - 2, COLS * 2 / 3 + 1, 1);
    werase(stdscr);
    handle_resize(tui);

    pthread_t debug_thread_id;
    pthread_create(&debug_thread_id, NULL, debug_thread, tui);
    pthread_detach(debug_thread_id);
}

/* Disable Debug-Mode */
/* @param tui Pointer to the main TUI struct */
void disable_debug(tui_t* tui) {
    add_output_msg(tui->output_terminal, "[CLIENT] Debug Mode deactivated.", COLOR_CLIENT);
    tui->debug_mode = false;
    usleep(200000);
    wclear(tui->debug_terminal);
    wclear(tui->debug);
    delwin(tui->debug_terminal);
    delwin(tui->debug);
    handle_resize(tui);
}

/* Terminal Shutdown Animation - to be played whenever the client is shutting down */
void terminal_shutdown_animation(void) {
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
}