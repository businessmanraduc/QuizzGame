#include "includes/utils.h"
#include "includes/data_loader.h"
#include <time.h>

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
game_session_t game_session;

void broadcast_all(const char* message, client_t* exclude) {
    pthread_mutex_lock(&game_session.lock);
    for (int i = 0; i < game_session.player_count; i++) {
        if (game_session.players[i] && 
            game_session.players[i]->state != CLIENT_DISCONNECTED &&
            game_session.players[i] != exclude) {
                send(game_session.players[i]->socket_fd, message, strlen(message), 0);
            }
    }
    pthread_mutex_unlock(&game_session.lock);
}

void send_to_client(client_t* client, const char* message) {
    if (client && client->state != CLIENT_DISCONNECTED)
        send(client->socket_fd, message, strlen(message), 0);
}

void remove_player(client_t* client) {
    pthread_mutex_lock(&game_session.lock);

    for (int i = 0; i < game_session.player_count; i++) {
        if (game_session.players[i] == client) {
            for (int j = i; j < game_session.player_count - 1; j++) 
                game_session.players[j] = game_session.players[j+1];
            game_session.player_count--;

            if (game_session.curr_player_turn >= game_session.player_count && game_session.player_count > 0)
                game_session.curr_player_turn = 0;
            break;
        }
    }

    pthread_mutex_unlock(&game_session.lock);
}

void send_question(client_t* player, question_t* question) {
    char buff[BUFF_SIZE];
    snprintf(buff, sizeof(buff),
             "QUES:It's your turn! Read the question and choose wisely:\n%s\nA:%s\nB:%s\nC:%s\nD:%s\n",
             question->text, question->option_a, question->option_b, question->option_c, question->option_d);
    send_to_client(player, buff);
}

void broadcast_question(question_t* question, const char* current_player_name) {
    char buff[BUFF_SIZE];
    snprintf(buff, sizeof(buff),
             "QUES:Spectating player %s:\n%s\nA:%s\nB:%s\nC:%s\nD:%s\n",
             current_player_name, question->text,
             question->option_a, question->option_b, question->option_c, question->option_d);

    broadcast_all(buff, game_session.players[game_session.curr_player_turn]);
}

void announce_result(client_t* player, bool correct, int points_earned, char correct_ans) {
    char buff[BUFF_SIZE];

    if (correct) {
        snprintf(buff, sizeof(buff),
                 "WRES:You answered correctly! +%d points (Total: %d)",
                 points_earned, player->score);
    } else {
        snprintf(buff, sizeof(buff),
                 "LRES:Oops! You answered incorrectly. Correct answer was: %c. (Total: %d points)",
                 correct_ans, player->score);
    }
    send_to_client(player, buff);
    broadcast_all("INFO:Player responded, moving on to the next contestant!\n", player);
    sleep(2);
}

void announce_timeout(client_t* player) {
    char buff[BUFF_SIZE];
    snprintf(buff, sizeof(buff), 
             "LRES:Oops, you ran out of time! No points awarded. (Total: %d points)",
             player->score);
    send_to_client(player, buff);
    broadcast_all("INFO:Player ran out of time...I wonder what happened. Anyways, moving on to the next contestant!\n", player);
    sleep(2);
}

void announce_winner() {
    pthread_mutex_lock(&game_session.lock);

    if (game_session.player_count == 0) {
        pthread_mutex_unlock(&game_session.lock);
        return;
    }

    int max_score = -1;
    for (int i = 0; i < game_session.player_count; i++) {
        if (game_session.players[i]->score > max_score)
            max_score = game_session.players[i]->score;
    }

    char winners[BUFF_SIZE / 4] = "";
    int winner_count = 0;

    for (int i = 0; i < game_session.player_count; i++) {
        if (game_session.players[i]->score == max_score) {
            if (winner_count > 0) strcat(winners, ", ");
            strcat(winners, game_session.players[i]->username);
            winner_count++;
            if (game_session.players[i]->user_data) {
                game_session.players[i]->user_data->games_won++;
                game_session.players[i]->user_data->curr_streak++;
                if (game_session.players[i]->user_data->max_streak < game_session.players[i]->user_data->curr_streak)
                    game_session.players[i]->user_data->max_streak = game_session.players[i]->user_data->curr_streak;
            }
        } else {
            if (game_session.players[i]->user_data) {
                game_session.players[i]->user_data->curr_streak = 0;
            }
        }

        if (game_session.players[i]->user_data) {
            game_session.players[i]->user_data->total_points += game_session.players[i]->score;
            game_session.players[i]->user_data->games_played++;
        }
    }

    pthread_mutex_unlock(&game_session.lock);
    
    save_users();

    char buff[BUFF_SIZE];
    if (winner_count == 1)
        snprintf(buff, sizeof(buff), "END_:Winner: %s with %d points!\n", winners, max_score);
    else
        snprintf(buff, sizeof(buff), "END_:Tie! Winners: %s with %d points each!\n", winners, max_score);

    broadcast_all(buff, NULL);
}

void* game_loop(void* arg) {
    printf(Green"[GAME] Game loop started, waiting for start signal...\n"Clear);

    pthread_mutex_lock(&game_session.lock);
    while (game_session.state == GAME_WAITING)
        pthread_cond_wait(&game_session.game_start, &game_session.lock);
    pthread_mutex_unlock(&game_session.lock);
    
    broadcast_all("GAME:Ladies and gentlemen, the game is starting! Get ready!", NULL);
    sleep(2);

    for (int q_idx = 0; q_idx < question_count; q_idx++) {
        pthread_mutex_lock(&game_session.lock);

        if (game_session.player_count == 0) {
            printf(Yellow"[GAME] No players left! Ending game.\n"Clear);
            pthread_mutex_unlock(&game_session.lock);
            break;
        }

        game_session.curr_question_idx = q_idx;
        question_t* curr_question = questions[q_idx];
        int players_in_round = game_session.player_count;

        pthread_mutex_unlock(&game_session.lock);
        printf(Cyan"[GAME] Question %d/%d: %s\n"Clear, q_idx + 1, question_count, curr_question->text);

        for (int player_idx = 0; player_idx < players_in_round; player_idx++) {
            pthread_mutex_lock(&game_session.lock);
            if (game_session.player_count == 0) {
                pthread_mutex_unlock(&game_session.lock);
                break;
            }
            if (player_idx >= game_session.player_count) {
                pthread_mutex_unlock(&game_session.lock);
                break;
            }

            game_session.curr_player_turn = player_idx;
            client_t* curr_player = game_session.players[player_idx];
            char curr_player_name[MAX_NAME_LEN];
            strcpy(curr_player_name, curr_player->username);
            if (curr_player->state == CLIENT_DISCONNECTED) {
                pthread_mutex_unlock(&game_session.lock);
                continue;
            }
            printf(Blue"[GAME] Player %d/%d: %s's turn\n"Clear, player_idx + 1, players_in_round, curr_player->username);
        
            pthread_mutex_unlock(&game_session.lock);

            send_question(curr_player, curr_question);
            broadcast_question(curr_question, curr_player->username);
            time_t start_time = time(NULL);
            game_session.question_start_time = start_time;

            pthread_mutex_lock(&curr_player->lock);
            curr_player->has_answered = false;
            pthread_mutex_unlock(&curr_player->lock);

            bool answer_time = false;
            while (difftime(time(NULL), start_time) < curr_question->time_limit) {
                pthread_mutex_lock(&curr_player->lock);
                bool answered = curr_player->has_answered;
                bool disconnected = (curr_player->state == CLIENT_DISCONNECTED);
                pthread_mutex_unlock(&curr_player->lock);

                if (answered || disconnected) {
                    answer_time = answered;
                    break;
                }

                usleep(100000);
            }

            if (curr_player->state == CLIENT_DISCONNECTED) {
                printf(Yellow"[GAME] Player %s disconnected during their turn\n"Clear, curr_player_name);
                char buff[BUFF_SIZE];
                snprintf(buff, sizeof(buff), "INFO:%s disconnected\n", curr_player_name);
                broadcast_all(buff, curr_player);
                continue;
            }

            if (answer_time) {
                pthread_mutex_lock(&curr_player->lock);
                char answer = curr_player->answer;
                pthread_mutex_unlock(&curr_player->lock);

                bool correct = (answer == curr_question->correct_answer);
                if (correct) {
                    curr_player->score += curr_question->points;
                    printf(Green"[GAME] %s answered correctly! +%d points\n"Clear,
                           curr_player->username, curr_question->points);
                } else {
                    printf(Blue"[GAME] %s answered incorrectly (answered %c, correct was %c)\n"Clear,
                           curr_player->username, answer, curr_question->correct_answer);
                }

                announce_result(curr_player, correct, curr_question->points, curr_question->correct_answer);
            } else {
                printf(Blue"[GAME] %s timed out\n"Clear, curr_player->username);
                announce_timeout(curr_player);
            }
        }

        if (q_idx < question_count - 1) {
            char buff[BUFF_SIZE];
            snprintf(buff, sizeof(buff), "INFO:Next question in 3 seconds... (%d/%d)", q_idx + 2, question_count);
            broadcast_all(buff, NULL);
            sleep(3);
        }
    }

    printf(Green"[GAME] All questions completed!\n"Clear);

    pthread_mutex_lock(&game_session.lock);
    game_session.state = GAME_FINISHED;
    pthread_mutex_unlock(&game_session.lock);

    announce_winner();
    return NULL;
}

void* handle_client(void* arg) {
    client_t* client = (client_t*) arg;
    char buff[BUFF_SIZE];
    while (true) {
        memset(buff, 0, sizeof(buff));
        int len = recv(client->socket_fd, buff, sizeof(buff), 0);
        if (len <= 0) {
            printf(Cyan"[SERVER_CHANDLER] Client %s disconnected successfully\n"Clear, client->username);

            if (client->state == CLIENT_IN_GAME) {
                char buff[BUFF_SIZE];
                snprintf(buff, sizeof(buff), "INFO:%s left the game", client->username);
                broadcast_all(buff, client);
                remove_player(client);
            }
            client->state = CLIENT_DISCONNECTED;

            break;
        }

        printf(Blue"[SERVER_CHANDLER] Command received: '%s'\n"Clear, buff);
        if (strncmp(buff, "answer : ", 9) == 0) {
            if (client->state != CLIENT_IN_GAME) {
                send_to_client(client, "ERR_:Not in game\n");
                continue;
            }

            pthread_mutex_lock(&game_session.lock);
            bool is_curr_turn = (game_session.curr_player_turn < game_session.player_count &&
                                 game_session.players[game_session.curr_player_turn] == client);
            pthread_mutex_unlock(&game_session.lock);

            if (!is_curr_turn) {
                send_to_client(client, "WARN:Not your turn!\n");
                continue;
            }

            char answer = buff[9] & 0x5F;
            if (answer < 'A' || answer > 'D') {
                send_to_client(client, "ERR_:Invalid answer. Use A, B, C or D.\n");
                continue;
            }

            pthread_mutex_lock(&client->lock);
            if (!client->has_answered) {
                client->answer = answer;
                client->has_answered = true;
                client->answer_time = time(NULL);
                send_to_client(client, "RESP:Answer received!\n");
            }
            pthread_mutex_unlock(&client->lock);
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
        } else if (strncmp(buff, "join", 4) == 0) {
            if (client->user_data == NULL) {
                send_to_client(client, "ERR_:Please login first to join the game!\n");
                continue;
            }

            pthread_mutex_lock(&game_session.lock);

            if (game_session.state != GAME_WAITING) {
                send_to_client(client, "ERR_:Game already in progress.\n");
                pthread_mutex_unlock(&game_session.lock);
                continue;
            }

            if (client->state == CLIENT_IN_GAME || client->state == CLIENT_LOBBY) {
                send_to_client(client, "WARN:Already in game lobby\n");
                pthread_mutex_unlock(&game_session.lock);
                continue;
            }

            client->state = CLIENT_LOBBY;
            client->score = 0;
            client->join_order = game_session.player_count;

            game_session.players = realloc(game_session.players,
                                          (game_session.player_count + 1) * sizeof(client_t*));
            game_session.players[game_session.player_count] = client;
            game_session.player_count++;

            char buff[BUFF_SIZE];
            snprintf(buff, sizeof(buff), "RESP:Joined game! Players: %d\n", game_session.player_count);
            send_to_client(client, buff);

            snprintf(buff, sizeof(buff), "INFO:%s joined the game (Total: %d players)\n", client->username, game_session.player_count);
            pthread_mutex_unlock(&game_session.lock);
            broadcast_all(buff, client);

            printf(Green"[GAME] %s joined the party! (Total players: %d)\n"Clear, client->username, game_session.player_count);
        } else if (strncmp(buff, "login : ", 8) == 0) {
            if (client->user_data != NULL) {
                send(client->socket_fd, "WARN:You are already logged in. Maybe you meant 'logout'?", 57, 0);
                continue;
            }
            char username[MAX_NAME_LEN];
            sscanf(buff, "login : %s", username);
            user_data_t* user = find_user(username);

            if (user == NULL) {
                send(client->socket_fd, "ERR_:User not found. Please register first.", 43, 0);
            } else {
                strcpy(client->username, username);
                client->user_data = user;
                time_t now = time(NULL);
                strftime(user->last_login, sizeof(user->last_login), "%Y-%m-%d %H:%M:%S", localtime(&now));
                save_users();
                char resp[BUFF_SIZE];
                snprintf(resp, sizeof(resp), "RESP:Welcome back %s! Points: %d, Games: %d, Wins: %d", 
                         username, user->total_points, user->games_played, user->games_won);
                send(client->socket_fd, resp, strlen(resp), 0);
            }
        } else if (strncmp(buff, "logout", 6) == 0) {
            if (client->user_data == NULL) {
                send(client->socket_fd, "WARN:You are already logged out. Maybe you meant 'quit'?", 56, 0);
                continue;
            }
            client->user_data = NULL;
            send(client->socket_fd, "RESP:Logged Out", 15, 0);
        } else if (strncmp(buff, "meow", 4) == 0) {
            send(client->socket_fd, "RESP:meow :3", 12, 0);
        } else if (strncmp(buff, "register : ", 11) == 0) {
            if (client->user_data != NULL) {
                send(client->socket_fd, "WARN:You are already logged in.", 31, 0);
                continue;
            }
            char username[MAX_NAME_LEN];
            sscanf(buff, "register : %255s", username);
            user_data_t* existing_user = find_user(username);

            if (existing_user)
                send(client->socket_fd, "ERR_:Username already exists!", 29, 0);
            else {
                client->user_data = create_user(username);
                strcpy(client->username, username);
                char resp[BUFF_SIZE];
                snprintf(resp, sizeof(resp), "RESP:Registered new user '%s'", username);
                send(client->socket_fd, resp, strlen(resp), 0);
            }
        } else if (strncmp(buff, "stats", 5) == 0) {
            if (client->user_data == NULL)
                send(client->socket_fd, "ERR_:Please login first to view stats", 37, 0);
            else {
                char resp[BUFF_SIZE];
                snprintf(resp, sizeof(resp),
                         "RESP:Stats: %s | Points: %d | Games: %d | Wins: %d | Win Rate: %.1f | Max Streak: %d",
                         client->username, client->user_data->total_points, client->user_data->games_played, client->user_data->games_won, 
                         (client->user_data->games_played > 0) ? (100.0 * client->user_data->games_won / client->user_data->games_played) : 0.0,
                         client->user_data->max_streak);
                send(client->socket_fd, resp, strlen(resp), 0);
            } 
        } else if (strncmp(buff, "start", 5) == 0) {
            pthread_mutex_lock(&game_session.lock);

            if (game_session.state != GAME_WAITING) {
                send_to_client(client, "ERR_:Game already started!\n");
                pthread_mutex_unlock(&game_session.lock);
                continue;
            }

            if (game_session.player_count < 2) {
                send_to_client(client, "ERR_:Need at least 2 players to start!\n");
                pthread_mutex_unlock(&game_session.lock);
                continue;
            }

            game_session.state = GAME_ACTIVE;
            for (int i = 0; i < game_session.player_count; i++)
                game_session.players[i]->state = CLIENT_IN_GAME;
            
            pthread_cond_signal(&game_session.game_start);
            pthread_mutex_unlock(&game_session.lock);

            printf(Green"[GAME] Game started by %s with %d players\n"Clear, client->username, game_session.player_count);
        } else if (strncmp(buff, "quit", 4) == 0) {
            send(client->socket_fd, "RESP:bye-bye!", 13, 0);
            
            if (client->state == CLIENT_IN_GAME || client->state == CLIENT_LOBBY) {
                remove_player(client);
            }
            client->state = CLIENT_DISCONNECTED;
            break;
        } else {
            send(client->socket_fd, "ERR_:Unrecognized Command", 25, 0);
        }
    }

    close(client->socket_fd);
    pthread_mutex_destroy(&client->lock);
    free(client);
    return NULL;
}

int main() {
    load_users();
    load_questions();

    if (question_count == 0) {
        printf(Red"[SERVER] No questions loaded! Cannot start server.\n"Clear);
        return 1;
    }

    memset(&game_session, 0, sizeof(game_session_t));
    game_session.state = GAME_WAITING;
    game_session.players = NULL;
    game_session.player_count = 0;
    game_session.curr_question_idx = 0;
    game_session.curr_player_turn = 0;
    pthread_mutex_init(&game_session.lock, NULL);
    pthread_cond_init(&game_session.game_start, NULL);

    pthread_t game_thread;
    pthread_create(&game_thread, NULL, game_loop, NULL);
    pthread_detach(game_thread);

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
    listen(server_fd, 10);
    printf(Green"[SERVER] Quiz Game Server started on port 8080\n"Clear);
    printf(Green"[SERVER] Loaded %d questions, ready for players!\n"Clear, question_count);

    while(1) {
        int client_socket = accept(server_fd, NULL, NULL);
        printf(Blue"[SERVER] New client connected! Socket: %d\n"Clear, client_socket);

        client_t* client = malloc(sizeof(client_t));
        if (!client) {
            close(client_socket);
            continue;
        }

        client->socket_fd = client_socket;
        client->score = 0;
        strcpy(client->username, "__anon__");
        client->user_data = NULL;
        client->state = CLIENT_CONNECTED;
        client->has_answered = false;
        client->answer = '\0';
        pthread_mutex_init(&client->lock, NULL);

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, client);
        pthread_detach(thread_id);
        printf(Blue"[SERVER] Created new thread for new client.\nWaiting new connections...\n"Clear);
    }

    pthread_mutex_destroy(&game_session.lock);
    pthread_cond_destroy(&game_session.game_start);
    return 0;
}