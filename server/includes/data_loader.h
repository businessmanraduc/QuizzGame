#pragma once
#include "utils.h"
#include "libxml.h"

void load_users();
void load_questions();
void save_users();
char* int_to_str(int value);
user_data_t* find_user(const char* username);
user_data_t* create_user(const char* username);
