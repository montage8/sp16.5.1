#include <windows.h>
#include <stdio.h>
#include <curl/curl.h>
#include <string.h>

struct MemoryStruct {
    char *memory;
    size_t size;
};

__declspec(dllexport) size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        // out of memory!
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

__declspec(dllexport) char *get_filename_from_url(const char *url) {
    char *slash = strrchr(url, '/');
    if (slash) {
        char *filename = strdup(slash + 1);
        char *prefixed_filename = malloc(strlen("sounds/") + strlen(filename) + 1);
        strcpy(prefixed_filename, "sounds/");
        strcat(prefixed_filename, filename);
        free(filename);
        return prefixed_filename;
    }
    return NULL;
}

__declspec(dllexport) void DownloadFile(const char *url) {
    CURL *curl;
    FILE *fp;
    CURLcode res;
    char *filename = get_filename_from_url(url);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        fp = fopen(filename ? filename : "sounds/downloaded_file", "wb");
        if (fp) {
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                fprintf(stderr, "CURL error: %s\n", curl_easy_strerror(res));
            }
            fclose(fp);
        }
        curl_easy_cleanup(curl);
        free(filename);
    }
}

// main 함수 제거
