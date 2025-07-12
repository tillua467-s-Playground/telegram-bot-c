#ifndef TGBOT_H
#define TGBOT_H

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cjson/cJSON.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

typedef struct Mem_struct {
    char *memory;
    size_t size;
} Mem_struct;

typedef struct updateData {
    char *chat_Result;
    long long chat_id;
    long long group_chat_id;
    int reply_id;
    char *file_id;
} updateData;

// optional or helper functions
size_t callback(char *ptr, size_t size, size_t nmemb, void *userdata);
void last_up_add();
void last_update_read();
char* latest_file(const char* file_path);

// main functions
updateData *get_updates();
int send_message(long long *CID, const char *message, const int reply, long long reply_id);
void send_document(long long *CID, const char *file_name, const int reply, const int reply_id);
void delete_message(long long *CID, const int bot_msgid, long long *group_chat_id);
void edit_message(long long *CID, const int edit_msg_id, const char *message);

#endif