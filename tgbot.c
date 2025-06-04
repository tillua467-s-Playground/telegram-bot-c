#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cjson/cJSON.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

#define BOT_API "NULL"
#define Tg_link "https://api.telegram.org/bot" BOT_API "/"


typedef struct Mem_struct {
    char *memory;
    size_t size;
}Mem_struct;

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
            snprintf(url, url_length, "%ssendMessage?chat_id=%lld&text=%s&reply_to_message=%lld", Tg_link, *CID, escaped_message, reply_id);
            curl_free(escaped_message);
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);

        if(res != CURLE_OK){
            fprintf(stderr, "Failed: %s\n", curl_easy_strerror(res));
        }
        printf("Measage was sent to %lld successfully", CID);
        printf("server response:\n%s\n", chunk.memory);

        curl_easy_cleanup(curl);
        free(chunk.memory);
        free(url);
    } else{
        fprintf(stderr, "Failed to initialize cURL\n");
    }
}