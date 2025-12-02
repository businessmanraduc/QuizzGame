#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <pthread.h>
#include <ncurses.h>

#define BUFF_SIZE 1024
#define MAX_NAME_LEN 256
#define HISTORY_SIZE 128
#define MAX_MESSAGES 32
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
    volatile bool program_running;
    bool loggedIn;
    bool server_online;
    volatile bool resize_req;
    char client_name[256];
} client_state_t;

extern pthread_mutex_t ncurses_mutex;
extern pthread_mutex_t server_status_mutex;
#define NCURSES_LOCK()   pthread_mutex_lock(&ncurses_mutex)
#define NCURSES_UNLOCK() pthread_mutex_unlock(&ncurses_mutex)
extern client_state_t client_state;

typedef struct {
    char messages[MAX_MESSAGES][BUFF_SIZE];
    output_color_t colors[MAX_MESSAGES];
    int count;
    int start;
} output_state_t;

typedef struct {
    WINDOW* output;
    WINDOW* output_terminal;
    WINDOW* debug;
    WINDOW* debug_terminal;
    WINDOW* input;
    WINDOW* input_terminal;
    int output_height;
    int input_height;
    int total_width;
    bool debug_mode;
} tui_t;

void set_program_running(bool);
bool is_program_running();
void handle_sigint(int);
void set_resize_req(bool);
bool is_resize_needed();
void handle_sigwinch(int);
void set_server_status(bool);
bool get_server_status();
void add_output_msg(WINDOW*, const char*, output_color_t);
#endif