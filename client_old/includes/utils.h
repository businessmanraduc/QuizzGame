#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <pthread.h>

#define MAX_NAME_LEN 256
#define OUTPUT_RATIO 0.67

extern volatile bool program_running;
extern pthread_mutex_t resize_mutex;
extern pthread_mutex_t input_state_mutex;
extern pthread_mutex_t debug_mutex;

void set_program_running(bool state);
bool is_program_running();

#endif