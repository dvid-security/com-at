// Eun0us - DVID - AT Server

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "at_server.h"
#include "esp_idf_version.h"  // Pour version IDF

#define UART_NUM UART_NUM_0
#define UART_RX_BUF_SIZE 1024
#define AT_MAX_CMDS 32
#define AT_MAX_CMD_LEN 128

static const char *TAG = "AT_SERVER";

// Table dynamique des commandes AT
typedef void (*at_handler_t)(const char *params);

static at_command_t at_cmd_table[AT_MAX_CMDS];
static int at_cmd_count = 0;

// ====== ENREGISTREMENT DES COMMANDES ======
bool at_server_register_command(const at_command_t *cmd)
{
    if (at_cmd_count >= AT_MAX_CMDS) return false;
    at_cmd_table[at_cmd_count++] = *cmd;
    return true;
}

// ====== HANDLERS DE BASE ======
static void at_handle_test(const char *params)
{
    uart_write_bytes(UART_NUM, "OK\r\n", 4);
}

static void at_handle_help(const char *params)
{
    for (int i = 0; i < at_cmd_count; i++) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%s : %s\r\n",
                 at_cmd_table[i].cmd,
                 at_cmd_table[i].help ? at_cmd_table[i].help : "");
        uart_write_bytes(UART_NUM, buf, strlen(buf));
    }
    uart_write_bytes(UART_NUM, "OK\r\n", 4);
}

// ====== HANDLER POUR AT+GMR ======
static void at_handle_gmr(const char *params)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "ESP-IDF version: %d.%d.%d\r\n",
             ESP_IDF_VERSION_MAJOR,
             ESP_IDF_VERSION_MINOR,
             ESP_IDF_VERSION_PATCH);
    uart_write_bytes(UART_NUM, buf, strlen(buf));
    uart_write_bytes(UART_NUM, "OK\r\n", 4);
}

// ====== AJOUT COMMANDES DE BASE ======
static void at_server_register(void)
{
    static const at_command_t cmds[] = {
        { "AT",       at_handle_test, "Test, retourne OK" },
        { "AT+HELP",  at_handle_help, "Liste les commandes" },
        { "AT+GMR",   at_handle_gmr, "Affiche la version firmware" },
    };
    for (size_t i = 0; i < sizeof(cmds)/sizeof(cmds[0]); i++)
        at_server_register_command(&cmds[i]);
}

// ====== PARSER PRINCIPAL ======
static void at_server_handle(const char *line)
{
    char cmd[AT_MAX_CMD_LEN] = {0};
    const char *params = "";

    int len = strlen(line);
    int split = len; // par défaut: tout copier
    for (int i = 0; i < len && i < AT_MAX_CMD_LEN - 1; i++) {
        if (line[i] == '=' || line[i] == ' ' || line[i] == '\0') {
            split = i;
            break;
        }
    }
    strncpy(cmd, line, split);
    cmd[split] = '\0';

    if (split < len && (line[split] == '=' || line[split] == ' '))
        params = &line[split + 1];

    // Recherche et appel du handler
    for (int i = 0; i < at_cmd_count; i++) {
        if (strcasecmp(cmd, at_cmd_table[i].cmd) == 0) {
            at_cmd_table[i].handler(params);
            return;
        }
    }
    uart_write_bytes(UART_NUM, "ERROR\r\n", 7);
}

// ====== TASK UART PRINCIPALE ======
static void at_uart_task(void *pvParameters)
{
    static char rx_buf[UART_RX_BUF_SIZE];
    int rx_idx = 0;

    while (1) {
        uint8_t c;
        int len = uart_read_bytes(UART_NUM, &c, 1, pdMS_TO_TICKS(10));
        if (len > 0) {
            if (c == '\r' || c == '\n') {
                if (rx_idx > 0) {
                    rx_buf[rx_idx] = '\0';
                    ESP_LOGI(TAG, "AT: '%s'", rx_buf);
                    at_server_handle(rx_buf);
                    rx_idx = 0;
                }
            } else if (rx_idx < UART_RX_BUF_SIZE - 1) {
                rx_buf[rx_idx++] = c;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// ====== DEMARRAGE SERVEUR AT ======
void at_server_start(void)
{
    at_server_register();
    ESP_LOGI(TAG, "Table AT après init:");
    for (int i = 0; i < at_cmd_count; i++) {
        ESP_LOGI(TAG, "  cmd[%d]: %s", i, at_cmd_table[i].cmd);
    }
    ESP_LOGI(TAG, "AT Server started (UART task)...");
    xTaskCreate(at_uart_task, "at_uart_task", 4096, NULL, 10, NULL);
}