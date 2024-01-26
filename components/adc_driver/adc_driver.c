#include "adc_driver.h"

#include "esp_adc_cal.h"
#include "esp_log.h"
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
esp_adc_cal_characteristics_t *adcCharacteristics;
#define NUMBER_OF_SAMPLES 500
#define DEFAULT_VREF 3000


float R1 = 10000; // value of R1 on board 10k
// Constants for the Steinhart-Hart equation
#define C1 0.001129148
#define C2 0.000234125
#define C3 0.0000000876741

const char *TAG_ADC="adc";

static esp_adc_cal_characteristics_t adc1_chars;
static esp_adc_cal_characteristics_t adc2_chars;
#define ADC_EXAMPLE_ATTEN           ADC_ATTEN_DB_11

static bool adc_calibration_init(void)
{
    esp_err_t ret;
    bool cali_enable = false;

    ret = esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP_FIT);
    if (ret == ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGW(TAG_ADC, "Calibration scheme not supported, skip software calibration");
    } else if (ret == ESP_ERR_INVALID_VERSION) {
        ESP_LOGW(TAG_ADC, "eFuse not burnt, skip software calibration");
    } else if (ret == ESP_OK) {
        cali_enable = true;
        esp_adc_cal_characterize(ADC_UNIT_1, ADC_EXAMPLE_ATTEN, ADC_WIDTH_BIT_12, DEFAULT_VREF, &adc1_chars);
        esp_adc_cal_characterize(ADC_UNIT_2, ADC_EXAMPLE_ATTEN, ADC_WIDTH_BIT_12, DEFAULT_VREF, &adc2_chars);
    } else {
        ESP_LOGE(TAG_ADC, "Invalid arg");
    }

    return cali_enable;
}


void ADC_init()
{
    adc_calibration_init();
    // ADC1 config
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_12));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_2, ADC_ATTEN_DB_11)); // max 1.75 //gpio3
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_11)); // max 1.75 //gpio4
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_11)); // max 1.75 //gpio5  //probe1 
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11)); // max 1.75 //gpio6  //probe2
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11)); // max 1.75 //gpio7
    ESP_ERROR_CHECK(adc2_config_channel_atten(ADC2_CHANNEL_6, ADC_EXAMPLE_ATTEN));

}

/*!
 *  @brief  Gets the  voltage_STR in volts
 *  @return the  raw reading of adc  converted to voltage in mV
 */
float getOverloadVoltage() {
    int totalAdcRaw = 0;
    for (int sampleIndex = 0; sampleIndex < NUMBER_OF_SAMPLES; sampleIndex++) {
        int adcRaw = adc1_get_raw(ADC1_CHANNEL_7);
        totalAdcRaw += adcRaw;
        vTaskDelay(pdMS_TO_TICKS(5));  // Optional delay between samples
    }

    int averagedAdcRaw = totalAdcRaw / NUMBER_OF_SAMPLES;

    // Convert averagedAdcRaw to voltage in millivolts (mV)
    float voltageMillivolts = averagedAdcRaw * DEFAULT_VREF / 4095;
   uint32_t  voltage = esp_adc_cal_raw_to_voltage(averagedAdcRaw, &adc1_chars);

    // Print the results
    printf("vpropi oc Voltage Raw: %d  %.2f mV %d\n", averagedAdcRaw, voltageMillivolts,voltage);
    return voltageMillivolts;
}


void adc2_read(void) {
    uint32_t adc_reading = 0;
    int adc2_raw;
    uint32_t voltage;
    // Perform multiple ADC readings and average the results
    for (int i = 0; i < NUMBER_OF_SAMPLES; i++) {
        adc2_get_raw(ADC2_CHANNEL_6, ADC_WIDTH_BIT_12, &adc2_raw);
        vTaskDelay(5 / portTICK_PERIOD_MS);  // Delay between samples

        adc_reading += adc2_raw;
    }
    adc_reading /= NUMBER_OF_SAMPLES;

    voltage = esp_adc_cal_raw_to_voltage(adc_reading, &adc2_chars);
    printf("Raw ADC2 channel Reading: %d  %d\n", adc_reading,voltage);

}

/*!
 *  @brief  Gets the  V_BATT_BACK in volts
 *  @return the  raw reading of adc  converted to voltage
 */
// uint16_t get_BatteryBackup_V(void)
// {
//     uint16_t voltage;
//     int adc2_raw,adc_reading=0;
//       for (int i = 0; i < NO_OF_SAMPLES; i++) 
//       {
//         adc2_get_raw( ADC2_CHANNEL_8, ADC_WIDTH_BIT_12, &adc2_raw);
//         adc_reading += adc2_raw;
//       }
//       adc_reading /= NO_OF_SAMPLES;
//     // //Convert adc_reading to voltage in mV
//      voltage = (DEFAULT_VREF * adc_reading) / 4096; //esp_adc_cal_raw_to_voltage(adc2_raw,adc_chars2);
//     ESP_LOGI(TAG_ADC,"get_BatteryBackup_V Raw: %d\tVoltage: %d mV\n", adc_reading, voltage);   
//     return voltage;
// }


void temperature_sensor_read(float *T_celsius, float *T_fahrenheit)
{
    int rawValue;
    rawValue = adc1_get_raw(ADC1_CHANNEL_3);
    float VSense = (DEFAULT_VREF / 4096.0) * rawValue; // Calculate voltage at the thermistor
    float R2 = R1 / ((DEFAULT_VREF /(float) VSense) - 1);      // Calculate resistance on thermistor
    // Only use the Steinhart-Hart equation if necessary

        // Calculate temperature in Celsius
    *T_celsius = 1.0 / (C1 + (C2 * log(R2)) + C3 * log(R2) * log(R2) * log(R2)) - 273.15;
    *T_fahrenheit = (*T_celsius * 9 / 5) + 32; // Convert Celsius to Fahrenheit

}


void temperature_probe_1(float *T_celsius, float *T_fahrenheit)
{
    int rawValue;
    rawValue = adc1_get_raw(ADC1_CHANNEL_5);
    float VSense = (DEFAULT_VREF / 4096.0) * rawValue; // Calculate voltage at the thermistor
    float R2 = R1 / ((DEFAULT_VREF /(float) VSense) - 1);      // Calculate resistance on thermistor
    // Only use the Steinhart-Hart equation if necessary

        // Calculate temperature in Celsius
    *T_celsius = 1.0 / (C1 + (C2 * log(R2)) + C3 * log(R2) * log(R2) * log(R2)) - 273.15;
    *T_fahrenheit = (*T_celsius * 9 / 5) + 32; // Convert Celsius to Fahrenheit

}



void temperature_probe_2(float *T_celsius, float *T_fahrenheit)
{
  int rawValue;
    rawValue = adc1_get_raw(ADC1_CHANNEL_4);
    float VSense = (DEFAULT_VREF / 4096.0) * rawValue; // Calculate voltage at the thermistor
    float R2 = R1 / ((DEFAULT_VREF /(float) VSense) - 1);      // Calculate resistance on thermistor

    // Only use the Steinhart-Hart equation if necessary
    *T_celsius = 1.0 / (C1 + (C2 * log(R2)) + C3 * log(R2) * log(R2) * log(R2)) - 273.15;
    *T_fahrenheit = (*T_celsius * 9 / 5) + 32; // Convert Celsius to Fahrenheit
}