#include "I2C.h"
#include "driver/i2c.h"


/**
 * @brief i2c master initialization
 */
esp_err_t I2C0_init(void)
{
    int i2c_master_port = I2C_NUM_0;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = EXT0_SDA,
        .scl_io_num = EXT0_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
}

// esp_err_t I2C1_init(void)
// {
//     int i2c_port_num = I2C_NUM_1;

//     i2c_config_t i2c1_conf = {
//         .mode = I2C_MODE_MASTER,
//         .sda_io_num = EXT1_SDA,
//         .scl_io_num = EXT1_SCL,
//         .sda_pullup_en = GPIO_PULLUP_ENABLE,
//         .scl_pullup_en = GPIO_PULLUP_ENABLE,
//         .master.clk_speed = I2C1_MASTER_FREQ_HZ,
//     };

//     i2c_param_config(i2c_port_num, &i2c1_conf);
//     return i2c_driver_install(i2c_port_num, i2c1_conf.mode, 0, 0, 0);
// }

esp_err_t i2c_master_Write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data,uint16_t len)
{
    uint8_t write_buf[2] = {reg_addr, *reg_data};
    return i2c_master_write_to_device(I2C0_NUM_0, dev_id, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
}

esp_err_t i2c_master_Read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data,uint16_t len)
{
    return i2c_master_write_read_device(I2C0_NUM_0, dev_id, &reg_addr, 1, reg_data, len, I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
}

// esp_err_t i2c_sensorRead(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data,uint16_t len)
// {
//        return i2c_master_write_read_device(I2C1_NUM_1, dev_id, &reg_addr, 1, reg_data, len, I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
// }

// esp_err_t i2c_sensorWrite(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data,uint16_t len)
// {
//     uint8_t write_buf[2] = {reg_addr, *reg_data};
//     return i2c_master_write_to_device(I2C1_NUM_1, dev_id, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
// }


// esp_err_t platform_write(void *handle, uint8_t reg, uint8_t *bufp,
//                               uint16_t len)
// {
//     return i2c_master_Write(LSM6DSOX_I2C_ADD_H, reg,bufp,len);
// }

// esp_err_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
//                              uint16_t len)
// {
//     return i2c_master_Read(LSM6DSOX_I2C_ADD_H, reg,bufp,len);
// }