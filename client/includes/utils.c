#include "utils.h"
#include <signal.h>

volatile bool program_running = true;
pthread_mutex_t resize_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t input_mutex = PTHREAD_MUTEX_INITIALIZER;

void set_program_running(bool state) {
    program_running = state;
}

bool is_program_running() {
    return program_running;
}

void handle_sigint(int sig) {
    (void)sig;
    signal(SIGINT, SIG_IGN);
    set_program_running(false);
}