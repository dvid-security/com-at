#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

#define BUF_SIZE 1024

// Mets ici les pins que tu veux tester ! IO1/IO0 etc.
#define UART_NUM            UART_NUM_1
#define UART_TX_PIN         1      // IO1
#define UART_RX_PIN         0      // IO0

void uart_task(void *arg) {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_driver_install(UART_NUM, BUF_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    int c = 0;
    while (1) {
        char msg[64];
        snprintf(msg, sizeof(msg), "C6 UART1 TX=IO1 RX=IO0 - Count:%d\r\n", c++);
        uart_write_bytes(UART_NUM, msg, strlen(msg));
        ESP_LOGI("DATA SEND UART", "OK");
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void app_main(void) {
    xTaskCreate(uart_task, "uart_task", 2048, NULL, 10, NULL);
    ESP_LOGI("UART_SLAVE", "Sending data continuously on UART1 (TX=IO1, RX=IO0)");
}
