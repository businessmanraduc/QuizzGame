#include "includes/utils.h"
#include "includes/xml.h"

#define Clear   "\033[3;0;0m"
#define Black   "\033[0;30m"
#define Red     "\033[0;31m"
#define Green   "\033[0;32m"
#define Yellow  "\033[0;33m"
#define Blue    "\033[0;34m"
#define Purple  "\033[0;35m"
#define Cyan    "\033[0;36m"
#define White   "\033[0;37m"

user_data_t** users = NULL;
int user_count = 0;
question_t** questions = NULL;
int question_count = 0;

typedef struct {
    int socket_fd;
    int score;
    char username[MAX_NAME_LEN];
} client_t;

void* handle_client(void* arg) {
    client_t* client = (client_t*) arg;
    char buff[BUFF_SIZE];
    user_data_t* user_data = NULL;

    while (true) {
        memset(buff, 0, sizeof(buff));
        int len = recv(client->socket_fd, buff, sizeof(buff), 0);
        if (len <= 0) {
            printf(Cyan"[SERVER_CHANDLER] Client %s disconnected successfully\n"Clear, client->username);
            break;
        }

        printf(Blue"[SERVER_CHANDLER] Command received: '%s'\n"Clear, buff);
        if (strncmp(buff, "answer : ", 9) == 0) {
            send(client->socket_fd, "TEMP:ANSWER_RECEIVED", 20, 0);
        } else if (strncmp(buff, "help", 4) == 0) {
            send(client->socket_fd, "RESP:// ===== Server Specific Commands =====//\n\
answer : abcd...      => Answer to current question\n\
help                  => Display command list\n\
login : username      => Login into user with name 'username'\n\
logout                => Log out of the current user\n\
meow                  => 'meow :3' back\n\
register : username   => Register new user with name 'username'\n\
stats                 => Show stats of current user\n\
quit/exit             => Quit cleanly from the server and close client\n\
// ===== Client Specific Commands =====//\n\
clear                 => Clear terminal\n", 569, 0);
        } else if (strncmp(buff, "login : ", 8) == 0) {
            if (user_data != NULL) {
                send(client->socket_fd, "WARN:You are already logged in. Maybe you meant 'logout'?", 57, 0);
                continue;
            }
            char username[MAX_NAME_LEN];
            sscanf(buff, "login : %s", username);
            user_data = find_user(username);

            if (user_data == NULL) {
                send(client->socket_fd, "ERR_:User not found. Please register first.", 43, 0);
            } else {
                strcpy(client->username, username);
                time_t now = time(NULL);
                strftime(user_data->last_login, sizeof(user_data->last_login), "%Y-%m-%d %H:%M:%S", localtime(&now));
                save_users();
                char resp[BUFF_SIZE];
                snprintf(resp, sizeof(resp), "RESP:Welcome back %s! Points: %d, Games: %d, Wins: %d", 
                         username, user_data->total_points, user_data->games_played, user_data->games_won);
                send(client->socket_fd, resp, strlen(resp), 0);
            }
        } else if (strncmp(buff, "logout", 6) == 0) {
            if (user_data == NULL) {
                send(client->socket_fd, "WARN:You are already logged out. Maybe you meant 'quit'?", 56, 0);
                continue;
            }
            user_data = NULL;
            send(client->socket_fd, "RESP:Logged Out", 15, 0);
        } else if (strncmp(buff, "meow", 4) == 0) {
            send(client->socket_fd, "RESP:meow :3", 12, 0);
        } else if (strncmp(buff, "register : ", 11) == 0) {
            if (user_data != NULL) {
                send(client->socket_fd, "WARN:You are already logged in.", 31, 0);
                continue;
            }
            char username[MAX_NAME_LEN];
            sscanf(buff, "register : %s", username);
            user_data_t* existing_user = find_user(username);

            if (existing_user)
                send(client->socket_fd, "ERR_:Username already exists!", 29, 0);
            else {
                user_data = create_user(username);
                strcpy(client->username, username);
                char resp[BUFF_SIZE];
                snprintf(resp, sizeof(resp), "RESP:Registered new user '%s'", username);
                send(client->socket_fd, resp, strlen(resp), 0);
            }
        } else if (strncmp(buff, "stats", 5) == 0) {
            if (user_data == NULL)
                send(client->socket_fd, "ERR_:Please login first to view stats", 37, 0);
            else {
                char resp[BUFF_SIZE];
                snprintf(resp, sizeof(resp),
                         "RESP:Stats: %s | Points: %d | Games: %d | Wins: %d | Win Rate: %.1f | Max Streak: %d",
                         client->username, user_data->total_points, user_data->games_played, user_data->games_won, 
                         (user_data-> games_played > 0) ? (100.0 * user_data->games_won / user_data->games_played) : 0.0,
                         user_data->max_streak);
                send(client->socket_fd, resp, strlen(resp), 0);
            } 
        } else if (strncmp(buff, "quit", 4) == 0) {
            send(client->socket_fd, "RESP:bye-bye!", 13, 0);
        } else {
            send(client->socket_fd, "ERR_:Unrecognized Command", 25, 0);
        }
    }

    close(client->socket_fd);
    free(client);
    return NULL;
}

int main() {
    LIBXML_TEST_VERSION

    load_users();
    load_questions();

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror(Red"[SERVER] Error - SO_REUSEADDR failed\n"Clear);
    }
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);
    printf(Blue"[SERVER] Listening on port 8080...\n"Clear);

    while(1) {
        int client_socket = accept(server_fd, NULL, NULL);
        printf(Blue"[SERVER] New client connected! Socket: %d\n"Clear, client_socket);

        client_t* client = malloc(sizeof(client_t));
        client->socket_fd = client_socket;
        client->score = 0;
        strcpy(client->username, "__anon__");

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, client);
        pthread_detach(thread_id);
        printf(Blue"[SERVER] Created new thread for new client.\nWaiting new connections...\n"Clear);
    }

    xmlCleanupParser();
    return 0;
}