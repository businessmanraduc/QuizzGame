#include "utils.h"
#include <signal.h>
#include <string.h>

output_state_t output_state = {0};
client_state_t client_state = {0, true, false, false, false, {0}};
pthread_mutex_t ncurses_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t server_status_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Set the running state of the Client */
void set_program_running(bool state) {
    client_state.program_running = state;
}

/* Fetch the running state of the Client */
bool is_program_running() {
    return client_state.program_running;
}

/* SIGINT Signal Handler */
void handle_sigint(int sig) {
    (void)sig;
    signal(SIGINT, SIG_IGN);
    set_program_running(false);
}

/* Set the resize requirement */
void set_resize_req(bool req) {
    client_state.resize_req = req;
}

/* Read the resize requirement */
bool is_resize_needed() {
    return client_state.resize_req;
}

/* SIGWINCH Signal Handler */
void handle_sigwinch(int sig) {
    (void)sig;
    set_resize_req(true);
}

/* Set the server status */
/* @param status The new status value*/
void set_server_status(bool status) {
    pthread_mutex_lock(&server_status_mutex);
    client_state.server_online = status;
    pthread_mutex_unlock(&server_status_mutex);
}

/* Get the server status */
bool get_server_status() {
    pthread_mutex_lock(&server_status_mutex);
    bool status = client_state.server_online;
    pthread_mutex_unlock(&server_status_mutex);
    return status;
}

/* Add output message to the screen (safe, using NCURSES_LOCK())*/
/* @param output_window Pointer to the output window */
/* @param msg Message to be printed */
/* @param color Color to be used for the message */
void add_output_msg(WINDOW* output_window, const char* msg, output_color_t color) {
    if (output_state.count < MAX_MESSAGES) {
        strncpy(output_state.messages[output_state.count], msg, BUFF_SIZE - 1);
        output_state.colors[output_state.count] = color;
        output_state.count++;
    } else {
        strncpy(output_state.messages[output_state.start], msg, BUFF_SIZE - 1);
        output_state.colors[output_state.start] = color;
        output_state.start = (output_state.start + 1) & 0x1F;
    }

    NCURSES_LOCK();
    if (has_colors())
        wattron(output_window, COLOR_PAIR(color));
    wprintw(output_window, "%s\n", msg);
    if (has_colors())
        wattroff(output_window, COLOR_PAIR(color));
    wrefresh(output_window);
    NCURSES_UNLOCK();
}