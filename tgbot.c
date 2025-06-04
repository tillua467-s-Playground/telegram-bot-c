#include "tgbot.h"

#define BOT_API "NULL"
#define Tg_link "https://api.telegram.org/bot" BOT_API "/"

long long last_update = 0;

size_t callback(char *ptr, size_t size, size_t nmemb, void *userdata){
    size_t total_size = size * nmemb;
    Mem_struct *mem = (struct Mem_struct *)userdata;

    char *tempptr = realloc(mem->memory, mem->size + total_size + 1);

    if(tempptr == NULL){ // if the pointer is NULL that means it's out of memory
        printf("Not enought memory(realloc returned NULL)\n");
        return 0;
    }

    mem->memory = tempptr;
    memcpy(&(mem->memory[mem->size]), ptr, total_size);
    mem->size +=total_size;
    mem->memory[mem->size] = 0;

    return total_size;
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
                                    fprintf(stderr, "Warning: Could not get valid 'update_id' from last update.\n");
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
                                        fprintf(stderr, "Warning: 'message_id' not found or not a number in message.\n");
                                    }

                                    cJSON *chat_info = cJSON_GetObjectItem(mesg, "chat");
                                    if (chat_info){
                                        cJSON *chat_id = cJSON_GetObjectItem(chat_info, "id");
                                        if (chat_id && cJSON_IsNumber(chat_id)){
                                            *group_chat_id = (long long) chat_id->valuedouble;
                                        } else {
                                            fprintf(stderr, "Warning: 'chat id' not found or not a number in chat info.\n");
                                        }
                                    } else {
                                        fprintf(stderr, "Warning: 'chat' object not found in message.\n");
                                    }

                                    cJSON *text_item = cJSON_GetObjectItem(mesg, "text");
                                    if(text_item && cJSON_IsString(text_item)){
                                        *C_RES = strdup(text_item->valuestring);
                                    } else {
                                        fprintf(stderr, "Info: 'text' field not found in message (e.g., sticker, photo, command).\n");
                                    }

                                    cJSON *from_info = cJSON_GetObjectItem(mesg, "from");
                                    if (from_info){
                                        cJSON *sender_id = cJSON_GetObjectItem(from_info, "id");
                                        if (sender_id && cJSON_IsNumber(sender_id)){
                                            *C_ID = (long long) sender_id->valuedouble;
                                        } else {
                                            fprintf(stderr, "Warning: 'from id' not found or not a number in sender info.\n");
                                        }
                                    } else {
                                        fprintf(stderr, "Warning: 'from' object not found in message.\n");
                                    }
                                } else {
                                    fprintf(stderr, "Info: First update is not a 'message' type (e.g., edited_message, callback_query).\n");
                                }
                            }
                        } else {
                            fprintf(stderr, "No new updates in 'result' array.\n");
                        }
                    } else {
                        fprintf(stderr, "Error: 'result' object not found or not an array.\n");
                    }
                } else {
                    fprintf(stderr, "Error: API response 'ok' status is false or not found.\n");
                }
            } else {
                fprintf(stderr, "Error: Failed to parse JSON response from Telegram API.\n");
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