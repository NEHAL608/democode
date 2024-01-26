#include "pin_config.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "dgus_lcd.h"
#include "dgus_util.h"
#include "dgus_control_text.h"
#include "ble_mesh.h"

#include "esp_sleep.h"
#include <stdint.h>
#include <stddef.h>
#include "INA219.h"
#include "weather.h"
#include "esp_sleep.h"
#include "update.h"
#include "adc_driver.h"

//#define DEBUG_ON   1  // Uncomment this line to enable debug logs


#define MAIN_DEVICE      0x01
#define STIRAID_DEVICE   0x02
#define WEIGHT_SCALE     0X03

#define MOTOR_ON           1
#define MOTOR_OFF          0 

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_FREQUENCY          (5000) // Frequency in Hertz. Set frequency at 5 kHz


// Define the maximum allowed battery temperature
#define MAX_BATT_TEMP_WARNING 50.0 // in Celsius
#define MAX_BATT_TEMP_SHUTDOWN 60.0 // in Celsius

// Define temperature increase threshold to trigger fan control
#define TEMP_INCREASE_THRESHOLD 10.0 // in Celsius
// Define maximum fan speed
#define MAX_FAN_SPEED 10
#define MAX_STEP_RECIPE 30

/*
 *stiraid Mode
 *This enumeration describes the current select mode
 */
typedef enum ModeState
{
    /*! Auto Mode  */
    AUTO_MODE = 1, 
    /*! Manual mode */
    MANUAL_MODE,  
    /*! Recipes mode */
    RECIPES_MODE,
    /*! record recipes mode */
    RECORD_MODE,
    /*! BLENDING MAIN DEVICE MODE */
    BLENDING_MODE,
    /*! WHISK MAIN DEVICE MODE */
    WHISKHING_MODE,
    /*! FROTHING MAIN DEVICE MODE */
    FROTHING_MODE,
    /*! MIXING_MODE MAIN DEVICE MODE */
    MIXING_MODE,
    /*! CHOPING MAIN DEVICE MODE */
    CHOPPING_MODE,
    WEIGHT_SCALE_MODE=11,
    NUTIRTION_MODE=13,
    TEMPRATUREPROBE_MODE = 14,

} ModeState;

// Define an enumerated type (enum) named "AlertStatus"
typedef enum AlertStatus
{
    CLEAR , 
    SENSOR_READ_ERROR,
    TEMPERATURE_RISE, 
    TEMPERATURE_LOW,
    OVERLOAD,
    BATTERY_LOW,
    BACKLIGHT_OFF ,
    POWER_WAKEUP ,
    POWER_SHUTDOWN ,
} AlertStatus;

// Declare an external variable of type AlertStatus with the name "alertStatus"
extern AlertStatus alertStatus;



// Define the Device structure
struct DeviceStatus {
    uint8_t loadDetect;  // Default load detect is zero
    int32_t motorSpeed;
    ModeState operationMode;  // Default auto mode
    uint8_t powerSaving;    // Default power-saving state is on
    bool motorStatus;
    bool playPauseState;        // Default play/pause state is off
    uint8_t minute; //default minute is 30
    uint8_t hour; //default minute is 30
    uint8_t cureentstatus;
    uint8_t intervalMin;
    uint8_t intervalSec;
    int temperatures;

};
extern struct DeviceStatus myDevice;

void initializeDriver(void);
void file_server_init(void);
void checksdcard(void);
void setMotorSpeed(int32_t desiredSpeed);
void listFiles(const char* path);
esp_err_t example_start_file_server(const char *base_path);
void PlayCallback(bool state,void *ptr);
void ModeCallback(void *ptr);