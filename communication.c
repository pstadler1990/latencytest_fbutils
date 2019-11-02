//
// Created by pstadler on 25.10.19.
//
#include "communication.h"
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

extern int uart0_filestream;

/* https://www.einplatinencomputer.com/raspberry-pi-uart-senden-und-empfangen-in-c/ */
uint8_t
init_uart(const char* uartIdentifier) {
    /* */
    uart0_filestream = open(uartIdentifier, O_RDWR | O_NOCTTY | O_NDELAY);
    if(uart0_filestream == -1) {
        return 0;
    }

    struct termios options;
    tcgetattr(uart0_filestream, &options);
    options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(uart0_filestream, TCIFLUSH);
    tcsetattr(uart0_filestream, TCSANOW, &options);

    return 1;
}

uint8_t
uart_send(const uint8_t* buf, size_t len) {
    if (uart0_filestream != -1)	{
        ssize_t out = write(uart0_filestream, buf, len);
        if(out < 0) {
            return 0;
        }
    }
    return 1;
}

int
uart_receive(char* buf, size_t len) {
    ssize_t receivedBytes = read(uart0_filestream, (void*) buf, len);
    if(receivedBytes == 0) {
        return -1;
    } else if(receivedBytes > 0) {
        /* Receive buffer not empty */
        buf[receivedBytes] = 0;
        return receivedBytes;
    }
    return -1;
}