/**
 * @file dgus_lcd.c
 * @author Barry Carter
 * @date 01 Jan 2021
 * @brief DGUS II LCD Driver Utility functions
 */

#include "dgus_lcd.h"
#include <stdint.h>
#include <stddef.h>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

const char *TAG_dgus="lcd";
uint8_t i;
SemaphoreHandle_t command_sync,string_sync; 
QueueHandle_t uart_queue,recvRetStringQueue; 

extern DGUS_obj *D_list[],*string_list[];
#define RX_BUFFER_SIZE 256
uint8_t Rxbuffer[RX_BUFFER_SIZE]={0},rx_data_size;

bool nextion_core_event_process(dgus_handle_t handle);
void uart_event_task(void *pvParameters);

void delay_ms(uint32_t ms)
{
    // Return control or wait, for a period amount of milliseconds
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

uint16_t Calculate_CRC16(uint8_t *updata, uint32_t len, uint8_t mode) {
    uint16_t Reg_CRC = 0xffff;
    uint32_t i, j;
    // Check if len is 0
    if (len == 0) {
        return 0; // or another appropriate value
    }

 
    for (i = 0; i < len; i++) {
        Reg_CRC ^= *updata++;

        for (j = 0; j < 8; j++) {
            if (Reg_CRC & 0x0001) {
                Reg_CRC = Reg_CRC >> 1 ^ 0XA001;
            } else {
                Reg_CRC >>= 1;
            }
        }
    }
    // Append CRC to the data if mode is set to 1
    if (mode == 1) {
        *updata++ = (uint8_t)Reg_CRC;
        *updata = (uint8_t)(Reg_CRC >> 8);
    }
  return Reg_CRC;
}

int sendcrc(uint8_t *buf, uint8_t len) {
    // Calculate CRC16 checksum
   printf("sendcrc is %d \n", buf[2] );

    Calculate_CRC16(&buf[3], buf[2] - 2, 1);

    // Send data via serial communication
    if (serial_send_data(buf, len + 2) == -1) {
        printf("Queue is full\n");
        return -1; //Queue is full
    }
    return 0;
}

// Send data via serial communication
int serial_send_data(uint8_t *data, size_t len) {

    #ifdef _DEBUG_ON
    for (size_t i = 0; i < len; i++) {
        printf("%X ", data[i]);
    }
    printf("\n");
  #endif
    if (uart_write_bytes(PORT, data, len) < 0) {
        return -1;
    }

    return 0;
}

int32_t nextion_core_uart_read_as_byte(uint8_t *buffer, size_t length)
{
    uint8_t *movable_buffer = buffer;
    int ends_found = 0; // Some commands use NEX_DVC_CMD_END_LENGTH chars as end response.
    int bytes_read = 0;
    int result = 0;
  
        for (size_t i = 0; i < length; i++)
        {
            
              result = uart_read_bytes(PORT, movable_buffer, 1, pdMS_TO_TICKS(300));
               //     ESP_LOGI(TAG_dgus," %x",buffer[i]);

               if (result > 0) // We got something.
            {
                if (buffer[i] == 0XEF)
                {
                    ends_found++;
                }
               if (buffer[2] == 0X0A && buffer[3]==0X83)
                {
                    ESP_LOGI(TAG_dgus," TOUCH DATA RECIVE");
                    length=11;
                }
               
                movable_buffer++;
                bytes_read++;
                if (ends_found == 1){
                    break;
                }

                continue;
            }
            if (result < 1) // Error or timeout.
            {
                if (result == 0) // It's a timeout.
                {
                    if (bytes_read > 0)
                    {
                        break;
                    }
                        #ifdef _DEBUG_ON

                        ESP_LOGI(TAG_dgus,"response timed out");
                        #endif
                }

                // For error or timeout, always return -1;
                bytes_read = -1;
                break;
            }
        }
   
        return bytes_read;
    }


void Touch_attach_cb(DGUS_obj *touch, TouchEventCb cb_function,void *ptr)
{
    touch->__cb_func= cb_function;
    touch->__cb_ptr="main";
}

void CreateDGUSObject(DGUS_obj *obj, uint32_t addr, uint16_t data)
{
    obj->addr=addr;
    obj->value=data;
}

void CreateDGUSObject_assign_string(DGUS_obj *obj, char* buffer) {

    // Allocate memory for the string
    obj->stringValue = (char *)malloc(strlen(buffer) + 1); // +1 for null terminator

    if (obj->stringValue != NULL) {
        // Copy the contents of the string to the allocated memory
        strcpy(obj->stringValue, buffer);
    } else {
        // Handle memory allocation failure
        fprintf(stderr, "Memory allocation failed for stringValue\n");
    }
}
/**
 * @brief  A packet header that every packet needs 
 * len incudes data + 1 (for cmd byte)
 * 
 * @return typedef struct 
 */
typedef struct __attribute__((packed)) dgus_packet_header_t {
  uint8_t header0;
  uint8_t header1;
  uint8_t len;
  uint8_t cmd;
} dgus_packet_header; /**< packet header structure */

struct __attribute__((packed)) dgus_packet {
  dgus_packet_header header;
  union Data {
    uint8_t cdata[SEND_BUFFER_SIZE];
    uint16_t sdata[SEND_BUFFER_SIZE/2];
    uint32_t ldata[SEND_BUFFER_SIZE/4];
  } data;
  uint8_t len;
};

typedef struct __attribute__((packed)) dgus_var_data_t {
  uint16_t address;
  uint16_t data[16];
} dgus_var_data; /**< Header for a VAR command */



static void _prepare_header(dgus_packet_header *header, uint16_t cmd, uint8_t len) {
  header->header0 = HEADER0;
  header->header1 = HEADER1;
  header->len =len + 3;
  header->cmd = cmd;
 // printf("_prepare_header %d ", header->len);
}

DGUS_RETURN send_data(enum command cmd, dgus_packet *p) {
    DGUS_RETURN r=DGUS_OK;

  //  _prepare_header(&p->header, cmd, p->len);
    p->header.header0 = HEADER0;
    p->header.header1 = HEADER1;
    p->header.len = p->len + 3;
    p->header.cmd = cmd;
   //   printf(" %d ", p->header.len);
//  for (int i = 0; i < p->len; ++i) {
//         printf("  %d: 0x%02x\n", i, p->data.cdata[i]);
//     }
    if (xSemaphoreTake(string_sync, portMAX_DELAY) == pdTRUE) {
        sendcrc((uint8_t *)p, sizeof(p->header) + p->len);
        r=recvRetCommandFinished();
        xSemaphoreGive(string_sync);
    }
    return r;
}



int text_sendcrc(uint8_t *buf, uint8_t len) {
    // Calculate CRC16 checksum
    Calculate_CRC16(&buf[3], buf[2] - 2, 1);

    // Send data via serial communication
    if (serial_send_data(buf, buf[2]+3) == -1) {
        printf("Queue is full\n");
        return -1; //Queue is full
    }
    return 0;
}
DGUS_RETURN send_text_data(enum command cmd, dgus_packet *p) {
    DGUS_RETURN r=DGUS_OK;

    p->header.header0 = HEADER0;
    p->header.header1 = HEADER1;
    p->header.len = p->len + 5;
    p->header.cmd = cmd;

    // for (int i = 0; i < p->len; ++i) {
    //    printf("  %d: 0x%02x\n", i, p->data.cdata[i]);
    // }

       text_sendcrc((uint8_t *)p, sizeof(p->header) + p->len);

   
        if (xSemaphoreTake(string_sync, portMAX_DELAY) == pdTRUE) {

        r=recvRetCommandFinished();
        xSemaphoreGive(string_sync);
    }
    return r;
}

DGUS_RETURN get_data(enum command cmd, dgus_packet *p,uint16_t *buf) {
    DGUS_RETURN r=DGUS_OK;
    _prepare_header(&p->header, cmd, p->len);
   // if (xSemaphoreTake(string_sync, portMAX_DELAY) == pdTRUE) {
        sendcrc((uint8_t *)p, sizeof(p->header) + p->len);
    //    r= recvRetNumber(buf);
     //   xSemaphoreGive(command_sync);
  //  }
return r;
}

DGUS_RETURN get_string(enum command cmd, dgus_packet *p,uint16_t *buf) {
    DGUS_RETURN r=DGUS_OK;

    _prepare_header(&p->header, cmd, p->len);
        sendcrc((uint8_t *)p, sizeof(p->header) + p->len);
       if (xSemaphoreTake(string_sync, portMAX_DELAY) == pdTRUE) {

         r=recvRetString(buf);
        // if (r)        {
        //     r=recvRetString(buf);
        // }
         xSemaphoreGive(string_sync);
       }
return r;

}

/* tail n 8 bit variable to the output buffer */
void buffer_u8(dgus_packet *p, uint8_t *data, size_t len) {
  memcpy(&p->data.cdata[p->len], data, len);
  p->len += len;
}

/* tail n 16 bit variables to the output buffer */
void buffer_u16(dgus_packet *p, uint16_t *data, size_t len) {
  for(i=0; i < len; i++) {
    uint16_t pt = data[i];
    uint16_t pn = SWP16(pt);
    memcpy(&p->data.cdata[p->len], &pn, 2);
    p->len +=2;
  }
}

/* tail n 32 bit variables to the output buffer */
void buffer_u32(dgus_packet *p, uint32_t *data, size_t len) {
  for(i=0; i < len; i++) {
    uint32_t t = SWP32(data[i]);
    memcpy(&p->data.cdata[p->len], &t, 4);
    p->len += 4;
  }
}

/* tail a 32 bit variable to the output buffer */
void buffer_u32_1(dgus_packet *p, uint32_t data) {
  if (data < 0xFFFF) {
    buffer_u16(p, (uint16_t *)&data, 1);
  }
  else {
    uint32_t t = SWP32(data);
    memcpy(&p->data.cdata[p->len], &t, 4);
    p->len += 4;
  }
}

/* re-init the packet buffer */
dgus_packet *dgus_packet_init() {
  static dgus_packet d;
  memset(&d, 0, sizeof(d));
  d.len = 0;
  return &d;
}

void dgus_packet_set_data(dgus_packet *p, uint8_t offset, uint8_t *data, uint8_t len) {
    memcpy(&p->data.cdata[offset], data, len);
}

DGUS_RETURN dgus_request_var(uint16_t addr, uint8_t len) {
  dgus_packet *d = dgus_packet_init();
  buffer_u16(d, &addr, 1);
  buffer_u8(d, &len, 1);
  send_data(DGUS_CMD_VAR_R, d);

  // async wait for reply
  return DGUS_OK;
}

/* Sync read n 16 bit variables from the VAR register at addr */
/*DGUS_RETURN dgus_get_var8(uint16_t addr, uint8_t *buf, uint8_t len) {

  dgus_packet *d = dgus_packet_init();
  buffer_u16(d, &addr, 1);
  buffer_u8(d, &len, 1);
  DGUS_RETURN r= get_data(DGUS_CMD_VAR_R, d,buf);
  printf("%d",*buf);
   if (r != DGUS_OK) return r;
  //got a packet
//   memcpy(buf, recvdata, len);
//  for (i=0; i < len; i+=2) {
//    uint16_t *bp = (uint16_t *)(&buf[i]);
//    *bp = SWP16(*bp);
//  }
  
 return DGUS_OK;
}*/

DGUS_RETURN dgus_get_var(uint16_t addr, uint16_t *buf, uint8_t len) {
  DGUS_RETURN r=DGUS_OK;

  dgus_packet *d = dgus_packet_init();
  buffer_u16(d, &addr, 1);
  buffer_u8(d, &len, 1);
  r= get_data(DGUS_CMD_VAR_R, d,buf);
  return r;
}


/* Set a variable in the VAR buffer
 * data width is 16 bit
 */
DGUS_RETURN dgus_set_var(uint16_t addr, uint32_t data)
{
      DGUS_RETURN r=DGUS_OK;
      dgus_packet *d = dgus_packet_init();
      buffer_u16(d, &addr, 1);
      buffer_u32_1(d, (data));
      r=send_data(DGUS_CMD_VAR_W, d);
  return r;
}


DGUS_RETURN dgus_set_val(DGUS_obj  *obj ,uint32_t data) 
{
  uint16_t addres=obj->addr;
  DGUS_RETURN r=DGUS_OK;
  obj->value=data;
  dgus_packet *d = dgus_packet_init();
  buffer_u16(d, &addres, 1);
  buffer_u32_1(d, (data));
  r=send_data(DGUS_CMD_VAR_W, d);
  return r;
}

DGUS_RETURN dgus_set_var8(uint16_t addr, uint8_t *data, uint8_t len) 
{
      DGUS_RETURN r=DGUS_OK;
      dgus_packet *d = dgus_packet_init();
      buffer_u16(d, &addr, 1);
      buffer_u8(d, data, len);

      r=send_data(DGUS_CMD_VAR_W, d);
  return r;
}

/* internal */
void dgus_packet_set_len(dgus_packet *p, uint16_t len) {
  p->len = len;
}

/*
 * Receive uint32_t data.
 *
 * @retval 1 - error.
 * @retval 0 - success.
 *
 */
DGUS_RETURN recvRetNumber(uint16_t *number)
{
    uint8_t response[11];

    if (!number)
      return DGUS_ERROR;

    nextion_core_uart_read_as_byte(response,11);
    #ifdef _DEBUG_ON
    for (size_t i = 0; i < 11; i++) {
        ESP_LOGI(TAG_dgus, "Byte %d: 0x%02X", i, response[i]);
    }
    #else
    delay_ms(100);

    #endif
     if (response[0] == HEADER0 && response[1] == HEADER1  && response[3] == 0x83) {
        *number = (uint16_t)response[8];
        ESP_LOGI(TAG_dgus, "recive data: %d", *number);
    }
    else {
        ESP_LOGI(TAG_dgus, "Command failed");
        uart_flush_input(PORT);

        return DGUS_ERROR;
    }
  return DGUS_OK;
}
/*
 * Command is executed successfully.
 *
 * @retval 1 - success.
 * @retval 0 - failed.
 *
 */
DGUS_RETURN recvRetCommandFinished()
{
    const uint8_t expectedResponse[8] = {HEADER0, HEADER1, 0x05, 0x82, 0x4F, 0x4B, 0xA5, 0xEF}; // Modify as needed
    uint8_t response[sizeof(expectedResponse)]; // Use the size of expectedResponse for the response buffer

    nextion_core_uart_read_as_byte(response, sizeof(expectedResponse));

    #ifdef _DEBUG_ON
    for (size_t i = 0; i < sizeof(expectedResponse); i++) {
        printf("0x%02X  ", response[i]);
    }
    #else
    delay_ms(50);
    #endif

    // Check if the received response matches the expected response
    for (size_t i = 0; i < sizeof(expectedResponse); i++) {
        if (response[i] != expectedResponse[i]) {
         //   #ifdef _DEBUG_ON
            ESP_LOGI(TAG_dgus, "Command failed");
          //  #endif
            uart_flush_input(PORT);
            return DGUS_ERROR; // Exit early if a mismatch is found
        }
    }

    return DGUS_OK;
}

DGUS_RETURN recvRetString(uint16_t *recstr) {
    uint8_t header[] = {0x5A, 0xA5}; // The first 2 bytes of your frame
    uint16_t data_length;
    uint8_t buffer[256]; // Adjust the buffer size as necessary

    // Read header bytes from UART
    for (size_t i = 0; i < sizeof(header); i++) {
        if (uart_read_bytes(PORT, &buffer[i], 1, pdMS_TO_TICKS(300)) <= 0) {
            ESP_LOGW(TAG_dgus, "recvRetString failed to read header byte %d", i);
            return -1;
        }
    }

    // Check header bytes
    if (memcmp(buffer, header, sizeof(header)) != 0) {
        ESP_LOGW(TAG_dgus, "Header mismatch");
        return -1;
    }

    // Read the third byte separately
    for (size_t i = 2; i <7 ; i++) {
        if (uart_read_bytes(PORT, &buffer[i], 1, pdMS_TO_TICKS(300)) <= 0) {
            ESP_LOGW(TAG_dgus, "recvRetString failed to read header byte %d", i);
            return -1;
        }
    }

    data_length = buffer[8];
             printf("0x%02X  ", buffer[2]);

    // Read the rest of the frame (length - header length - third byte)
    for (size_t i = 7; i < data_length + 2; i++) {
        if (uart_read_bytes(PORT, &buffer[i], 1, pdMS_TO_TICKS(300)) <= 0) {
            ESP_LOGW(TAG_dgus, "Failed to read byte %d", i);
            return -1;
        }
       //  printf("0x%02X  ", buffer[i]);
    }

    // Copy buffer data into recstr
    uint16_t recstr_len = data_length - 6;
 
    // Convert buffer bytes into uint16_t values and store in recstr
    for (int i = 0; i < recstr_len / 2; i++) {
        recstr[i] = (buffer[7 + 2 * i] << 8) | buffer[8 + 2 * i];
    }


    return DGUS_OK;
}


/**
 * Listen touch event and calling callbacks attached before.
 * @param D_listen_list - index to Dgus Components list.
 * @return none.
 * */
void Touch_Event(DGUS_obj *D_list[], uint8_t *buffer, uint16_t len) {
    if (D_list == NULL || buffer[3] != 0x83) {
        return;  // Invalid input, exit early
    }
   
    uint16_t address = (buffer[4] << 8) | buffer[5];
    uint8_t value = buffer[8];
        ESP_LOGI(TAG_dgus, "Address: 0x%x, Value: %d", address,value);

   #ifdef _DEBUG_ON
    // Print the contents of the buffer
    for (uint16_t i = 0; i < len; i++) {
        ESP_LOGI(TAG_dgus, "Byte %d: 0x%02X", i, buffer[i]);
    }
    #else
   delay_ms(10);
    #endif

    for (uint8_t j = 0; D_list[j] != 0; j++) {
        if (D_list[j]->addr == address) {
            D_list[j]->value = value;
            if (D_list[j]->__cb_func) {
                D_list[j]->__cb_func(D_list[j]->__cb_ptr);
            }
            break;  // Exit loop once matching address is found
        }
    }
}

void touch_string_event(DGUS_obj *D_list[], uint8_t *buffer, uint16_t len) {
    if (D_list == NULL || buffer[3] != 0x83) {
        return;  // Invalid input, exit early
    }
   
    uint16_t address = (buffer[4] << 8) | buffer[5];
    uint8_t value = buffer[8];

    char *subString = malloc(value + 1); // Allocate memory for the substring (+1 for null terminator)
    if (subString != NULL) {
        memcpy(subString, &buffer[9], value); // Copy the substring from buffer
        subString[value] = '\0'; // Null-terminate the string
    }
            ESP_LOGI(TAG_dgus, "Address: 0x%x, Value: %s", address,subString );

    // #ifdef _DEBUG_ON

    //   for (uint16_t i = 0; i < len; i++) {
    //      ESP_LOGI(TAG_dgus"Byte %d: 0x%02X", i, buffer[i]);
    //     }
    // //   ESP_LOGI (TAG_dgus, "touch string   %s",  subString);

    // #endif

    for (uint8_t j = 0; D_list[j] != 0; j++) {
        if (D_list[j]->addr == address) {
            // Store the received string into stringValue field of DGUS_obj
            if (D_list[j]->stringValue != NULL) {
                free(D_list[j]->stringValue); // Free previously allocated string if any
            }
            D_list[j]->stringValue = subString;
            D_list[j]->value = value; // Update value field with the substring length

            if (D_list[j]->__cb_func) {
                D_list[j]->__cb_func(D_list[j]->__cb_ptr);
            }
            break;  // Exit loop once matching address is found
        }
    }
}


DGUS_RETURN dgus_set_buffer(uint16_t addr,uint8_t data, uint8_t len)
{
 uint32_t r = SWP32(data);
 return dgus_set_var8(addr, (uint8_t *)&r, sizeof(r));
}


/**
     * @struct nextion_t
     * @brief Holds control data for a context.
     */
    struct dgus_t
    {
        QueueHandle_t uart_queue;                                                     /*!< Queue used for UART event. */
        TaskHandle_t uart_task;                                                       /*!< Task used for UART queue handling. */
        size_t transparent_data_mode_size;                                            /*!< How many bytes are expected to be written while in "Transparent Data Mode". */
        uart_port_t uart_num;                                                         /*!< UART port number. */
    };

dgus_handle_t dgus_driver_install(uart_port_t uart_num, uint32_t baud_rate) {
    ESP_LOGI(TAG_dgus, "Installing driver on UART %d with baud rate %d", uart_num, baud_rate);

    // Configure UART settings
    const uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_num, 1, 2, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // Allocate memory for the driver structure
    dgus_t *driver = (dgus_t *)calloc(1, sizeof(dgus_t));
    driver->uart_num = uart_num;

    // Create semaphores
   command_sync = xSemaphoreCreateMutex();
   string_sync = xSemaphoreCreateBinary();

  // Initialize the queue
    uart_queue = xQueueCreate(20, sizeof(uart_event_t));
    if (uart_queue == NULL) {
        abort();
    }
    // Install UART driver
    ESP_ERROR_CHECK(uart_driver_install(uart_num, 256, 0, 10, &driver->uart_queue, 0));

    if (xTaskCreate(uart_event_task, "uart_event_task", 8192, (void *)driver, configMAX_PRIORITIES -2, &driver->uart_task) != pdPASS) {
        ESP_LOGE(TAG_dgus, "Failed creating UART event task");
        // Log the reason or specific error code if available
        abort();
    }

       
  //  recvRetStringQueue = xQueueCreate(20, sizeof(uint16_t[255]));

    xSemaphoreGive(command_sync);
        xSemaphoreGive(string_sync);

  return driver;
}

void uart_event_task(void *pvParameters)
{
      dgus_handle_t handle = (dgus_handle_t)pvParameters;
      QueueHandle_t queue = handle->uart_queue;
      uart_port_t uart = handle->uart_num;
      uart_event_t event;
    
      for (;;)
      {
          if (xQueueReceive(queue, (void *)&event, portMAX_DELAY))
          {      
         //       ESP_LOGE("uart_event_task", " %d",event.size);
              switch (event.type)
              {
              case UART_DATA:
          
                if (event.size >=11)
                {
                      if (xSemaphoreTake(string_sync, portMAX_DELAY) == pdTRUE)
                        {

                            if (event.size == 11)
                            {
                            
                                int read_bytes = nextion_core_uart_read_as_byte(Rxbuffer, 11);
                                ESP_LOGE("touch event", " %d",read_bytes);

                                    if (read_bytes == 8) {
                                        read_bytes = nextion_core_uart_read_as_byte(Rxbuffer, event.size);
                                    }

                                xSemaphoreGive(string_sync);

                                if (read_bytes == 11) {
                                    Touch_Event(D_list, Rxbuffer, 11); // Handle the touch event with received data
                                }
                            } 
                     else{

                         int read_bytes = nextion_core_uart_read_as_byte(Rxbuffer, event.size);
                         ESP_LOGE("string", " %d",read_bytes);
                        if (read_bytes == 11) {
                               Touch_Event(D_list, Rxbuffer, 11); // Handle the touch event with received data
                             //  break;
                             }
                        if (read_bytes != event.size || read_bytes<0) {
                            read_bytes = nextion_core_uart_read_as_byte(Rxbuffer, event.size);          
                            }                     
                         if(read_bytes!=8)
                                 touch_string_event(string_list, Rxbuffer,  event.size); // Handle the touch event with received data

                    }
                   xSemaphoreGive(string_sync);
                }}
                 
               
                  break;
              case UART_FIFO_OVF:
              case UART_BUFFER_FULL:
                  uart_flush_input(uart);
                  xQueueReset(queue);
                  break;
              default:
                  break;
              }
          }
      }
}

