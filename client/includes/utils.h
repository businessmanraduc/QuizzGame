#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <pthread.h>

#define BUFF_SIZE 1024
#define MAX_NAME_LEN 256
#define HISTORY_SIZE 128
#define OUTPUT_RATIO 0.67

typedef enum {
    COLOR_DEFAULT = 1,
    COLOR_ERROR = 2,
    COLOR_WARNING = 3,
    COLOR_SUCCESS = 4,
    COLOR_INFO = 5,
    COLOR_SERVER = 6,
    COLOR_CLIENT = 7
} output_color_t;

typedef struct {
    int socket_fd;
    bool loggedIn;
    char client_name[256];
} client_state_t;

extern volatile bool program_running;
extern pthread_mutex_t resize_mutex;
extern pthread_mutex_t input_mutex;

extern client_state_t client_state;

void set_program_running(bool);
bool is_program_running();
void handle_sigint(int);
#endif