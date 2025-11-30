#include "xml.h"

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
    xmlDoc* doc = xmlReadFile("data/users.xml", NULL, 0);
    if (doc == NULL) {
        printf(Yellow"[SERVER-XML] No users.xml found, starting with empty user database...\n"Clear);
        return;
    }

    xmlNode* root = xmlDocGetRootElement(doc);
    for (xmlNode* node = root->children; node != NULL; node = node->next) {
        if (node->type == XML_ELEMENT_NODE && xmlStrcmp(node->name, (const xmlChar*)"user") == 0) {
            user_data_t* user = malloc(sizeof(user_data_t));
            for (xmlNode* user_node = node->children; user_node != NULL; user_node = user_node->next) {
                if (user_node->type == XML_ELEMENT_NODE) {
                    char* content = (char*)xmlNodeGetContent(user_node);
                    if (xmlStrcmp(user_node->name, (const xmlChar*)"username") == 0)
                        strncpy(user->username, content, MAX_NAME_LEN - 1);
                    else if (xmlStrcmp(user_node->name, (const xmlChar*)"total_points") == 0)
                        user->total_points = atoi(content);
                    else if (xmlStrcmp(user_node->name, (const xmlChar*)"games_played") == 0)
                        user->games_played = atoi(content);
                    else if (xmlStrcmp(user_node->name, (const xmlChar*)"max_streak") == 0)
                        user->max_streak = atoi(content);
                    else if (xmlStrcmp(user_node->name, (const xmlChar*)"current_streak") == 0)
                        user->curr_streak = atoi(content);
                    else if (xmlStrcmp(user_node->name, (const xmlChar*)"last_login") == 0)
                        strncpy(user->last_login, content, sizeof(user->last_login) - 1);

                    xmlFree(content);
                }
            }
            users = realloc(users, (user_count + 1) * sizeof(user_data_t*));
            users[user_count] = user;
            user_count++;
        }
    }

    xmlFreeDoc(doc);
    printf(Cyan"[SERVER-XML] Loaded %d users from database\n"Clear, user_count);
}

void load_questions() {
    xmlDoc* doc = xmlReadFile("data/questions.xml", NULL, 0);
    if (doc == NULL) {
        printf(Red"[SERVER-XML] Error - questions.xml not found!\n"Clear);
        exit(1);
    }

    xmlNode* root = xmlDocGetRootElement(doc);
    for (xmlNode* node = root->children; node != NULL; node = node->next) {
        if (node->type == XML_ELEMENT_NODE && xmlStrcmp(node->name, (const xmlChar*)"question") == 0) {
            question_t* question = malloc(sizeof(question_t));
            for (xmlNode* q_node = node->children; q_node != NULL; q_node = q_node->next) {
                if (q_node->type == XML_ELEMENT_NODE) {
                    char* content = (char*)xmlNodeGetContent(q_node);
                    if (xmlStrcmp(q_node->name, (const xmlChar*)"id") == 0)
                        question->id = atoi(content);
                    else if (xmlStrcmp(q_node->name, (const xmlChar*)"text") == 0)
                        strncpy(question->text, content, BUFF_SIZE);
                    else if (xmlStrcmp(q_node->name, (const xmlChar*)"option_a") == 0)
                        strncpy(question->option_a, content, Q_OPTION_SIZE);
                    else if (xmlStrcmp(q_node->name, (const xmlChar*)"option_b") == 0)
                        strncpy(question->option_b, content, Q_OPTION_SIZE);
                    else if (xmlStrcmp(q_node->name, (const xmlChar*)"option_c") == 0)
                        strncpy(question->option_c, content, Q_OPTION_SIZE);
                    else if (xmlStrcmp(q_node->name, (const xmlChar*)"option_d") == 0)
                        strncpy(question->option_d, content, Q_OPTION_SIZE);
                    else if (xmlStrcmp(q_node->name, (const xmlChar*)"correct_answer") == 0)
                        question->correct_answer = content[0];
                    else if (xmlStrcmp(q_node->name, (const xmlChar*)"points") == 0)
                        question->points = atoi(content);
                    else if (xmlStrcmp(q_node->name, (const xmlChar*)"category") == 0)
                        strncpy(question->category, content, sizeof(question->category) - 1);
                    else if (xmlStrcmp(q_node->name, (const xmlChar*)"difficulty") == 0)
                        strncpy(question->difficulty, content, sizeof(question->difficulty) - 1);
                    else if (xmlStrcmp(q_node->name, (const xmlChar*)"time_limit") == 0)
                        question->time_limit = atoi(content);

                    xmlFree(content);
                }
            }

            questions = realloc(questions, (question_count + 1) * sizeof(question_t*));
            questions[question_count] = question;
            question_count++;
        }
    }

    xmlFreeDoc(doc);
    printf(Cyan"[SERVER-XML] Loaded %d questions\n"Clear, question_count);
}

void save_users() {
    xmlDoc* doc = xmlNewDoc((const xmlChar*)"1.0");
    xmlNode* root = xmlNewNode(NULL, (const xmlChar*)"users");
    xmlDocSetRootElement(doc, root);

    for (int i = 0; i < user_count; i++) {
        xmlNode* user_node = xmlNewChild(root, NULL, (const xmlChar*)"user", NULL);
        xmlNewChild(user_node, NULL, (const xmlChar*)"username", (const xmlChar*)users[i]->username);
        xmlNewChild(user_node, NULL, (const xmlChar*)"total_points", (const xmlChar*)int_to_str(users[i]->total_points));
        xmlNewChild(user_node, NULL, (const xmlChar*)"games_played", (const xmlChar*)int_to_str(users[i]->games_played));
        xmlNewChild(user_node, NULL, (const xmlChar*)"games_won", (const xmlChar*)int_to_str(users[i]->games_won));
        xmlNewChild(user_node, NULL, (const xmlChar*)"max_streak", (const xmlChar*)int_to_str(users[i]->max_streak));
        xmlNewChild(user_node, NULL, (const xmlChar*)"current_streak", (const xmlChar*)int_to_str(users[i]->curr_streak));
        xmlNewChild(user_node, NULL, (const xmlChar*)"last_login", (const xmlChar*)users[i]->last_login);
    }

    xmlSaveFormatFile("data/users.xml", doc, 1);
    xmlFreeDoc(doc);
    printf(Cyan"[SERVER-XML] Saved users to database\n"Clear);
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