#include "driver/i2c.h"
 #include "driver/uart.h"
#include "pin_config.h"
#include "BMH01XX.h"
#include "esp_log.h"
#include "dgus_lcd.h"
    //Define command numbers
#define CMD_READ_WEIGHT 0xAE
#define CMD_READ_ADC 0xAD
#define CMD_READ_TEMP 0xAF
#define CMD_CALIBRATE 0xCA
#define CMD_CALIBRATION_STATUS 0xCB
#define CMD_MODULE_SLEEP 0xC0

const char *TAG_BMH="weight";

// Define a flag to keep track of whether calibration has been done
bool calibrationDone = false;

typedef struct {
    uint8_t header;
    uint8_t command;
    uint8_t reserved1;
    uint8_t reserved2;
    uint8_t endFrame;
} RequestFrame;

typedef struct {
    uint8_t header;
    uint8_t command;
    uint8_t data1;
    uint8_t data2;
    uint8_t endFrame;
} ResponseFrame;

  // Example usage
    uint16_t adcValue;
    uint16_t weightValue;
    uint16_t tempValue;
    uint8_t calStatus;

/*************************************************
Description: Read AD value (test mode)

Return: AD value    
*************************************************/

uint16_t Read_Adc(void) {
    uint8_t Buffer[2];
    uint16_t data = 0;
    int counter = 0;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SLAVE_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd,CMD_READ_ADC, true);  // Send the AD read command
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    vTaskDelay(10 / portTICK_RATE_MS);  // Time required before reading data

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SLAVE_ADDRESS << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &Buffer[counter++], I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &Buffer[counter++], I2C_MASTER_ACK);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    data = (Buffer[0] << 8) | Buffer[1];  // Data = Data_HighByte + Data_LowByte

    return data;
}

// void Write_Zero() {
//     uint8_t data[3] = {0xCA, 0x00, 0x00};
//     i2c_cmd_handle_t cmd = i2c_cmd_link_create();
//     i2c_master_start(cmd);
//     i2c_master_write_byte(cmd, (SLAVE_ADDRESS << 1) | I2C_MASTER_WRITE, true);
//     i2c_master_write(cmd, data, 3, true);
//     i2c_master_stop(cmd);
//     i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
//     i2c_cmd_link_delete(cmd);
// }

/*************************************************
Description: Read Calibration value

Return: Calibration value    

Note: value = 0 : Calibration Fail
      value = 1 : During zero-point calibration
      value = 2 : Zero-point calibration complete
      value = 5 : During maximum-point calibration
      value = 6 : Maximum-point calibration complete
*************************************************/
uint8_t readCalibration() {
    uint8_t value;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SLAVE_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0xCA, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    vTaskDelay(10 / portTICK_RATE_MS);  // Time required before reading data

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SLAVE_ADDRESS << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &value, I2C_MASTER_ACK);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return value;
}

/*************************************************
Description: Read Weight value

Return: Weight value    
*************************************************/
uint16_t Read_Weight() {
    uint8_t Buffer[2], counter = 0;
    uint16_t data;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SLAVE_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0xAE, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    vTaskDelay(10 / portTICK_RATE_MS);  // Time required before reading data

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SLAVE_ADDRESS << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &Buffer[counter++], I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &Buffer[counter++], I2C_MASTER_ACK);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    data = (Buffer[0] << 8) | Buffer[1];  // Data = Data_HighByte + Data_LowByte

    return data;
}

/*************************************************
Description: Calibrate maximum point

Parameter: maximum point calibration
*************************************************/
void Write_Calibration(uint16_t value) {
    uint8_t cal_h = (uint8_t)(value >> 8);     // High Byte of value
    uint8_t cal_l = (uint8_t)(value & 0xFF);   // Low Byte of value
    uint8_t data[4] = {CMD_CALIBRATE, cal_h, cal_l};
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SLAVE_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data, 3, true);      // Send calibration command and data
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
}

#ifdef uart_enable


// Print the calibration status in a human-readable format
void printCalibrationStatus(uint8_t status) {
    switch (status) {
        case 0:
             ESP_LOGI(TAG_BMH,"Calibration Fail\n");
            break;
        case 1:
             ESP_LOGI(TAG_BMH,"During zero-point calibration\n");
            break;
        case 2:
             ESP_LOGI(TAG_BMH,"Zero-point calibration complete\n");
            break;
        case 5:
             ESP_LOGI(TAG_BMH,"During maximum-point calibration\n");
            break;
        case 6:
             ESP_LOGI(TAG_BMH,"Maximum-point calibration complete\n");
            break;
        default:
             ESP_LOGI(TAG_BMH,"Unpredictable\n");
            break;
    }
}


bool sendCommand(uint8_t command, ResponseFrame *response) {
    RequestFrame request = {0xAA, command, 0x00, 0x00, 0x55};

    if (uart_write_bytes(UART_PORT,(const char*)&request, sizeof(request)) < 0) {
        printf("Return -1 if data couldn't be sent");
        return -1; // Return -1 if data couldn't be sent (queue is full)
    }

  
    // Send request
   // uart_write_bytes(UART_PORT, (const char*)&request, sizeof(request));

    // Wait for response
    int bytesRead = uart_read_bytes(UART_PORT, (uint8_t*)response, sizeof(ResponseFrame), portMAX_DELAY);
       //           printf("responce d1 %x d2 %x \n",response->data1,response->data2);
    

    return (bytesRead == sizeof(ResponseFrame) &&
            response->header == 0xAA &&
            response->command == command &&
            response->endFrame == 0x55);
}


bool readADCValue(uint16_t *adcValue) {
    ResponseFrame response;
    if (sendCommand(CMD_READ_ADC, &response)) {
        *adcValue = (response.data1 << 8) | response.data2;
        return true;
    }
    return false;
}

bool readWeightValue(uint16_t *weightValue) {
    ResponseFrame response;
    if (sendCommand(CMD_READ_WEIGHT, &response)) {
        *weightValue = (response.data1 << 8) | response.data2;
        return true;
    }
    return false;
}

bool readTemperatureValue(uint16_t *tempValue) {
    ResponseFrame response;
    if (sendCommand(CMD_READ_TEMP, &response)) {
        *tempValue = (response.data1 << 8) | response.data2;
        return true;
    }
    return false;
}

bool calibrate(uint16_t calWeight) {
    ResponseFrame response;
    RequestFrame request = {0xAA, CMD_CALIBRATE, (uint8_t)(calWeight >> 8), (uint8_t)(calWeight & 0xFF), 0x55};
            printf("weight %d \n",calWeight);

    // Send request
  //  uart_write_bytes(UART_PORT, (const char*)&request, sizeof(request));
 if (uart_write_bytes(UART_PORT,(const char*)&request, sizeof(request)) < 0) {
        printf("Return -1 if data couldn't be sent");
        return -1; // Return -1 if data couldn't be sent (queue is full)
    }

    // Wait for response
    int bytesRead = uart_read_bytes(UART_PORT, (uint8_t*)&response, sizeof(response), portMAX_DELAY);
            printf("responce d1 %x d2 %x \n",response.data1,response.data2);

    if (bytesRead == sizeof(response) &&response.header == 0xAA &&  response.command == CMD_CALIBRATE &&  
        response.endFrame == 0x55 &&response.data1 == 0x00)
    {
        printf("send claibrate data sucess\n");  
           return true;
    }
    else
    return false;
 
}

uint8_t readCalibrationStatus() {
    ResponseFrame response;
    RequestFrame request = {0xAA, CMD_CALIBRATION_STATUS, 0x00 ,0x00, 0x55};

    // Send request
    uart_write_bytes(UART_PORT, (const char*)&request, sizeof(request));

    // Wait for response
    int bytesRead = uart_read_bytes(UART_PORT, (uint8_t*)&response, sizeof(response), portMAX_DELAY);

       // Check response
    if (bytesRead == sizeof(response) && response.header == 0xAA && response.command == CMD_CALIBRATION_STATUS && response.endFrame == 0x55) {
       // printf("Calibration status received.  %x \n",response.data1);

    }
                return response.data1;

}



void weightsensor_init(void)
{

    // Configure UART settings
    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, 41, 42, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
        ESP_ERROR_CHECK(uart_driver_install(UART_PORT, 256, 0, 10, NULL, 0));

}

#endif


void weight_sensor_task(void *pvParameters) {
    #ifdef uart_enable
    // Perform calibration only the first time
   if (!calibrationDone) {
        calibrate(0);
        while (readCalibrationStatus() != 2) {
         ESP_LOGI(TAG_BMH,"Zero-point calibration status: ");
            vTaskDelay(1000 / portTICK_RATE_MS);
        }

         calibrate(MAX);
         while (readCalibrationStatus() != 6) {
             // Calibration logic
                      ESP_LOGI(TAG_BMH,"Maximum point calibration status");

             vTaskDelay(1000 / portTICK_RATE_MS);
        }
        
         calibrationDone = true;
   }
    #else
    // I2C init & scan
    if (I2C0_init() != ESP_OK)
         ESP_LOGI(TAG_BMH, "I2C init failed\n");
    scanI2CDevices();

    // Calibrate zero point
     ESP_LOGI(TAG_BMH,"Calibration status start\n");
    Write_Calibration(0);
    while (readCalibration() != 2) {
         ESP_LOGI(TAG_BMH,"Zero-point calibration status: ");
        printCalibrationStatus(readCalibration());
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    // Calibrate maximum point
    Write_Calibration(MAX);
    while (readCalibration() != 6) {
                 weightValue = Read_Weight();

         ESP_LOGI(TAG_BMH,"Maximum point calibration status:%d ",weightValue);
        printCalibrationStatus(readCalibration());
        vTaskDelay(2000 / portTICK_RATE_MS);
    }
    #endif

    // Main loop
    while (1) {
        #ifdef uart_enable
        // Read ADC and weight values (UART-based)
        if (!readADCValue(&adcValue)) {
         ESP_LOGI(TAG_BMH,"Failed to read ADC value\n");
        } 
        if (!readWeightValue(&weightValue)) {
         ESP_LOGI(TAG_BMH,"Failed to read weight value\n");
        }
        #else
        // Read weight and ADC values (I2C-based)
         weightValue = Read_Weight();
         adcValue = Read_Adc();
        #endif
     //   float weightInKg = (float)weightValue / 1000;
     //    ESP_LOGI(TAG_BMH,"Weight value: %u gm (%.2f kg) ADC value: %u\n", weightValue, weightInKg, adcValue);
                  ESP_LOGI(TAG_BMH," %u gm \n", weightValue);
           dgus_set_var(0x1042,weightValue);  // weight data update to lcd 

        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

