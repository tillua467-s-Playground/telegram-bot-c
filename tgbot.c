#include "tgbot.h"

#define BOT_API "NULL"
#define Tg_link "https://api.telegram.org/bot" BOT_API "/"
#define Tg_link_file "https://api.telegram.org/file/bot" BOT_API "/"

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

void cdir(const char *filepath) {
    int size = sizeof(filepath);
    char *path_copy = malloc(size);
    strncpy(path_copy, filepath, sizeof(path_copy));
    path_copy[sizeof(path_copy) - 1] = '\0';

    char *slash = strrchr(path_copy, '/');
    if (slash) {
        *slash = '\0';
        mkdir_p(path_copy);
    }
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

updateData *get_updates(){
    updateData* userdata = malloc(sizeof(updateData));
    userdata->chat_id = -1;
    userdata->chat_Result = "chat";
    userdata->group_chat_id = -1;
    userdata->reply_id = -1;
    userdata->file_id = "file_id=NULL";
    CURL *curl;
    CURLcode res;
    char url[512];
    Mem_struct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

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
            if(!root)
                fprintf(stderr, "Error: Failed to parse JSON\n");
            
            cJSON *ok_status = cJSON_GetObjectItem(root, "ok");
            if (cJSON_IsFalse(ok_status)){
                fprintf(stderr, "Error: ok status is false, there was some error getting updates\n");
                return NULL;
            }
           cJSON *result = cJSON_GetObjectItem(root, "result");
            if (!result)
                fprintf(stderr, "Error: result object not found\n");

            int num_results = cJSON_GetArraySize(result);
            if (num_results == 0){
                fprintf(stderr, "No new updates\n");
                cJSON_Delete(root);
                curl_easy_cleanup(curl);
                free(chunk.memory);
                return NULL;
            }

            cJSON *last_update_obj = cJSON_GetArrayItem(result, num_results - 1);
            if (!last_update_obj)
                fprintf(stderr, "last update is NULL\n");

            cJSON *up_id = cJSON_GetObjectItem(last_update_obj, "update_id");
            if (!up_id)
                fprintf(stderr, "Error: Could not get update_id from last update.\n");

            last_update = (long long) up_id->valuedouble;
            last_up_add();

            cJSON *first_update_obj = cJSON_GetArrayItem(result, 0);
            if (!first_update_obj)
                fprintf(stderr, "Error: Couldn't parse RESULT\n");

            cJSON *mesg = cJSON_GetObjectItem(first_update_obj, "message");
            if (!mesg)
                fprintf(stderr, "Error: First update is not a message\n");

            cJSON *reply_msg_id = cJSON_GetObjectItem(mesg, "message_id");
            if (!reply_msg_id){
                fprintf(stderr, "Error: message_id not found in message.\n");
                cJSON_Delete(root);
                curl_easy_cleanup(curl);
                free(chunk.memory);
                return NULL;
            }
            cJSON *pic_det = cJSON_GetObjectItem(mesg, "photo");
            if(pic_det){
                
                int photo_count = cJSON_GetArraySize(pic_det);
                if(pic_det == 0) return NULL;
                cJSON *tpic = cJSON_GetArrayItem(pic_det, photo_count - 1);
                if(tpic == 0) return NULL;
                cJSON *pic_id = cJSON_GetObjectItem(tpic, "file_id");
                if(!pic_id){
                    fprintf(stderr, "Error: tpic buffer is empty\n");
                    cJSON(root);
                    curl_easy_cleanup(curl);
                    free(chunk.memory);
                    return NULL;
                }
                userdata->file_id = pic_id->valuestring;
            }
            cJSON *vid_det = cJSON_GetObjectItem(mesg, "video");        
            if(vid_det){
                if(!vid_det)
                    fprintf(stderr, "Error: video details are missing.\n");
                cJSON *vid = cJSON_GetObjectItem(vid_det, "file_id");
                if(!vid){
                    fprintf(stderr, "Error: video id is missing.\n");
                    cJSON(root);
                    curl_easy_cleanup(curl);
                    free(chunk.memory);
                    return NULL;
                }
                userdata->file_id = vid->valuestring;
            }
            cJSON *aud_det = cJSON_GetObjectItem(mesg, "audio");
            if(aud_det){
                if(!aud_det)
                    fprintf(stderr, "Error: audio details are missing.\n");
                cJSON *vid = cJSON_GetObjectItem(aud_det, "file_id");
                if(!vid){
                    fprintf(stderr, "Error: audio id is missing.\n");
                    cJSON(root);
                    curl_easy_cleanup(curl);
                    free(chunk.memory);
                    return NULL;
                }
                userdata->file_id = vid->valuestring;
            }
            cJSON *doc = cJSON_GetObjectItem(mesg, "document");
            if(doc){
                if(!doc)
                    fprintf(stderr, "Error: document not found.\n");
                cJSON *doc_id = cJSON_GetObjectItem(doc, "file_id");
                if(!doc_id){
                    fprintf(stderr, "Error: doc_id not found.\n");
                    cJSON_Delete(root);
                    curl_easy_cleanup(curl);
                    free(chunk.memory);
                    return NULL;
                }
                userdata->file_id = doc_id->valuestring;
            }

            userdata->reply_id = (int) reply_msg_id->valueint;

            cJSON *chat_info = cJSON_GetObjectItem(mesg, "chat");
            if (!chat_info)
                fprintf(stderr, "Error: chat object not found in message.\n");

            cJSON *chat_id = cJSON_GetObjectItem(chat_info, "id");
            if (!chat_id){
                fprintf(stderr, "Error: chat id not found in chat info.\n");
                cJSON_Delete(root);
                curl_easy_cleanup(curl);
                free(chunk.memory);
                return NULL;
            }

            userdata->group_chat_id = (long long) chat_id->valuedouble;

            if(cJSON_GetObjectItem(mesg, "text")){
                cJSON *text_item = cJSON_GetObjectItem(mesg, "text");
                if(!text_item){
                    fprintf(stderr, "Error: text field not found in message\n");
                    cJSON_Delete(root);
                    curl_easy_cleanup(curl);
                    free(chunk.memory);
                    return NULL;
                }
                userdata->chat_Result = strdup(text_item->valuestring);
            }
            cJSON *from_info = cJSON_GetObjectItem(mesg, "from");
            if (!from_info)
                fprintf(stderr, "Error: from object not found in message.\n");
            cJSON *sender_id = cJSON_GetObjectItem(from_info, "id");
            if (!sender_id){
                fprintf(stderr, "Error: from id not found in sender info.\n");
                cJSON_Delete(root);
                curl_easy_cleanup(curl);
                free(chunk.memory);
                return NULL;
            }
            userdata->chat_id = (long long) sender_id->valuedouble;
        }
        curl_easy_cleanup(curl);
    } else {
        fprintf(stderr, "Error: Failed to initialize cURL.\n");
    }

    free(chunk.memory);

    return userdata;
}

void get_document(char *file_id){
    CURL *curl;
    CURLcode res;
    Mem_struct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;
    char *url;

    curl = curl_easy_init();

    if(!curl)
        fprintf(stderr, "Failed to initialize cURL\n");
    
    int url_lenght = snprintf(NULL, 0, "%sgetFile?file_id=%s", Tg_link, file_id);
    url = malloc(url_lenght + 1);
    snprintf(url, url_lenght + 1, "%sgetFile?file_id=%s", Tg_link, file_id);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
        fprintf(stderr, "Failed: %s\n", curl_easy_strerror(res));

    printf("file_id: %s got it!\n", chunk.memory);
    // file download
    cJSON *root = cJSON_Parse(chunk.memory);
    if(!root)
        fprintf(stderr, "Failed: to parse json\n");
    cJSON *status = cJSON_GetObjectItem(root, "ok");
    if(!status)
        fprintf(stderr, "Failed: to get the status\n");
    if(cJSON_IsFalse(status)){
        fprintf(stderr, "Error: ok status is false, there was some error getting file address\n");
        return;
    }
    cJSON *resu = cJSON_GetObjectItem(root, "result");
    if(!resu)
        fprintf(stderr, "Failed: to get the result\n");
    cJSON *pFILE = cJSON_GetObjectItem(resu, "file_path");
    if(!pFILE)
        fprintf(stderr, "Error: file_path is missing\n");

    free(chunk.memory);
    free(url);
    curl_easy_cleanup(curl);

    Mem_struct chunk2;
    chunk2.memory = malloc(1);
    chunk2.size = 0;

    curl = curl_easy_init();

    if(!curl)
        fprintf(stderr, "Failed to initialize cURL\n");
    
    int url_len = snprintf(NULL, 0, "%s%s", Tg_link_file, pFILE->valuestring);
    url = malloc(url_len + 1);
    snprintf(url, url_len + 1, "%s%s", Tg_link_file, pFILE->valuestring);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk2);

    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
        fprintf(stderr, "Failed: %s\n", curl_easy_strerror(res));

    printf("SAVING FILE: %s", pFILE->valuestring);
    cdir(pFILE->valuestring);

    FILE *fp = fopen(pFILE->valuestring, "wb");
    if (!fp)
        fprintf(stderr, "Error opening file for writing: %s\n", pFILE->valuestring);
    else {
        fwrite(chunk2.memory, 1, chunk2.size, fp);
        fclose(fp);
        printf("File saved as: %s\n", pFILE->valuestring);
    }

    printf("Server response: %s\n", chunk2.memory);
    free(chunk2.memory);
    free(url);
    curl_easy_cleanup(curl);
}

int send_message(long long *CID, const char *message, const int reply, long long reply_id){
    CURL *curl;
    CURLcode res;
    Mem_struct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;
    char *url;
    int bot_msgid = -1;

    curl = curl_easy_init();

    if(curl){
        char *escaped_message = curl_easy_escape(curl, message, 0);

        if(reply == 0){
            int url_length = snprintf(NULL, 0, "%ssendMessage?chat_id=%lld&text=%s", Tg_link, *CID, escaped_message);
            url = malloc(url_length + 1);
            snprintf(url, url_length + 1, "%ssendMessage?chat_id=%lld&text=%s", Tg_link, *CID, escaped_message);
            curl_free(escaped_message);
        } else{
            int url_length = snprintf(NULL, 0, "%ssendMessage?chat_id=%lld&text=%s&reply_to_message_id=%lld", Tg_link, *CID, escaped_message, reply_id);
            url = malloc(url_length + 1);
            snprintf(url, url_length + 1, "%ssendMessage?chat_id=%lld&text=%s&reply_to_message_id=%lld", Tg_link, *CID, escaped_message, reply_id);
            curl_free(escaped_message);
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);

        if(res != CURLE_OK){
            fprintf(stderr, "Failed: %s\n", curl_easy_strerror(res));
        }
        printf("Measage was sent to %lld successfully\n", *CID);
        printf("server response:\n%s\n", chunk.memory);

        cJSON *root = cJSON_Parse(chunk.memory);
        if (!root)
            fprintf(stderr, "Failed to parse JSON\n");

        cJSON *result = cJSON_GetObjectItem(root, "result");
        if (result)
            fprintf(stderr, "Failed to parse JSON\n");

        cJSON *msg_id = cJSON_GetObjectItem(result, "message_id");
        if (!msg_id){
            fprintf(stderr, "Failed to parse JSON\n");
            cJSON_Delete(root);
            return -1;
        }
        
        bot_msgid = msg_id->valueint;
        printf("Bot message ID: %d\n", bot_msgid);
        cJSON_Delete(root);
        free(chunk.memory);
        free(url);
        curl_easy_cleanup(curl);
    } else{
        fprintf(stderr, "Failed to initialize cURL\n");
    }
    return bot_msgid;
}


void send_document(long long *CID, const char *file_name, const int reply, const int reply_id){
    CURL *curl;
    CURLcode res;
    char *url;
    char cid_strn[64];
    snprintf(cid_strn, sizeof(cid_strn), "%lld", *CID);

    struct stat st;
    if(stat(file_name, &st) != 0){
        fprintf(stderr, "Error: stat\n");
        return;
    }

    curl = curl_easy_init();
    if(curl){
        int url_length = snprintf(NULL, 0, "%ssendDocument", Tg_link);
        url = malloc(url_length + 1);
        snprintf(url, url_length + 1, "%ssendDocument", Tg_link);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_mime *form = curl_mime_init(curl);
        // filed1 -> hold chat id -> where if the file is going
        curl_mimepart *field1 = curl_mime_addpart(form);
        curl_mime_name(field1, "chat_id");
        curl_mime_data(field1, cid_strn, CURL_ZERO_TERMINATED);
        // filed2 -> hold the document -> that will be sended
        curl_mimepart *field2 = curl_mime_addpart(form);
        curl_mime_name(field2, "document");
        curl_mime_filedata(field2, file_name);
        // feild3 -> if the user wanna reply it
        if(reply == 0){
            char reply_id_char[32];
            snprintf(reply_id_char, sizeof(reply_id_char), "%d", reply_id);
            curl_mimepart *field3 = curl_mime_addpart(form);
            curl_mime_name(field3, "reply_to_message_id");
            curl_mime_data(field3, reply_id_char, CURL_ZERO_TERMINATED);
        }
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
        res = curl_easy_perform(curl);
        if (res == CURLE_OK){
            fprintf(stderr, "Uploaded!: %s\n", file_name);
            curl_mime_free(form);
            curl_easy_cleanup(curl);
        } else {
            fprintf(stderr, "Upload failed: %s\n", curl_easy_strerror(res));
        }
    } else {
        fprintf(stderr, "Failed to initialize cURL\n");
    }

    free(url);
}

void delete_message(long long *CID, const int bot_msgid, long long *group_chat_id){
    CURL *curl;
    CURLcode res;
    Mem_struct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;
    char url[1024];

    curl = curl_easy_init();
    if (curl){
        if (*group_chat_id != -1){
        snprintf(url, sizeof(url), "%sdeleteMessage?chat_id=%lld&message_id=%d", Tg_link, *group_chat_id, bot_msgid);
        } else{
            snprintf(url, sizeof(url), "%sdeleteMessage?chat_id=%lld&message_id=%d", Tg_link, *CID, bot_msgid);
        }
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK){
            fprintf(stderr, "Failed: %s\n", curl_easy_strerror(res));
        }
        printf("server response:\n%s\n", chunk.memory);

        free(chunk.memory);
        curl_easy_cleanup(curl);
    } else {
        fprintf(stderr, "Failed to initialize cURL\n");
    }
}

void edit_message(long long *CID, const int edit_msg_id, const char *message){
    CURL *curl;
    CURLcode res;
    Mem_struct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;
    char *url;

    curl = curl_easy_init();

    if(curl){
        char *escaped_message = curl_easy_escape(curl, message, 0);

        int url_length = snprintf(NULL, 0, "%seditMessageText?chat_id=%lld&message_id=%d&text=%s", Tg_link, *CID, edit_msg_id, escaped_message);
        url = malloc(url_length + 1);
        snprintf(url, url_length + 1,"%seditMessageText?chat_id=%lld&message_id=%d&text=%s", Tg_link, *CID, edit_msg_id, escaped_message);
        curl_free(escaped_message);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);

        if(res != CURLE_OK){
            fprintf(stderr, "Failed: %s\n", curl_easy_strerror(res));
        }
        printf("The id %lld message was edited successfully\n", edit_msg_id);
        printf("server response:\n%s\n", chunk.memory);
        free(chunk.memory);
        free(url);
        curl_easy_cleanup(curl);
    } else{
        fprintf(stderr, "Failed to initialize cURL\n");
    }

}
