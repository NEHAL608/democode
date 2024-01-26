/*
 * I2C_Driver.h
 *  Created on: 05-May-2021
 *      Author: acer
 */

#ifndef I2C_DRIVER_H_
#define I2C_DRIVER_H_

#include <stdint.h>
#include "pin_config.h"

#include <esp_err.h>

void delay_ms(uint32_t ms);
esp_err_t I2C0_init(void);
esp_err_t I2C1_init(void);

esp_err_t i2c_master_Read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);
esp_err_t i2c_master_Write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);

/*ir sensor*/
esp_err_t i2c_sensorRead(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);
esp_err_t i2c_sensorWrite(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);


#endif /* I2C_DRIVER_H_ */
