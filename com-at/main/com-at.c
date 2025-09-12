// Eun0us - DVID - main.c

#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "at_server.h"
#include "at_wifi.h"  // Ton module AT Wi-Fi
#include "at_ble.h"
#include "at_ota.h"
//#include "at_mqtt.h"


#define BUF_SIZE 1024

// Mets ici les pins que tu veux tester ! IO1/IO0 etc.
#define UART_TX_PIN         1      // IO1
#define UART_RX_PIN         0      // IO0

// Initialisation de l'UART pour AT et debug            
void uart_init(void)
{
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
}

void app_main(void)
{
    // Met le log global bien verbeux pour debug (optionnel)
    esp_log_level_set("*", ESP_LOG_INFO);

    // Initialisation non-volatile storage
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(ret);
    }

    printf("DVID com-at starting...\n");
    
    // UART ready pour AT et debug
    uart_init();

    // Enregistre toutes tes commandes Wi-Fi (init Wi-Fi incluse dans wifi_at_register)
    wifi_at_register();
    //at_wifi_test_all();

    // Initialise et enregistre les commandes BLE
    ble_at_register();
    //at_ble_test_all();


    //at_ota_register();
    //at_ota_test_all(); // Pas de test automatique pour OTA
    
    // Initialise et enregistre les commandes MQTT
    //mqtt_at_register();
    //at_mqtt_test_all();

    // Démarre le serveur AT (tâche FreeRTOS)
    at_server_start();
    // Le serveur AT tourne maintenant sur une tâche FreeRTOS, tout est prêt !
}
