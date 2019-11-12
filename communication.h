//
// Created by pstadler on 25.10.19.
//

#ifndef FB_COMMUNICATION_H
#define FB_COMMUNICATION_H

#include <stdint.h>
#include <glob.h>
#include <stdbool.h>

typedef enum{
    CTRL_CMD_CALIB_MODE = 'C',
    CTRL_CMD_TEST_MODE = 'T',
    CTRL_CMD_GET_MEASURES = 'R',
    CTRL_CMD_CALIB_BLACK = 'B',
    CTRL_CMD_CALIB_WHITE = 'W',
} UART_CTRL_COMMAND;

uint8_t init_uart(const char* uartIdentifier);
uint8_t uart_send(const uint8_t* buf, size_t len);
int uart_receive(char* buf, size_t len);
uint8_t uart_send_command(UART_CTRL_COMMAND command, bool timed);
uint8_t uart_receive_response(uint32_t responseLen, const char* response, bool timed);
#endif //FB_COMMUNICATION_H
