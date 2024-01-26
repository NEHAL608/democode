/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

#include <iot_button.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>

#include <app_reset.h>
#include <ws2812_led.h>
#include "app_priv.h"
#include "main_driver.h"
#include "driver/gpio.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_rmaker_core.h"
#include "esp_rmaker_standard_params.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "app_wifi.h"

/* This is the button that is used for toggling the power */
#define BUTTON_GPIO          CONFIG_EXAMPLE_BOARD_BUTTON_GPIO
#define BUTTON_ACTIVE_LEVEL  0
/* This is the GPIO on which the power will be set */
#define OUTPUT_GPIO    48
/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO 3

static uint8_t s_led_state = 0;
/* These values correspoind to H,S,V = 120,100,10 */
#define DEFAULT_RED     0
#define DEFAULT_GREEN   25
#define DEFAULT_BLUE    0

#define WIFI_RESET_BUTTON_TIMEOUT       3
#define FACTORY_RESET_BUTTON_TIMEOUT    10

const char *TAG_app="app_driver";
static bool g_power_state = DEFAULT_SWITCH_POWER;
static float g_temperature = DEFAULT_TEMPERATURE;
static TimerHandle_t sensor_timer;

extern DGUS_obj currentModeControl;

static void app_sensor_update(TimerHandle_t handle)
{
    static float delta = 0.5;
    g_temperature += delta;
    if (g_temperature > 99) {
        delta = -0.5;
    } else if (g_temperature < 1) {
        delta = 0.5;
    }
    esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_type(temp_sensor_device, ESP_RMAKER_PARAM_TEMPERATURE),
                esp_rmaker_float(g_temperature));
}


void rainamker_powerbutton_update(bool value)
{
        esp_rmaker_param_update_and_report(esp_rmaker_device_get_param_by_type(switch_device, ESP_RMAKER_PARAM_POWER),
         esp_rmaker_bool(value));
}

void rainamker_modebutton_update(bool value)
{
    esp_rmaker_param_update_and_report(esp_rmaker_device_get_param_by_type(Mode_device, ESP_RMAKER_PARAM_SPEED),
    esp_rmaker_int(value));
}


void rainamker_speed_update(uint8_t value)
{
    esp_rmaker_param_update_and_report(esp_rmaker_device_get_param_by_type(fan_device, ESP_RMAKER_PARAM_SPEED),
    esp_rmaker_int(value));
}

float app_get_current_temperature()
{
    return g_temperature;
}

esp_err_t app_sensor_init(void)
{
    g_temperature = DEFAULT_TEMPERATURE;
    sensor_timer = xTimerCreate("app_sensor_update_tm", (REPORTING_PERIOD * 1000) / portTICK_PERIOD_MS,
                            pdTRUE, NULL, app_sensor_update);
    if (sensor_timer) {
        xTimerStart(sensor_timer, 0);
        return ESP_OK;
    }
    return ESP_FAIL;
}

static void app_indicator_set(bool state)
{
    if (state) {
        ws2812_led_set_rgb(DEFAULT_RED, DEFAULT_GREEN, DEFAULT_BLUE);
    } else {
        ws2812_led_clear();
    }
}

static void app_indicator_init(void)
{
    ws2812_led_init();
   // app_indicator_set(g_power_state);
}
/*
static void push_btn_cb(void *arg)
{
    bool new_state = !g_power_state;
    app_driver_set_state(new_state);
    esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_type(switch_device, ESP_RMAKER_PARAM_POWER),
                esp_rmaker_bool(new_state));
}*/

static void set_power_state(bool target)
{
    gpio_set_level(3, target);

    app_indicator_set(target);
}



static void configure_gpio(void)
{
    gpio_reset_pin(V_LCM_CNTR);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(V_LCM_CNTR, GPIO_MODE_OUTPUT);
    gpio_set_level(V_LCM_CNTR, 1);

}


void app_driver_init()
{
   esp_sleep_enable_ext0_wakeup(POWER_BUTTON, 0);
  app_sensor_init();
   initializeDriver();
   //updateFile();
    configure_gpio();

}

int IRAM_ATTR app_driver_set_state(bool state)
{
    char *ptr="rainamker power";
     dgus_set_var(0x1023,state);

    PlayCallback(state,ptr);
    return ESP_OK;
}


int IRAM_ATTR app_motor_set_state(uint8_t state)
{
    if(myDevice.operationMode == MANUAL_MODE || myDevice.operationMode == RECORD_MODE||myDevice.operationMode == MAIN_DEVICE){ 
        myDevice.motorSpeed=state;
        dgus_set_var(0x1068,  myDevice.motorSpeed);

    }
  return ESP_OK;
}

int IRAM_ATTR app_mode_set_state(uint8_t state)
{
 char *ptr="rainamker mode";
 currentModeControl.value =state;
 dgus_set_var(0x1022,currentModeControl.value);
 ModeCallback(ptr) ;

  return ESP_OK;
}

bool app_driver_get_state(void)
{
    return g_power_state;
}



