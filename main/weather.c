/*
 weather.c - Weather data retrieval from api.openweathermap.org

 This file is part of the ESP32 Everest Run project
 https://github.com/krzychb/esp32-everest-run

 Copyright (c) 2016 Krzysztof Budzynski <krzychb@gazeta.pl>
 This work is licensed under the Apache License, Version 2.0, January 2004
 See the file LICENSE for details.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>
//#include <esp_rmaker_utils.h>
#include "main_driver.h"
#include "weather.h"
#include "http.h"
#include "jsmn.h"
#include "dgus_lcd.h"
#include <unistd.h> // For usleep function

#include "esp_netif.h"


/* Constants that aren't configurable in menuconfig
   Typically only LOCATION_ID may need to be changed
 */
#define WEB_SERVER "api.openweathermap.org"
#define WEB_URL "http://api.openweathermap.org/data/2.5/weather"
// Location ID to get the weather data for
#define LOCATION_ID "Ankleshwar,IN"

// The API key below is configurable in menuconfig
#define OPENWEATHERMAP_API_KEY "f8746efe17f17891636d685af60199b9"
extern bool wifiConnected;

static const char *get_request = "GET " WEB_URL"?q="LOCATION_ID"&appid="OPENWEATHERMAP_API_KEY" HTTP/1.1\n"
    "Host: "WEB_SERVER"\n"
    "Connection: close\n"
    "User-Agent: esp-idf/1.0 esp32\n"
    "\n";

static weather_data weather;
static http_client_data http_client = {0};

TaskHandle_t  http_handle;
const char *TAG_weather="weather";

// Define global variables to store previous weather data
static uint32_t prev_reference_pressure = 0;
static uint32_t prev_reference_temp = 0;
static uint32_t prev_refrance_humi = 0;
static char prev_refrance_city_name[20] = {0};

static int prev_day = -1;
static int prev_month = -1;
static int prev_year = -1;

int getMonthNumber(const char* monthStr) {
    // Lookup table for month conversions
    const char* months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    const int numMonths = sizeof(months) / sizeof(months[0]);

    for (int i = 0; i < numMonths; i++) {
        if (strcmp(monthStr, months[i]) == 0) {
            return i + 1;  // Return 1-based index
        }
    }

    return -1;  // Month not found
}

Timestamp splitTimestamp(const char* timestamp) {
    Timestamp result;
    char *monthStr="";
    // Parse the timestamp string using sscanf
    sscanf(timestamp, "%*s %s %d %d:%d:%d %d %*s %*d",
            monthStr, &result.day, &result.hour, &result.minute, &result.second, &result.year);

    // Convert month string to month number
    result.month = getMonthNumber(monthStr);
    result.year %= 100;
   //  printf("Year: %d , %d, %d\n", result.year, result.month, result.day);
   //      printf("Hour: %d :%d:%d\n", result.hour, result.minute, result.second);

    return result;
}


/* Collect chunks of data received from server
   into complete message and save it in proc_buf
 */
static void process_chunk(uint32_t *args)
{
    http_client_data* client = (http_client_data*)args;

    int proc_buf_new_size = client->proc_buf_size + client->recv_buf_size;
    char *copy_from;

    if (client->proc_buf == NULL){
        client->proc_buf = malloc(proc_buf_new_size);
        copy_from = client->proc_buf;
    } else {
        proc_buf_new_size -= 1; // chunks of data are '\0' terminated
        client->proc_buf = realloc(client->proc_buf, proc_buf_new_size);
        copy_from = client->proc_buf + proc_buf_new_size - client->recv_buf_size;
    }
    if (client->proc_buf == NULL) {
        ESP_LOGE(TAG_weather, "Failed to allocate memory");
    }
    client->proc_buf_size = proc_buf_new_size;
    memcpy(copy_from, client->recv_buf, client->recv_buf_size);
}

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
            strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

static bool process_response_body(const char * body)
{
    /* Using great little JSON parser http://zserge.com/jsmn.html
       find specific weather information:
         - Humidity,
         - Temperature,
         - Pressure
       in HTTP response body that happens to be a JSON string

       Return true if phrasing was successful or false if otherwise
     */

    #define JSON_MAX_TOKENS 100
    jsmn_parser parser;
    jsmntok_t t[JSON_MAX_TOKENS];
    jsmn_init(&parser);
    int r = jsmn_parse(&parser, body, strlen(body), t, JSON_MAX_TOKENS);
    if (r < 0) {
        ESP_LOGE(TAG_weather, "JSON parse error %d", r);
        return false;
    }
    if (r < 1 || t[0].type != JSMN_OBJECT) {
        ESP_LOGE(TAG_weather, "JSMN_OBJECT expected");
        return false;
    } else {
    //    ESP_LOGI(TAG_weather, "Token(s) found %d %s %d", r, body, strlen(body));
        char subbuff[8];
        int str_length;
        for (int i = 1; i < r; i++) {
            if (jsoneq(body, &t[i], "humidity") == 0) {
                str_length = t[i+1].end - t[i+1].start;
                memcpy(subbuff, body + t[i+1].start, str_length);
                subbuff[str_length] = '\0';
                weather.humidity = atoi(subbuff);

                i++;
            } else if (jsoneq(body, &t[i], "temp") == 0) {
                str_length = t[i+1].end - t[i+1].start;
                memcpy(subbuff, body + t[i+1].start, str_length);
                subbuff[str_length] = '\0';
                weather.temperature = atof(subbuff);

                i++;
            } else if (jsoneq(body, &t[i], "pressure") == 0) {
                str_length = t[i+1].end - t[i+1].start;
                memcpy(subbuff, body + t[i+1].start, str_length);
                subbuff[str_length] = '\0';
                weather.pressure = atof(subbuff);
                i++;
            }
            else if (jsoneq(body, &t[i], "name") == 0) {
            str_length = t[i+1].end - t[i+1].start;
            memcpy(subbuff, body + t[i+1].start, str_length);
            subbuff[str_length] = '\0';
            strcpy(weather.city_name,subbuff);
            i++;
            }
        }
        return true;
    }
}


static void disconnected(uint32_t *args)
{
    http_client_data* client = (http_client_data*)args;
    bool weather_data_phrased = false;

    const char * response_body = find_response_body(client->proc_buf);
    if (response_body) {
       // printf("Response Body: %s\n", response_body);

        weather_data_phrased = process_response_body(response_body);
    } else {
        ESP_LOGE(TAG_weather, "No HTTP header found");
    }

    free(client->proc_buf);
    client->proc_buf = NULL;
    client->proc_buf_size = 0;

    // execute callback if data was retrieved
    if (weather_data_phrased) {
        if (weather.data_retreived_cb) {
            weather.data_retreived_cb((uint32_t*) &weather);
        }
    }
    ESP_LOGD(TAG_weather, "Free heap %u", xPortGetFreeHeapSize());
}

static void http_request_task(void *pvParameter) {
    const TickType_t regular_delay =  20 * 1000 / portTICK_PERIOD_MS; 

    // Flag to track whether it's the first iteration
    int first_iteration = 1;

    while (1) {
        if (wifiConnected ) {
            // Assuming that http_client_request sends the weather update request
            http_client_request(&http_client, WEB_SERVER, get_request);

        }
        vTaskDelay(regular_delay);
        
    }
}


void on_weather_data_retrieval(weather_data_callback data_retreived_cb)
{
    weather.data_retreived_cb = data_retreived_cb;
}

void initialise_weather_data_retrieval(unsigned long retreival_period)
{
    weather.retreival_period = retreival_period;

    http_client_on_process_chunk(&http_client, process_chunk);
    http_client_on_disconnected(&http_client, disconnected);
    xTaskCreate(&http_request_task, "http_request_task", 2 * 2048, NULL,configMAX_PRIORITIES-7, &http_handle);
}

void weather_data_retreived(uint32_t *args) {
    char local_time[64];
    weather_data* weather = (weather_data*) args;
    uint32_t reference_pressure, reference_temp, refrance_humi;
    char refrance_city_name[20];
    reference_pressure = weather->pressure;
    reference_temp = weather->temperature / 10;
    refrance_humi = weather->humidity;
    snprintf(refrance_city_name, sizeof(refrance_city_name), "%s  ", weather->city_name);

    // Check if there's a change in weather data
    if (reference_pressure != prev_reference_pressure ||
        reference_temp != prev_reference_temp ||
        refrance_humi != prev_refrance_humi ||
        strcmp(refrance_city_name, prev_refrance_city_name) != 0) {

        // Update only if there's a change in data
        dgus_set_var(0x1008, reference_temp); // Temperature
        vTaskDelay(20);
        dgus_set_var(0x1009, refrance_humi); // Humidity
        vTaskDelay(20);
         dgus_set_var(0x1010, reference_pressure); // Pressure
        vTaskDelay(20);

        // Check if city name has changed and update only if it's different
        if (strcmp(refrance_city_name, prev_refrance_city_name) != 0) {
            // dgus_set_text(0x1100, local_time, strlen(local_time)); // Update city name if needed
            strcpy(prev_refrance_city_name, refrance_city_name);
        }

        // Update previous values with the new values
        prev_reference_pressure = reference_pressure;
        prev_reference_temp = reference_temp;
        prev_refrance_humi = refrance_humi;
        strcpy(prev_refrance_city_name, refrance_city_name);
        }
        esp_rmaker_get_local_time_str(local_time, sizeof(local_time));
        Timestamp ts = splitTimestamp(local_time);
        
           // Check if the date has changed
        if (ts.day != prev_day || ts.month != prev_month || ts.year != prev_year) {
        // Update the LCD with the new date if it has changed
        // Add your code to update the LCD with the new date here
        dgus_set_rtc(ts.hour, ts.minute, ts.second, ts.day, ts.month, ts.year); // Update time
        
        // Update previous date values with the new date
        prev_day = ts.day;
        prev_month = ts.month;
        prev_year = ts.year;
    }
        uint32_t merged_value = (ts.hour << 8) | ts.minute;  // Initialize the 32-bit variable
       ESP_LOGI("weather ", "The current time is: %d.", merged_value);

        dgus_set_var(0x0012, merged_value); // Temperature

#ifdef _DEBUG_ON1
ESP_LOGI("weather", "Reference pressure: %d Pa temprature %d c", reference_pressure, reference_temp);
       ESP_LOGI("weather", "Reference humidity : %d hpa  cityname %s", refrance_humi, refrance_city_name);
       ESP_LOGI("weather ", "The current time is: %s.", local_time);
#endif
    
}
void Weather_Init(void)
{
     ESP_ERROR_CHECK(esp_netif_init());
     initialise_weather_data_retrieval(9000/ portTICK_PERIOD_MS);
     on_weather_data_retrieval(weather_data_retreived);

}
