#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#define BUFF_SIZE 4096
#define MAX_NAME_LEN 256
#define Q_OPTION_SIZE 256

typedef struct {
    char username[MAX_NAME_LEN];
    char pwd[32];
    int total_points;
    int games_played;
    int games_won;
    int max_streak;
    int curr_streak;
    char last_login[64];
} user_data_t;

typedef enum {
    CLIENT_CONNECTED,
    CLIENT_LOBBY,
    CLIENT_IN_GAME,
    CLIENT_DISCONNECTED
} client_state_t;

typedef struct {
    int socket_fd;
    int score;
    char username[MAX_NAME_LEN];
    user_data_t* user_data;
    client_state_t state;
    int join_order;
    bool has_answered;
    char answer;
    time_t answer_time;
    pthread_mutex_t lock;
} client_t;

typedef struct {
    int id;
    char text[BUFF_SIZE / 4];
    char option_a[Q_OPTION_SIZE];
    char option_b[Q_OPTION_SIZE];
    char option_c[Q_OPTION_SIZE];
    char option_d[Q_OPTION_SIZE];
    char correct_answer;
    int points;
    char category[64];
    char difficulty[32];
    int time_limit;
} question_t;

typedef enum {
    GAME_WAITING,
    GAME_ACTIVE,
    GAME_FINISHED
} game_state_t;

typedef struct {
    client_t** players;
    int player_count;
    int max_players;
    int curr_question_idx;
    int curr_player_turn;
    game_state_t state;
    pthread_mutex_t lock;
    pthread_cond_t game_start;
    time_t question_start_time;
    bool game_started;
} game_session_t;