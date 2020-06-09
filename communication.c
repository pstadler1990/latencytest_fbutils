//
// Created by pstadler on 25.10.19.
//
#include "communication.h"
#include "configuration.h"
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <memory.h>

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
    options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag |= ICANON;
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
        buf[receivedBytes] = '\0';
        return (int) receivedBytes;
    }
    return -1;
}

uint8_t
uart_send_command(UART_CTRL_COMMAND command, bool timed) {
    uint8_t buf[2] = {
        command,
        '\n'
    };

    int32_t receiveStatus = -1;
    uint32_t m_timeout = MEASUREMENT_TIMEOUT;

    /* Wait for trigger reception from STM8 to enable ADC mode */
    while(1) {
        char receiveBuf[10];

        uart_send(buf, 2);

        receiveStatus = uart_receive(receiveBuf, 10);
        if(receiveStatus) {

            if(strncmp("OK", receiveBuf, 2) == 0) {
                return 1;
            }
        }
        if(timed && --m_timeout == 0) {
            printf("Failed to send command, exit\n");
            break;
        }
        sleep(1);
    }
    return 0;
}

uint8_t
uart_receive_response(const uint32_t responseLen, const char* response, bool timed) {
    /* */
    int32_t receiveStatus = -1;
    uint32_t m_timeout = MEASUREMENT_TIMEOUT;

    while(1) {
        char receiveBuf[responseLen + 2];

        receiveStatus = uart_receive(receiveBuf, responseLen); 
        if(receiveStatus > 0) {
            if(strncmp(response, receiveBuf, responseLen) == 0) {
                return 1;
            }
        }

        if(timed && --m_timeout == 0) {
            break;
        }
    }
    return 0;
}

uint8_t
com_get_display_name(char* nameBuf, size_t nameBufSize) {
    /* Stores the name of the attached monitor (if possible) in the given buffer
       Uses popen to process external command tvservice */
    char tmpBuf[EDID_MAX_DISPLAY_NAME];    //device_name=

    FILE* fp = popen("tvservice -n", "r");
    if(!fp) {
       return 0;
    }

    while(fgets(tmpBuf, EDID_MAX_DISPLAY_NAME + 1, fp));

    tmpBuf[strlen(tmpBuf) - 1] = '\0';
    memcpy(nameBuf, &tmpBuf[12], nameBufSize + 1);

    if(pclose(fp)) {
        return 0;
    }

    return 1;
}
