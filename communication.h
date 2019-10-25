//
// Created by pstadler on 25.10.19.
//

#ifndef FB_COMMUNICATION_H
#define FB_COMMUNICATION_H

#include <stdint.h>
#include <glob.h>

uint8_t init_uart(const char* uartIdentifier);
uint8_t uart_send(const uint8_t* buf, size_t len);
int uart_receive(char* buf, size_t len);
#endif //FB_COMMUNICATION_H
