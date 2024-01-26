#include <stdio.h>

void ADC_init();

/**/
float getOverloadVoltage();
/**/

uint16_t get_Overcurrent_V();
/**/
void adc2_read();
uint16_t get_BatteryBackup_V(void);

void temperature_sensor_read(float *T_celsius, float *T_fahrenheit);
void temperature_probe_1(float *T_celsius, float *T_fahrenheit);

void temperature_probe_2(float *T_celsius, float *T_fahrenheit);
