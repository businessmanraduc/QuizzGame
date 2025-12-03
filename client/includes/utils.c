#include "utils.h"
#include <signal.h>

volatile bool program_running = true;
pthread_mutex_t resize_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ncurses_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Set the running state of the Client */
void set_program_running(bool state) {
    program_running = state;
}

/* Fetch the running state of the Client */
bool is_program_running() {
    return program_running;
}

/* SIGINT Signal Handler */
void handle_sigint(int sig) {
    (void)sig;
    signal(SIGINT, SIG_IGN);
    set_program_running(false);
}