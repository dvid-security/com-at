#pragma once

#include <stdbool.h>
#include "driver/uart.h"

#define UART_NUM            UART_NUM_1
#define UART_RX_BUF_SIZE 1024

typedef void (*at_cmd_handler_t)(const char *params);

typedef struct {
    const char *cmd;
    at_cmd_handler_t handler;
    const char *help;
} at_command_t;

void at_server_start(void);
bool at_server_register_command(const at_command_t *cmd);
