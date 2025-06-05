#include "tgbot.h"

#define BOT_API "NULL"
#define Tg_link "https://api.telegram.org/bot" BOT_API "/"

long long last_update = 0;

size_t callback(char *ptr, size_t size, size_t nmemb, void *userdata){
    size_t total_size = size * nmemb;
    Mem_struct *mem = (struct Mem_struct *)userdata;

    char *tempptr = realloc(mem->memory, mem->size + total_size + 1);

    if(tempptr == NULL){
        printf("Not enought memory(realloc returned NULL)\n");
        return 0;
    }

    mem->memory = tempptr;
    memcpy(&(mem->memory[mem->size]), ptr, total_size);
    mem->size +=total_size;
    mem->memory[mem->size] = 0;

    return total_size;
}

char* latest_file(const char* file_path){

    DIR* dir = opendir(file_path);
    struct dirent* entry;
    struct stat file_info;
    char* latest = NULL;
    time_t latest_modified_time = 0;

    if (dir == NULL){
        fprintf(stderr, "Failed to open the directory\n");
        return NULL;
    } while((entry = readdir(dir)) != NULL){
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        continue;

        size_t full_path_len = strlen(file_path) + strlen(entry->d_name) + 2;
        char* path = malloc(full_path_len);
        if(!path){
            fprintf(stderr, "malloc failed\n");
            free(latest);
        }
        snprintf(path, full_path_len, "%s/%s", file_path, entry->d_name);
        if (stat(path, &file_info) == 0){
            if(file_info.st_mtime > latest_modified_time){
                latest_modified_time = file_info.st_mtime;
                free(latest);
                latest = strdup(entry->d_name);
            }
        }
        free(path);
    }

    closedir(dir);
    return latest;
}

void last_up_add(){
    FILE *file = fopen("last_update.txt", "w");
    if (file) {
        fprintf(file, "%lld", last_update);
        fclose(file);
    }
}

void last_update_read(){
    FILE *file = fopen("last_update.txt", "r");
    if (file) {
        fscanf(file, "%lld", &last_update);
        fclose(file);
    }

}

void get_updates(char **C_RES, long long *C_ID, long long *group_chat_id, int *reply_id){
    CURL *curl;
    CURLcode res;
    char url[512];
    Mem_struct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    *C_RES = NULL;

    last_update_read();
    sprintf(url, "%sgetUpdates?offset=%lld&limit=15", Tg_link, last_update + 1);
    curl = curl_easy_init();

    if(curl){
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);

        if(res != CURLE_OK){
            fprintf(stderr, "cURL failed: %s\n", curl_easy_strerror(res));
        } else {
            printf("Received response: %s\n", chunk.memory);

            cJSON *root = cJSON_Parse(chunk.memory);
            if(root){
                cJSON *ok_status = cJSON_GetObjectItem(root, "ok");
                if (ok_status && cJSON_IsTrue(ok_status)) {
                    cJSON *result = cJSON_GetObjectItem(root, "result");
                    if (result && cJSON_IsArray(result)){
                        int num_results = cJSON_GetArraySize(result);

                        if (num_results > 0){
                            cJSON *last_update_obj = cJSON_GetArrayItem(result, num_results - 1);
                            if (last_update_obj) {
                                cJSON *up_id = cJSON_GetObjectItem(last_update_obj, "update_id");
                                if (up_id && cJSON_IsNumber(up_id)) {
                                    last_update = (long long) up_id->valuedouble;
                                    last_up_add();
                                } else {
                                    fprintf(stderr, "Error: Could not get update_id from last update.\n");
                                }
                            }
                            cJSON *first_update_obj = cJSON_GetArrayItem(result, 0);
                            if (first_update_obj) {
                                cJSON *mesg = cJSON_GetObjectItem(first_update_obj, "message");
                                if (mesg){
                                    cJSON *reply_msg_id = cJSON_GetObjectItem(mesg, "message_id");
                                    if (reply_msg_id && cJSON_IsNumber(reply_msg_id)){
                                        *reply_id = (int) reply_msg_id->valueint;
                                    } else {
                                        fprintf(stderr, "Error: message_id not found in message.\n");
                                    }

                                    cJSON *chat_info = cJSON_GetObjectItem(mesg, "chat");
                                    if (chat_info){
                                        cJSON *chat_id = cJSON_GetObjectItem(chat_info, "id");
                                        if (chat_id && cJSON_IsNumber(chat_id)){
                                            *group_chat_id = (long long) chat_id->valuedouble;
                                        } else {
                                            fprintf(stderr, "Error: chat id not found in chat info.\n");
                                        }
                                    } else {
                                        fprintf(stderr, "Error: chat object not found in message.\n");
                                    }

                                    cJSON *text_item = cJSON_GetObjectItem(mesg, "text");
                                    if(text_item && cJSON_IsString(text_item)){
                                        *C_RES = strdup(text_item->valuestring);
                                    } else {
                                        fprintf(stderr, "Error: text field not found in message\n");
                                    }

                                    cJSON *from_info = cJSON_GetObjectItem(mesg, "from");
                                    if (from_info){
                                        cJSON *sender_id = cJSON_GetObjectItem(from_info, "id");
                                        if (sender_id && cJSON_IsNumber(sender_id)){
                                            *C_ID = (long long) sender_id->valuedouble;
                                        } else {
                                            fprintf(stderr, "Error: from id not found in sender info.\n");
                                        }
                                    } else {
                                        fprintf(stderr, "Error: from object not found in message.\n");
                                    }
                                } else {
                                    fprintf(stderr, "Error: First update is not a message\n");
                                }
                            }
                        } else {
                            fprintf(stderr, "No new updates\n");
                        }
                    } else {
                        fprintf(stderr, "Error: result object not found\n");
                    }
                } else {
                    fprintf(stderr, "Error: ok status is false\n");
                }
            } else {
                fprintf(stderr, "Error: Failed to parse JSON\n");
            }
            cJSON_Delete(root);
        }
        curl_easy_cleanup(curl);
    } else {
        fprintf(stderr, "Error: Failed to initialize cURL.\n");
    }

    free(chunk.memory);
    sleep(1);
}

void send_message(long long *CID, const char *message, long long reply_id){
    CURL *curl;
    CURLcode res;
    Mem_struct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;
    char *url;

    curl = curl_easy_init();

    if(curl){
        char *escaped_message = curl_easy_escape(curl, message, 0);

        if(reply_id == -1){
            int url_length = snprintf(NULL, 0, "%ssendMessage?chat_id=%lld&text=%s", Tg_link, *CID, escaped_message);
            url = malloc(url_length + 1);
            snprintf(url, url_length + 1, "%ssendMessage?chat_id=%lld&text=%s", Tg_link, *CID, escaped_message);
            curl_free(escaped_message);
        } else{
            int url_length = snprintf(NULL, 0, "%ssendMessage?chat_id=%lld&text=%s&reply_to_message=%lld", Tg_link, *CID, escaped_message, reply_id);
            url = malloc(url_length + 1);
            snprintf(url, url_length + 1, "%ssendMessage?chat_id=%lld&text=%s&reply_to_message=%lld", Tg_link, *CID, escaped_message, reply_id);
            curl_free(escaped_message);
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);

        if(res != CURLE_OK){
            fprintf(stderr, "Failed: %s\n", curl_easy_strerror(res));
        }
        printf("Measage was sent to %lld successfully", *CID);
        printf("server response:\n%s\n", chunk.memory);

        free(chunk.memory);
        free(url);
        curl_easy_cleanup(curl);
    } else{
        fprintf(stderr, "Failed to initialize cURL\n");
    }
}

