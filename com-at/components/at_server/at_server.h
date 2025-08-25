#pragma once

#include <stdbool.h>

typedef void (*at_cmd_handler_t)(const char *params);

typedef struct {
    const char *cmd;
    at_cmd_handler_t handler;
    const char *help;
} at_command_t;

void at_server_start(void);

bool at_server_register_command(const at_command_t *cmd);
