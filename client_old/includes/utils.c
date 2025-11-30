#include "utils.h"

volatile bool program_running = true;
pthread_mutex_t resize_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t input_state_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t debug_mutex = PTHREAD_MUTEX_INITIALIZER;

void set_program_running(bool state) {
    program_running = state;
}

bool is_program_running() {
    return program_running;
}