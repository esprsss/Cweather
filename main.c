#include <stdio.h>
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <sys/resource.h>
struct MemoryStruct {
  char *memory;
  size_t size;
};
static size_t write_cb(char *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if(!ptr) {
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
  mem->memory = ptr;
  memcpy(&mem->memory[mem->size], contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
  return realsize;
}
void load_env(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0;
        if (line[0] == '\0' || line[0] == '#') continue;
        char *delimiter = strchr(line, '=');
        if (delimiter) {
            *delimiter = '\0';
            char *key = line;
            char *val = delimiter + 1;
            setenv(key, val, 1);
        }
    }
    fclose(file);
}
int main() {
load_env(".env");
    const char *api_key = getenv("OPENWEATHER_API_KEY");
    if (!api_key) {
        fprintf(stderr, "openweather api key not found.\n");
        return 1;
    }

  char city[100];
  char url[512];
  scanf("%99s", city);
  snprintf(url, sizeof(url), "https://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=metric", city, api_key ); 
  struct MemoryStruct chunk;
  CURL *curl;
  CURLcode result;
  result = curl_global_init(CURL_GLOBAL_ALL);
  if(result != CURLE_OK)
    return (int)result;

  chunk.memory = malloc(1); 
  chunk.size = 0;
  curl = curl_easy_init();
  if(curl) {
curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    result = curl_easy_perform(curl);
    if(result != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(result));
    }
    else {
      cJSON *json = cJSON_Parse(chunk.memory);
      if (json != NULL) {
          cJSON *mainf = cJSON_GetObjectItemCaseSensitive(json, "main");
              if (mainf != NULL) {
                  cJSON *temp = cJSON_GetObjectItemCaseSensitive(mainf, "temp");
                  cJSON *flike = cJSON_GetObjectItemCaseSensitive(mainf, "feels_like");
                    if (cJSON_IsNumber(temp)) {
                      double fckng_temp = cJSON_GetNumberValue(temp);
                      printf("погода: %.0f°C\n", fckng_temp);
                    }
                    if (cJSON_IsNumber(flike)) {
                      double fcking_fl = cJSON_GetNumberValue(flike);
                      printf("ощущается как: %.0f°C\n", fcking_fl);
          }
        }
         cJSON_Delete(json);
      }
    }
    curl_easy_cleanup(curl);
  }
  free(chunk.memory);
  curl_global_cleanup();
  return 0;
}
