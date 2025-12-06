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

#define BUFF_SIZE 1024
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

typedef struct {
    int id;
    char text[BUFF_SIZE];
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