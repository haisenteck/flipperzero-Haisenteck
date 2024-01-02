#pragma once

typedef struct SubGhz SubGhz;

typedef enum {
    UARTDefault, // pin 13,14
    UARTExtra, // pin 15,16
    UARTCount,
} UARTChannel;

typedef struct {
    UARTChannel uart_nmea_channel;
    UARTChannel uart_general_channel;
    UARTChannel uart_esp_channel;
} Gps_nm;