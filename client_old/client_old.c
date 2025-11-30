#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ncurses.h>

#define BUFF_SIZE 1024
#define MAX_NAME_LEN 256
#define MAX_HISTORY 32
#define OUTPUT_RATIO 0.67

typedef struct {
    WINDOW *output_win;
    WINDOW *output_terminal;
    WINDOW *input_win;
    int output_height;
    int input_height;
    int width;
} tui_t;

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
    char buff[BUFF_SIZE];
    int cursor_pos;
    bool has_input;
    int cursor_y, cursor_x;
    int prev_cursor_x;
} input_state_t;

input_state_t current_input_state = {{0}, 0, false, 1, 3};

int socket_fd;
bool loggedIn = false;
char clientName[MAX_NAME_LEN];
bool autoClear = false;
bool server_online = true;
pthread_mutex_t server_status_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t resize_mutex = PTHREAD_MUTEX_INITIALIZER;
tui_t tui;

volatile bool program_running = true;

// ====================== Terminal User Interface ====================== //
void init_tui(tui_t *tui) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    
    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_WHITE, -1);   // DEFAULT
        init_pair(2, COLOR_RED, -1);     // ERROR
        init_pair(3, COLOR_YELLOW, -1);  // WARNING
        init_pair(4, COLOR_GREEN, -1);   // SUCCESS
        init_pair(5, COLOR_CYAN, -1);    // INFO
        init_pair(6, COLOR_MAGENTA, -1); // SERVER
        init_pair(7, COLOR_BLUE, -1);    // CLIENT
    }
    
    refresh();
    
    int total_height = LINES;
    tui->width = COLS;
    tui->output_height = (int)(total_height * OUTPUT_RATIO);
    tui->input_height = total_height - tui->output_height;
    
    tui->output_win = newwin(tui->output_height, tui->width, 0, 0);
    tui->input_win = newwin(tui->input_height, tui->width, tui->output_height, 0);
    tui->output_terminal = newwin(tui->output_height - 2, tui->width - 2, 1, 1);
    
    scrollok(tui->output_terminal, TRUE);
    keypad(tui->input_win, TRUE);
    
    box(tui->output_win, 0, 0);
    box(tui->input_win, 0, 0);
    mvwprintw(tui->output_win, 0, 2, " Output ");
    mvwprintw(tui->input_win, 0, 2, " Input ");
    
    wrefresh(tui->output_win);
    wrefresh(tui->input_win);
}

void cleanup_tui(tui_t *tui) {
    delwin(tui->output_win);
    delwin(tui->output_terminal);
    delwin(tui->input_win);
    endwin();
}

void clear_scr() {
    pthread_mutex_lock(&resize_mutex);
    werase(tui.output_terminal);
    wrefresh(tui.output_terminal);
    pthread_mutex_unlock(&resize_mutex);
}

void add_output(tui_t *tui, const char *text, output_color_t color) {
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

void restore_input_state(tui_t* tui) {
    werase(tui->input_win);
    box(tui->input_win, 0, 0);
    mvwprintw(tui->input_win, 0, 2, " Input ");
    mvwprintw(tui->input_win, 1, 1, "> ");
    int y = 1, x = 3;
    if (current_input_state.has_input && current_input_state.cursor_pos > 0) {
        for (int i = 0; i < current_input_state.cursor_pos; i++) {
            wmove(tui->input_win, y, x);
            if (current_input_state.buff[i] == '\\') {
                y++, x = 2;
            }
            waddch(tui->input_win, current_input_state.buff[i]);
            x++;
            if (x >= tui->width - 2) {
                x = 3, y++;
                if (y >= tui->input_height - 1) break;
            }
        }
    }
    current_input_state.cursor_y = y;
    current_input_state.cursor_x = x;
    wrefresh(tui->input_win);
}

char* get_input(tui_t *tui) {
    static char input_buffer[1024];
    int ch;
    int pos = 0;
    
    current_input_state.has_input = false;
    current_input_state.cursor_pos = 0;
    current_input_state.buff[0] = '\0';
    current_input_state.cursor_y = 1;
    current_input_state.cursor_x = 3;

    pthread_mutex_lock(&resize_mutex);
    restore_input_state(tui);
    pthread_mutex_unlock(&resize_mutex);

    while ((ch = wgetch(tui->input_win)) != '\n' && ch != KEY_ENTER) {
        if (ch == KEY_BACKSPACE || ch == 127) {
            if (pos > 0) {
                pos--;
                current_input_state.cursor_pos--;
                pthread_mutex_lock(&resize_mutex);
                
                if (current_input_state.cursor_x > 3) {
                    current_input_state.cursor_x--;
                } else if (current_input_state.cursor_y > 1) {
                    current_input_state.cursor_y--;
                    if (current_input_state.buff[current_input_state.cursor_pos] == '\\')
                        current_input_state.cursor_x = current_input_state.prev_cursor_x, pos++;
                    else
                        current_input_state.cursor_x = tui->width - 2;
                }
                mvwaddch(tui->input_win, current_input_state.cursor_y, current_input_state.cursor_x, ' ');
                wmove(tui->input_win, current_input_state.cursor_y, current_input_state.cursor_x);
                current_input_state.buff[current_input_state.cursor_pos] = '\0';
                current_input_state.has_input = (current_input_state.cursor_pos > 0);
                pthread_mutex_unlock(&resize_mutex);
            }
        } else if (ch == '\\') {
            current_input_state.buff[current_input_state.cursor_pos++] = ch;
            current_input_state.prev_cursor_x = current_input_state.cursor_x;
            current_input_state.buff[current_input_state.cursor_pos] = '\0';
            current_input_state.has_input = true;
            pthread_mutex_lock(&resize_mutex);

            waddch(tui->input_win, ch);
            if (current_input_state.cursor_y < tui->input_height - 2) {
                current_input_state.cursor_x = 3;
                current_input_state.cursor_y++;
                wmove(tui->input_win, current_input_state.cursor_y, current_input_state.cursor_x);
            }

            input_buffer[pos] = '\0';
            pthread_mutex_unlock(&resize_mutex);
        } else if (ch >= 32 && ch <= 126 && pos < (int)sizeof(input_buffer) - 1) {
            input_buffer[pos++] = ch;
            current_input_state.buff[current_input_state.cursor_pos++] = ch;
            current_input_state.buff[current_input_state.cursor_pos] = '\0';
            current_input_state.has_input = true;
            pthread_mutex_lock(&resize_mutex);
            
            waddch(tui->input_win, ch);
            current_input_state.cursor_x++;
            if (current_input_state.cursor_x > tui->width - 3 && 
                current_input_state.cursor_y < tui->input_height - 2) {
                    current_input_state.cursor_x = 3;
                    current_input_state.cursor_y++;
                    wmove(tui->input_win, current_input_state.cursor_y, current_input_state.cursor_x);
            }
            
            input_buffer[pos] = '\0';
            pthread_mutex_unlock(&resize_mutex);
        }

        pthread_mutex_lock(&resize_mutex);
        wrefresh(tui->input_win);
        pthread_mutex_unlock(&resize_mutex);
        usleep(1000);
    }
    
    input_buffer[pos] = '\0';
    return input_buffer;
}

// ======================== Resizing Abilities ========================= //

void handle_resize(tui_t *tui) {
    pthread_mutex_lock(&resize_mutex);
    endwin();
    refresh();
    clear();
    
    int total_height = LINES;
    tui->width = COLS;
    tui->output_height = (int)(total_height * OUTPUT_RATIO);
    tui->input_height = total_height - tui->output_height;
    
    wresize(tui->output_win, tui->output_height, tui->width);
    wresize(tui->input_win, tui->input_height, tui->width);
    mvwin(tui->input_win, tui->output_height, 0);
    
    wresize(tui->output_terminal, tui->output_height - 2, tui->width - 2);
    mvwin(tui->output_terminal, 1, 1);
    
    werase(tui->output_win);
    box(tui->output_win, 0, 0);
    mvwprintw(tui->output_win, 0, 2, " Output ");
    
    touchwin(tui->output_win);
    touchwin(tui->input_win);
    touchwin(tui->output_terminal);
    
    wrefresh(tui->output_win);
    wrefresh(tui->input_win);
    wrefresh(tui->output_terminal);

    restore_input_state(tui);

    pthread_mutex_unlock(&resize_mutex);
}

void* resize_monitor_thread(void* arg) {
    int last_lines = LINES;
    int last_cols = COLS;
    bool resize_pending = false;
    while (program_running) {
        if (LINES != last_lines || COLS != last_cols) {
            resize_pending = true;
            last_lines = LINES;
            last_cols = COLS;
        }

        if (resize_pending) {
            handle_resize(&tui);
            resize_pending = false;
        }

        usleep(50000);
    }

    return NULL;
}

// ===================== Signal Handlers (SIGINT) ====================== //

void handle_sigint(int sig) {
    program_running = false;
    cleanup_tui(&tui);
    close(socket_fd);
    exit(0);
}

// ======================== Server Interfacing ========================= //

void set_server_status(bool status) {
    pthread_mutex_lock(&server_status_mutex);
    server_online = status;
    pthread_mutex_unlock(&server_status_mutex);
}

bool get_server_status() {
    pthread_mutex_lock(&server_status_mutex);
    bool status = server_online;
    pthread_mutex_unlock(&server_status_mutex);
    return status;
}

void format_server_resp(const char* resp) {
    if (strstr(resp, "ERR"))
        add_output(&tui, resp + 4, COLOR_ERROR);
    else if (strstr(resp, "WARN"))
        add_output(&tui, resp + 5, COLOR_WARNING);
    else if (strstr(resp, "SCORE") || strstr(resp, "WIN!!"))
        add_output(&tui, resp + 6, COLOR_SUCCESS);
    else if (strstr(resp, "OK:LOGIN"))
        add_output(&tui, resp + 11, COLOR_SUCCESS);
    else if (strstr(resp, "OK:REGISTERED"))
        add_output(&tui, resp + 16, COLOR_SUCCESS);
    else
        add_output(&tui, resp, COLOR_SERVER);
}

void send_command(const char* cmd) {
    if (!get_server_status()) {
        add_output(&tui, "[CLIENT] Error - Server is offline. Type 'reconnect' to attempt reconnection.\n", COLOR_ERROR);
        return;
    }
    if (send(socket_fd, cmd, strlen(cmd), 0) < 0) {
        add_output(&tui, "[CLIENT] Error - Command sending failed!\n", COLOR_ERROR);
        set_server_status(false);
        add_output(&tui, "[CLIENT] Server connection lost. Type 'reconnect' to attempt reconnection.\n", COLOR_WARNING);
        return;
    }
    usleep(50000);
}

void* receive_msg_thread(void* socket_ptr) {
    int socket = *(int*)socket_ptr;
    char buff[BUFF_SIZE];

    while (program_running) {
        memset(buff, 0, BUFF_SIZE);
        int len = recv(socket, buff, BUFF_SIZE - 1, 0);
        if (len <= 0) {
            add_output(&tui, "[CLIENT] Warning - Disconnected from server/Server offline.\n", COLOR_WARNING);
            add_output(&tui, "[CLIENT] Type 'reconnect' tp attempt reconnection.\n", COLOR_WARNING);
            set_server_status(false);
            break;
        }

        if (strstr(buff, "OK:REGISTERED")) {
            loggedIn = true;
            strcpy(clientName, buff + 16);
        } else if (strstr(buff, "OK:LOGGED")) {
            loggedIn = true;
            strcpy(clientName, buff + 12);
        } else if (strstr(buff, "OK:LOGOUT")) {
            loggedIn = false;
        }

        format_server_resp(buff);
    }

    return NULL;
}

bool connect_to_server() {
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        add_output(&tui, "[CLIENT] Error - Socket creation failed!\n", COLOR_ERROR);
        return false;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    
    if (connect(socket_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        add_output(&tui, "[CLIENT] Error - Connection to server failed.\n", COLOR_ERROR);
        close(socket_fd);
        return false;
    }

    set_server_status(true);
    loggedIn = false;
    memset(clientName, 0, MAX_NAME_LEN);
    add_output(&tui, "[CLIENT] Successfully connected to server.\n", COLOR_SUCCESS);
    return true;
}

bool attempt_reconnect() {
    add_output(&tui, "[CLIENT] Attempting to reconnect to server...\n", COLOR_CLIENT);
    if (socket_fd > 0) close(socket_fd);

    if (connect_to_server()) {
        pthread_t recv_thread;
        pthread_create(&recv_thread, NULL, receive_msg_thread, &socket_fd);
        pthread_detach(recv_thread);
        return true;
    }
    
    return false;
}

void send_msg_thread() {
    char* input;
    while (program_running) {
        input = get_input(&tui);
        if (strlen(input) == 0) {
            continue;
        }
        char output_line[1024];
        snprintf(output_line, sizeof(output_line), "> %s", input);
        add_output(&tui, output_line, COLOR_CLIENT);
        
        char* cmd = input;
        while (*cmd == ' ' || *cmd == '\t') cmd++;
        char* end = cmd + strlen(cmd) - 1;
        while (end > cmd && (*end == ' ' || *end == '\t' || *end == '\n')) end--;
        *(end + 1) = '\0';

        if (strlen(cmd) > 0) {
            if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
                send_command("quit");
                add_output(&tui, "[CLIENT] Shutting down...\n", COLOR_CLIENT);
                break;
            } else if (strcmp(cmd, "reconnect") == 0) {
                if (get_server_status())
                    add_output(&tui, "[CLIENT] Warning - Already connected!\n", COLOR_WARNING);
                else 
                    attempt_reconnect();
            } else if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "clr") == 0) {
                clear_scr();
            } else {
                send_command(cmd);
            }
        }
        
        if (is_term_resized(LINES, COLS)) {
            handle_resize(&tui);
        }

        usleep(10000);
    }

    sleep(1);
}

int main() {
    init_tui(&tui);
    signal(SIGINT, handle_sigint);

    pthread_t resize_thread;
    pthread_create(&resize_thread, NULL, resize_monitor_thread, NULL);
    pthread_detach(resize_thread);

    if (!connect_to_server()) {
        add_output(&tui, "[CLIENT] Error - Failed to connect to server automatically.\n", COLOR_ERROR);
        add_output(&tui, "[CLIENT] Type 'reconnect' to attempt manual reconnection to server.\n", COLOR_WARNING);
    } else {
        pthread_t recv_thread;
        pthread_create(&recv_thread, NULL, receive_msg_thread, &socket_fd);
        pthread_detach(recv_thread);
    }
    
    clear_scr();
    add_output(&tui, "Client Version 0.1", COLOR_CLIENT);
    add_output(&tui, "Note: localhost-mode only", COLOR_CLIENT);
    add_output(&tui, "Note: quiz game unavailable, WIP.", COLOR_CLIENT);
    add_output(&tui, "\nCLIENT: Entering anon-log mode. Type messages to send to server.", COLOR_CLIENT);
    add_output(&tui, "For help, type 'help'\n", COLOR_CLIENT);
    
    send_msg_thread();
 
    add_output(&tui, "[CLIENT] Thank you for playing!\n", COLOR_GREEN);
    handle_sigint(SIGINT);
    return 0;
}