#include <windows.h>
#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <sys/stat.h>
#include <direct.h>

FILE *log_file;

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
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

char *get_filename_from_url(const char *url, const char *download_path) {
    char *slash = strrchr(url, '/');
    if (slash) {
        char *filename = strdup(slash + 1);
        char *prefixed_filename = malloc(strlen(download_path) + 1 + strlen(filename) + 1);
        strcpy(prefixed_filename, download_path);
        strcat(prefixed_filename, "/");
        strcat(prefixed_filename, filename);
        free(filename);

        if (log_file) {
            fprintf(log_file, "Generated filename from URL: %s\n", prefixed_filename);
        }
        return prefixed_filename;
    }
    return NULL;
}

void DownloadFile(const char *url, const char *download_path) {
    if (log_file) {
        fprintf(log_file, "Starting download from: %s\n", url);
    }

    CURL *curl;
    FILE *fp;
    CURLcode res;
    char *filename = get_filename_from_url(url, download_path);
    struct stat st = {0};

    if (stat(download_path, &st) == -1) {
        if (log_file) {
            fprintf(log_file, "Creating directory: %s\n", download_path);
        }
        _mkdir(download_path);
    }

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        fp = fopen(filename ? filename : "sounds/downloaded_file", "wb");

        if (fp) {
            if (log_file) {
                fprintf(log_file, "Saving to: %s\n", filename);
            }

            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            res = curl_easy_perform(curl);

            if (res != CURLE_OK && log_file) {
                fprintf(log_file, "CURL error: %s\n", curl_easy_strerror(res));
            }

            fclose(fp);
        } else if (log_file) {
            fprintf(log_file, "Failed to open file for writing: %s\n", filename);
        }

        curl_easy_cleanup(curl);
        free(filename);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int log_enabled = 0;
    if (strstr(lpCmdLine, "-l") != NULL)
{
        log_enabled = 1;
}
if (log_enabled)
{
        log_file = fopen("update.log", "w");
        if (log_file == NULL) {
            MessageBox(NULL, L"Error opening log file", L"Error", MB_OK | MB_ICONERROR);
            return 1;
        }
    } else {
        log_file = NULL;
    }

    char *download_url;
    char *download_path;
    if (strstr(lpCmdLine, "system") != NULL) {
        download_url = "http://13.124.115.172/upgrade/links.txt";
        download_path = "program";
    } else {
        download_url = "http://13.124.115.172/upgrade/link.txt";
        download_path = "sounds";
    }

    if (log_file) {
        fprintf(log_file, "Initial Download URL: %s\n", download_url);
        fprintf(log_file, "Initial Download Path: %s\n", download_path);
    }

    struct MemoryStruct chunk;
    chunk.memory = malloc(1);  // will be grown as needed by the realloc above
    chunk.size = 0;

    CURL *curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, download_url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK && log_file) {
            fprintf(log_file, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        if (chunk.size > 0) {
            char *line = strtok(chunk.memory, "\r\n");
            while (line != NULL) {
                DownloadFile(line, download_path);
                line = strtok(NULL, "\r\n");
            }
        }

        curl_easy_cleanup(curl);
        free(chunk.memory);
    }

    if (log_file) {
        fclose(log_file);
    }

    return 0;
}
