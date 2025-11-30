#include "tui.h"
#include "utils.h"
#include "network.h"
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

void initialize_tui(tui_t *tui) {
    initscr();
    cbreak();
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
    tui->output_height = (int)(total_height * OUTPUT_RATIO);
    tui->input_height = total_height - tui->output_height;
    
    tui->output_win = newwin(tui->output_height, tui->total_width, 0, 0);
    tui->input_win = newwin(tui->input_height, tui->total_width, tui->output_height, 0);
    tui->output_terminal = newwin(tui->output_height - 2, tui->total_width - 2, 1, 1);
    tui->output_debug = NULL;

    scrollok(tui->output_terminal, TRUE);
    keypad(tui->input_win, TRUE);
    
    box(tui->output_win, 0, 0);
    box(tui->input_win, 0, 0);
    mvwprintw(tui->output_win, 0, 2, " Output ");
    mvwprintw(tui->input_win, 0, 2, " Input ");
    
    wrefresh(tui->output_win);
    wrefresh(tui->input_win);
}

void enable_debug_mode(tui_t* tui) {
    if (tui->debug_mode)
        return;
    
    pthread_mutex_lock(&resize_mutex);
    tui->debug_mode = true;
    int content_width = (tui->total_width - 2) * 2 / 3;
    int debug_width = (tui->total_width - 2) - content_width;

    if (tui->output_terminal)
        delwin(tui->output_terminal), tui->output_terminal = NULL;
    if (tui->output_debug)
        delwin(tui->output_debug), tui->output_debug = NULL;

    tui->output_terminal = newwin(tui->output_height - 2, content_width, 1, 1);
    tui->output_debug = newwin(tui->output_height - 2, debug_width - 1, 1, content_width + 2);

    scrollok(tui->output_terminal, TRUE);
    scrollok(tui->output_debug, TRUE);
    
    werase(tui->output_win);
    box(tui->output_win, 0, 0);
    mvwprintw(tui->output_win, 0, 2, " Output ");

    int separator_pos = 1 + content_width;
    for (int i = 1; i < tui->output_height - 1; i++) {
        mvwaddch(tui->output_win, i, separator_pos, ACS_VLINE);
    }

    touchwin(tui->output_win);
    touchwin(tui->output_terminal);
    touchwin(tui->output_debug);
    
    wrefresh(tui->output_win);
    wrefresh(tui->output_terminal);
    wrefresh(tui->output_debug);

    update_debug_display(tui);
    pthread_mutex_unlock(&resize_mutex);

    handle_window_resize(tui);

    pthread_t debug_thread;
    pthread_create(&debug_thread, NULL, debug_monitor_thread, tui);
    pthread_detach(debug_thread);
}

void disable_debug_mode(tui_t* tui) {
    if (!tui->debug_mode)
        return;

    pthread_mutex_lock(&resize_mutex);
    tui->debug_mode = false;
    if (tui->output_debug)
        delwin(tui->output_debug), tui->output_debug = NULL;
    if (tui->output_terminal)
        delwin(tui->output_terminal), tui->output_terminal = NULL;
        
    tui->output_terminal = newwin(tui->output_height - 2, tui->total_width - 2, 1, 1);
    scrollok(tui->output_terminal, TRUE);

    werase(tui->output_win);
    box(tui->output_win, 0, 0);
    mvwprintw(tui->output_win, 0, 2, " Output ");

    wrefresh(tui->output_win);
    pthread_mutex_unlock(&resize_mutex);

    handle_window_resize(tui);
}

void update_debug_display(tui_t* tui) {
    if (!tui->debug_mode || !tui->output_debug)
        return;

    pthread_mutex_lock(&debug_mutex);
    werase(tui->output_debug);
    int line = 0;
    int debug_width = getmaxx(tui->output_debug);

    mvwprintw(tui->output_debug, line++, 1, "Server Status:");
    mvwprintw(tui->output_debug, line++, 1, "-- Online: %s", get_server_status() ? "YES" : "NO");
    mvwprintw(tui->output_debug, line++, 1, "-- Logged in: %s", logged_in ? "YES" : "NO");
    if (logged_in) {
        mvwprintw(tui->output_debug, line++, 1, "-- User: %s", client_name);
    }
    
    line++;

    mvwprintw(tui->output_debug, line++, 1, "Input State:");
    mvwprintw(tui->output_debug, line++, 1, "-- Has input: %s", current_input_state.has_input ? "YES" : "NO");
    mvwprintw(tui->output_debug, line++, 1, "-- Cursor: (%d, %d)", current_input_state.cursor_x, current_input_state.cursor_y);
    mvwprintw(tui->output_debug, line++, 1, "-- Buff len: %d", current_input_state.cursor_pos);
    char buff_preview[21];
    strncpy(buff_preview, current_input_state.buff, 20);
    buff_preview[20] = '\0';
    mvwprintw(tui->output_debug, line++, 1, "-- Buffer: '%s'", buff_preview);
    
    line++;
    
    mvwprintw(tui->output_debug, line++, 1, "Command History:");
    command_history_t* history = &current_input_state.history;
    int show_count = (history->count < 3) ? history->count : 3;
    for (int i = 0; i < show_count; i++) {
        int hist_index = history->count - 1 - i;
        const char* cmd = get_history_command(history, hist_index);
        if (cmd) {
            char cmd_preview[debug_width - 4];
            strncpy(cmd_preview, cmd, debug_width - 5);
            cmd_preview[debug_width - 5] = '\0';
            mvwprintw(tui->output_debug, line++, 2, "%d: %s", i + 1, cmd_preview);
        }
    }

    wrefresh(tui->output_debug);
    pthread_mutex_unlock(&debug_mutex);
}

void* debug_monitor_thread(void* arg) {
    tui_t* tui = (tui_t*)arg;
    
    while (is_program_running() && tui->debug_mode) {
        update_debug_display(tui);
        usleep(100000);
    }
    
    add_output_message(tui, "[DEBUG_THREAD] Shutting down...", COLOR_INFO);
    return NULL;
}

void cleanup_tui(tui_t *tui) {
    if (tui->output_win) delwin(tui->output_win);
    if (tui->output_terminal) delwin(tui->output_terminal);
    if (tui->output_debug) delwin(tui->output_debug);
    if (tui->input_win) delwin(tui->input_win);
    endwin();
}

void clear_output_screen(tui_t* tui) {
    pthread_mutex_lock(&resize_mutex);
    werase(tui->output_terminal);
    wrefresh(tui->output_terminal);
    pthread_mutex_unlock(&resize_mutex);
}

void add_output_message(tui_t *tui, const char *text, output_color_t color) {
    pthread_mutex_lock(&resize_mutex);
    if (has_colors()) {
        wattron(tui->output_terminal, COLOR_PAIR(color));
    }
    wprintw(tui->output_terminal, "%s\n", text);
    if (has_colors()) {
        wattroff(tui->output_terminal, COLOR_PAIR(color));
    }
    wrefresh(tui->output_terminal);
    pthread_mutex_unlock(&resize_mutex);
}

void handle_window_resize(tui_t *tui) {
    pthread_mutex_lock(&resize_mutex);
    endwin();
    refresh();
    clear();
    
    int total_height = LINES;
    tui->total_width = COLS;
    tui->output_height = (int)(total_height * OUTPUT_RATIO);
    tui->input_height = total_height - tui->output_height;
    
    if (tui->debug_mode) {
        int content_width = (tui->total_width - 2) * 2 / 3;
        int debug_width = (tui->total_width - 2) - content_width;
        
        wresize(tui->output_win, tui->output_height, tui->total_width);
        wresize(tui->input_win, tui->input_height, tui->total_width);
        mvwin(tui->input_win, tui->output_height, 0);
        
        if (tui->output_terminal) {
            wresize(tui->output_terminal, tui->output_height - 2, content_width);
            mvwin(tui->output_terminal, 1, 1);
        }
        if (tui->output_debug) {
            wresize(tui->output_debug, tui->output_height - 2, debug_width - 1);
            mvwin(tui->output_debug, 1, 1 + content_width + 1);
        }
        
        werase(tui->output_win);
        box(tui->output_win, 0, 0);
        mvwprintw(tui->output_win, 0, 2, " Output ");
        int separator_pos = 1 + content_width;
        for (int i = 1; i < tui->output_height - 1; i++) {
            mvwaddch(tui->output_win, i, separator_pos, ACS_VLINE);
        }
    } else {
        wresize(tui->output_win, tui->output_height, tui->total_width);
        wresize(tui->input_win, tui->input_height, tui->total_width);
        mvwin(tui->input_win, tui->output_height, 0);
        
        wresize(tui->output_terminal, tui->output_height - 2, tui->total_width - 2);
        mvwin(tui->output_terminal, 1, 1);
        
        werase(tui->output_win);
        box(tui->output_win, 0, 0);
        mvwprintw(tui->output_win, 0, 2, " Output ");
    }
    
    restore_input_display(tui->input_win, &current_input_state, tui->total_width, tui->input_height);
    
    touchwin(tui->output_win);
    touchwin(tui->input_win);
    if (tui->output_terminal) touchwin(tui->output_terminal);
    if (tui->output_debug) touchwin(tui->output_debug);
    
    wrefresh(tui->output_win);
    wrefresh(tui->input_win);
    if (tui->output_terminal) wrefresh(tui->output_terminal);
    if (tui->output_debug) wrefresh(tui->output_debug);

    pthread_mutex_unlock(&resize_mutex);
}

void* monitor_resize_thread(void* arg) {
    tui_t* tui = (tui_t*)arg;
    int last_lines = LINES;
    int last_cols = COLS;
    
    while (is_program_running()) {
        if (LINES != last_lines || COLS != last_cols) {
            last_lines = LINES;
            last_cols = COLS;
            handle_window_resize(tui);
        }

        usleep(100000);
    }

    add_output_message(tui, "[RESIZE_THREAD] Shutting down...", COLOR_INFO);
    return NULL;
}

void terminal_shutdown_animation(void) {
    def_prog_mode();
    endwin();
    
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    reset_prog_mode();
    clear();
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