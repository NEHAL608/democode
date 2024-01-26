/*
 * dgus.h
 * Created on: 27-May-2021
 * Author: Nehal
 */

#ifndef DGUS_LIBRARY_DGUS_LCD_DGUS_LCD_H_
#define DGUS_LIBRARY_DGUS_LCD_DGUS_LCD_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "dgus_reg.h"
#include "dgus_util.h"
#include <stddef.h>
#include <stdint.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"

typedef struct dgus_t dgus_t;
//#define _DEBUG_ON   1  // Uncomment this line to enable debug logs

#define PORT   UART_NUM_2

/**
 * @typedef dgus_handle_t
 * @brief Pointer to a dgus context.
 */
typedef dgus_t *dgus_handle_t;

typedef void (*TouchEventCb)(void *ptr);

typedef struct DGUS_obj
{
    uint16_t addr;   //SP address
    int32_t value;
    char *stringValue;    // Pointer to store string value

    uint8_t *rx_val;
    TouchEventCb __cb_func; // callback function name
    void *__cb_ptr;
}DGUS_obj;

typedef struct dgus_packet dgus_packet; // Opaque reference to a packet

void Touch_attach_cb(DGUS_obj *touch, TouchEventCb cbfunction, void *ptr);

void CreateDGUSObject(DGUS_obj *obj, uint32_t addr, uint16_t data);
void CreateDGUSObject_assign_string(DGUS_obj *obj, char* buffer);

void buffer_u8(dgus_packet *p, uint8_t *data, size_t len);
void buffer_u16(dgus_packet *p, uint16_t *data, size_t len);
void buffer_u32(dgus_packet *p, uint32_t *data, size_t len);
void buffer_u32_1(dgus_packet *p, uint32_t data);
void buffer_u32_1(dgus_packet *p, uint32_t data);

void dgus_packet_set_data(dgus_packet *p, uint8_t offset, uint8_t *data, uint8_t len);

DGUS_RETURN send_data(enum command cmd, dgus_packet *p);

DGUS_RETURN send_text_data(enum command cmd, dgus_packet *p) ;

DGUS_RETURN get_data(enum command cmd, dgus_packet *p,uint16_t* buf);

DGUS_RETURN get_string(enum command cmd, dgus_packet *p,uint16_t *buf);

DGUS_RETURN dgus_request_var(uint16_t addr, uint8_t len);

DGUS_RETURN dgus_get_var(uint16_t addr, uint16_t *buf, uint8_t len);

DGUS_RETURN dgus_set_var(uint16_t addr, uint32_t data);

DGUS_RETURN dgus_get_var8(uint16_t addr, uint8_t *buf, uint8_t len);

DGUS_RETURN dgus_set_var8(uint16_t addr, uint8_t *data, uint8_t len);

dgus_packet *dgus_packet_init();

void dgus_packet_set_len(dgus_packet *p, uint16_t len);

uint8_t *dgus_packet_get_recv_buffer();

DGUS_RETURN dgus_set_cmd(uint16_t addr, uint8_t *data, uint8_t len);

DGUS_RETURN recvRetNumber(uint16_t *number);

DGUS_RETURN recvRetCommandFinished();

DGUS_RETURN recvRetString(uint16_t *buffer);

DGUS_RETURN dgus_set_buffer(uint16_t addr,uint8_t data,uint8_t len);

/*it is used only when obj.value also automatic */
DGUS_RETURN dgus_set_val(DGUS_obj  *obj ,uint32_t data); 

void Touch_Event(DGUS_obj *D_list[],uint8_t *buffer,uint16_t len);

dgus_handle_t dgus_driver_install(uart_port_t uart_num,uint32_t baud_rate);

int serial_send_data(uint8_t* data,size_t len);

int32_t nextion_core_uart_read_as_byte(uint8_t *buffer, size_t length);

int sendcrc(uint8_t *buf,uint8_t len);

uint16_t Calculate_CRC16(uint8_t *updata, uint32_t len, uint8_t mode);

int sendToT5L(uint8_t *buf, uint16_t address, uint8_t len, uint8_t mode);

#endif /* DGUS_LIBRARY_DGUS_LCD_DGUS_LCD_H_ */
