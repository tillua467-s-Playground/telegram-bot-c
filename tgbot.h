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

void get_updates(char **C_RES, long long *C_ID, long long *group_chat_id, int *reply_id);
void send_message(long long *CID, const char *message, long long reply_id);

#endif