#include "data_loader.h"

extern user_data_t** users;
extern int user_count;
extern question_t** questions;
extern int question_count;

#define Clear   "\033[3;0;0m"
#define Black   "\033[0;30m"
#define Red     "\033[0;31m"
#define Green   "\033[0;32m"
#define Yellow  "\033[0;33m"
#define Blue    "\033[0;34m"
#define Purple  "\033[0;35m"
#define Cyan    "\033[0;36m"
#define White   "\033[0;37m"

void load_users() {
    XMLDocument doc;
    XMLError err = XMLDocument_load(&doc, "data/users.xml");
    
    if (err != XML_SUCCESS) {
        if (err == XML_ERROR_FILE) {
            printf(Yellow"[SERVER-XML] No users.xml found, starting with empty user database...\n"Clear);
        } else {
            printf(Red"[SERVER-XML] Error loading users.xml: %s\n"Clear, XMLDocument_etos(err));
        }
        return;
    }

    // Get all user nodes using the first version with attributes
    XMLNode* root = XMLNode_child(doc.root, 0);
    XMLNode* user_node;
    if (!root || strcmp(root->tag, "users") != 0) {
        printf(Red"[SERVER-XML] Invalid users.xml format\n"Clear);
        XMLDocument_free(&doc);
        return;
    }

    XML_FOREACH_CHILD(root, user_node) {
        if (!user_node->tag || strcmp(user_node->tag, "user") != 0)
            continue;
        
        user_data_t* user = malloc(sizeof(user_data_t));
        if (!user) continue;
        
        // Extract attributes from the user node
        char* username = XMLNode_attr_val(user_node, "username");
        if (username) {
            strncpy(user->username, username, MAX_NAME_LEN - 1);
            user->username[MAX_NAME_LEN - 1] = '\0';
        }
        
        char* points = XMLNode_attr_val(user_node, "points");
        user->total_points = points ? atoi(points) : 0;
        
        char* games = XMLNode_attr_val(user_node, "games");
        user->games_played = games ? atoi(games) : 0;
        
        char* wins = XMLNode_attr_val(user_node, "wins");
        user->games_won = wins ? atoi(wins) : 0;
        
        char* max_streak = XMLNode_attr_val(user_node, "max_streak");
        user->max_streak = max_streak ? atoi(max_streak) : 0;
        
        char* current_streak = XMLNode_attr_val(user_node, "current_streak");
        user->curr_streak = current_streak ? atoi(current_streak) : 0;
        
        XMLNode* login_child = XMLNode_child(user_node, 2);
        char* last_login = login_child->inner_text;
        //printf("[DEBUG] last_login value : %s\n\n", last_login);
        if (last_login) {
            strncpy(user->last_login, last_login, sizeof(user->last_login) - 1);
            user->last_login[sizeof(user->last_login) - 1] = '\0';
        }
        
        // Add to users array
        users = realloc(users, (user_count + 1) * sizeof(user_data_t*));
        users[user_count] = user;
        user_count++;
    }

    XMLDocument_free(&doc);
    printf(Cyan"[SERVER-XML] Loaded %d users from database\n"Clear, user_count);
}

void load_questions() {
    XMLDocument doc;
    XMLError err = XMLDocument_load(&doc, "data/questions.xml");
    
    if (err != XML_SUCCESS) {
        printf(Red"[SERVER-XML] Error - questions.xml not found or invalid: %s\n"Clear, XMLDocument_etos(err));
        exit(1);
    }

    XMLNode* root = XMLNode_child(doc.root, 0);
    if (!root || !root->tag || strcmp(root->tag, "questions") != 0) {
        printf(Red"[SERVER-XML] Invalid questions.xml format - expected <questions> as root\n"Clear);
        exit(1);
    }

    XMLNode* question_node;
    XML_FOREACH_CHILD(root, question_node) {
        if (!question_node->tag || strcmp(question_node->tag, "question") != 0)
            continue;
        
        question_t* question = malloc(sizeof(question_t));
        if (!question) continue;

        memset(question, 0, sizeof(question_t));

        char* id_attr = XMLNode_attr_val(question_node, "id");
        if (id_attr)
            question->id = atoi(id_attr);

        char* points_attr = XMLNode_attr_val(question_node, "points");
        if (points_attr)
            question->points = atoi(points_attr);

        char* time_limit_attr = XMLNode_attr_val(question_node, "time_limit");
        if (time_limit_attr)
            question->time_limit = atoi(time_limit_attr);

        char* category_attr = XMLNode_attr_val(question_node, "category");
        if (category_attr) {
            strncpy(question->category, category_attr, sizeof(question->category) - 1);
            question->category[sizeof(question->category) - 1] = '\0';
        }

        char* difficulty_attr = XMLNode_attr_val(question_node, "difficulty");
        if (difficulty_attr) {    
            strncpy(question->difficulty, difficulty_attr, sizeof(question->difficulty) - 1);
            question->difficulty[sizeof(question->difficulty) - 1] = '\0';
        }

        XMLNode* child_node;
        XML_FOREACH_CHILD(question_node, child_node) {
            if (!child_node->tag)
                continue;
            
            if (strcmp(child_node->tag, "text") == 0 && child_node->inner_text) {
                strncpy(question->text, child_node->inner_text, BUFF_SIZE / 4 - 1);
                question->text[BUFF_SIZE / 4 - 1] = '\0';
            } else if (strcmp(child_node->tag, "correct_answer") == 0 && child_node->inner_text) {
                question->correct_answer = child_node->inner_text[0];
            } else if (strcmp(child_node->tag, "options") == 0) {
                XMLNode* option_node;
                XML_FOREACH_CHILD(child_node, option_node) {
                    if (!option_node->tag || strcmp(option_node->tag, "option") != 0)
                        continue;
                    
                    char* letter_attr = XMLNode_attr_val(option_node, "letter");
                    if (!letter_attr || !option_node->inner_text)
                        continue;

                    if (letter_attr[0] == 'A' || letter_attr[0] == 'a') {
                        strncpy(question->option_a, option_node->inner_text, Q_OPTION_SIZE - 1);
                        question->option_a[Q_OPTION_SIZE - 1] = '\0';
                    } else if (letter_attr[0] == 'B' || letter_attr[0] == 'b') {
                        strncpy(question->option_b, option_node->inner_text, Q_OPTION_SIZE - 1);
                        question->option_b[Q_OPTION_SIZE - 1] = '\0';
                    } else if (letter_attr[0] == 'C' || letter_attr[0] == 'c') {
                        strncpy(question->option_c, option_node->inner_text, Q_OPTION_SIZE - 1);
                        question->option_c[Q_OPTION_SIZE - 1] = '\0';
                    } else if (letter_attr[0] == 'D' || letter_attr[0] == 'd') {
                        strncpy(question->option_d, option_node->inner_text, Q_OPTION_SIZE - 1);
                        question->option_d[Q_OPTION_SIZE - 1] = '\0';
                    }
                }
            }          
        }
        
        questions = realloc(questions, (question_count + 1) * sizeof(question_t*));
        questions[question_count] = question;
        question_count++;
    }

    XMLDocument_free(&doc);
    printf(Cyan"[SERVER-XML] Loaded %d questions\n"Clear, question_count);
}

void save_users() {
    XMLDocument doc;
    doc.version = strdup("1.0");
    doc.encoding = strdup("UTF-8");
    doc.root = XMLNode_new(NULL);
    doc.root->tag = NULL;

    XMLNode* users_node = XMLNode_new(doc.root);
    users_node->tag = strdup("users");
    
    for (int i = 0; i < user_count; i++) {
        XMLNode* user_node = XMLNode_new(users_node);
        user_node->tag = strdup("user");
        
        // Add username attribute
        XMLAttribute attr;
        attr.key = strdup("username");
        attr.value = strdup(users[i]->username);
        XMLAttributeList_add(&user_node->attributes, &attr);
        
        // Create stats node
        XMLNode* stats_node = XMLNode_new(user_node);
        stats_node->tag = strdup("stats");
        
        char buffer[32];
        
        snprintf(buffer, sizeof(buffer), "%d", users[i]->total_points);
        attr.key = strdup("points");
        attr.value = strdup(buffer);
        XMLAttributeList_add(&stats_node->attributes, &attr);
        
        snprintf(buffer, sizeof(buffer), "%d", users[i]->games_played);
        attr.key = strdup("games");
        attr.value = strdup(buffer);
        XMLAttributeList_add(&stats_node->attributes, &attr);
        
        snprintf(buffer, sizeof(buffer), "%d", users[i]->games_won);
        attr.key = strdup("wins");
        attr.value = strdup(buffer);
        XMLAttributeList_add(&stats_node->attributes, &attr);
        
        // Create streaks node
        XMLNode* streaks_node = XMLNode_new(user_node);
        streaks_node->tag = strdup("streaks");
        
        snprintf(buffer, sizeof(buffer), "%d", users[i]->max_streak);
        attr.key = strdup("max");
        attr.value = strdup(buffer);
        XMLAttributeList_add(&streaks_node->attributes, &attr);
        
        snprintf(buffer, sizeof(buffer), "%d", users[i]->curr_streak);
        attr.key = strdup("current");
        attr.value = strdup(buffer);
        XMLAttributeList_add(&streaks_node->attributes, &attr);
        
        // Create last_login node with text content
        XMLNode* last_login_node = XMLNode_new(user_node);
        last_login_node->tag = strdup("last_login");
        
        char time_buff[20];
        strncpy(time_buff, users[i]->last_login, sizeof(time_buff) - 1);
        time_buff[sizeof(time_buff) - 1] = '\0';
        last_login_node->inner_text = strdup(time_buff);
    }
    
    bool success = XMLDocument_write(&doc, "data/users.xml", 2);
    if (!success) {
        printf(Red"[SERVER-XML] Error saving users to database\n"Clear);
    } else {
        printf(Cyan"[SERVER-XML] Saved users to database\n"Clear);
    }
    
    XMLDocument_free(&doc);
}

char* int_to_str(int value) {
    static char buff[32];
    snprintf(buff, sizeof(buff), "%d", value);
    return buff;
}

user_data_t* find_user(const char* username) {
    for (int i = 0; i < user_count; i++)
        if (strcmp(users[i]->username, username) == 0)
            return users[i];
    
    return NULL;
}

user_data_t* create_user(const char* username) {
    user_data_t* user = malloc(sizeof(user_data_t));
    strncpy(user->username, username, MAX_NAME_LEN - 1);
    user->username[MAX_NAME_LEN - 1] = '\0';
    user->total_points = 0;
    user->games_played = 0;
    user->games_won = 0;
    user->max_streak = 0;
    user->curr_streak = 0;
    
    time_t now = time(NULL);
    strftime(user->last_login, sizeof(user->last_login), "%Y-%m-%d %H:%M:%S", localtime(&now));
    users = realloc(users, (user_count + 1) * sizeof(user_data_t*));
    users[user_count] = user;
    user_count++;

    save_users();
    return user;
}