#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_https_ota.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "driver/uart.h"

#include "at_server.h"

static const char *TAG = "AT_OTA";

typedef enum {
    OTA_IDLE = 0,
    OTA_DOWNLOADING,
    OTA_FLASHING,
    OTA_OK,
    OTA_ERROR
} ota_status_t;

static ota_status_t g_ota_status = OTA_IDLE;
static esp_err_t g_ota_last_result = ESP_OK;

#define OTA_VERSION_STR_MAXLEN 64

// ====================== OTA CORE =========================
static void at_handle_ota(const char *params)
{
    while (*params == ' ' || *params == '=') params++;
    if (strlen(params) < 8) {
        uart_write_bytes(UART_NUM, "OTA ERROR\r\n", 11);
        ESP_LOGE(TAG, "Paramètre OTA invalide : '%s'", params);
        g_ota_status = OTA_ERROR;
        return;
    }

    char url[256] = {0};
    snprintf(url, sizeof(url), "%s", params);

    uart_write_bytes(UART_NUM, "OTA START\r\n", 11);
    ESP_LOGI(TAG, "Démarrage OTA depuis l'URL : %s", url);

    g_ota_status = OTA_DOWNLOADING;
    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 60000,
    };

    g_ota_status = OTA_FLASHING;
    g_ota_last_result = esp_https_ota(&config);

    if (g_ota_last_result == ESP_OK) {
        uart_write_bytes(UART_NUM, "OTA OK\r\n", 8);
        ESP_LOGI(TAG, "OTA success, reboot...");
        g_ota_status = OTA_OK;
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    } else {
        uart_write_bytes(UART_NUM, "OTA ERROR\r\n", 11);
        ESP_LOGE(TAG, "OTA failed, code=0x%x", g_ota_last_result);
        g_ota_status = OTA_ERROR;
    }
}

// ====================== VERSION =========================
static void at_handle_ota_version(const char *params)
{
    // Utilise esp_app_desc_t pour récupérer la version compilée
    const esp_app_desc_t *app_desc = esp_app_get_description();
    char ver_str[OTA_VERSION_STR_MAXLEN] = {0};
    snprintf(ver_str, sizeof(ver_str), "+OTAVERSION:\"%s\"\r\n", app_desc->version);
    uart_write_bytes(UART_NUM, ver_str, strlen(ver_str));
    uart_write_bytes(UART_NUM, "OK\r\n", 4);
}

// ====================== STATUS ==========================
static void at_handle_ota_status(const char *params)
{
    char resp[64];
    snprintf(resp, sizeof(resp), "+OTASTATUS:%d\r\n", g_ota_status);
    uart_write_bytes(UART_NUM, resp, strlen(resp));
    uart_write_bytes(UART_NUM, "OK\r\n", 4);
}

// ===================== ROLLBACK =========================
static void at_handle_ota_rollback(const char *params)
{
    // Essaie d'activer la partition précédente (OTA)
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *boot = esp_ota_get_boot_partition();
    const esp_partition_t *next_ota = esp_ota_get_next_update_partition(NULL);

    esp_err_t ret = esp_ota_mark_app_invalid_rollback_and_reboot();
    if (ret == ESP_OK) {
        uart_write_bytes(UART_NUM, "OK\r\n", 4);
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_restart();
    } else {
        uart_write_bytes(UART_NUM, "ERROR\r\n", 7);
    }
}

// ======================= INFO ===========================
static void at_handle_ota_info(const char *params)
{
    const esp_partition_t *part = esp_ota_get_running_partition();
    char resp[128];
    snprintf(resp, sizeof(resp), "+OTAINFO:partition=%s,offset=0x%lx,size=%lu\r\n",
             part->label, (unsigned long)part->address, (unsigned long)part->size);
    uart_write_bytes(UART_NUM, resp, strlen(resp));
    uart_write_bytes(UART_NUM, "OK\r\n", 4);
}

// ==================== ENREGISTREMENT ====================
void at_ota_register(void)
{
    static const at_command_t cmds[] = {
        { "AT+OTA",        at_handle_ota,         "AT+OTA=<url> : MAJ OTA depuis URL HTTP(S)" },
        { "AT+OTAVERSION?",at_handle_ota_version, "AT+OTAVERSION? : Version firmware actuelle" },
        { "AT+OTASTATUS?", at_handle_ota_status,  "AT+OTASTATUS? : Statut du dernier OTA" },
        { "AT+OTAROLLBACK",at_handle_ota_rollback,"AT+OTAROLLBACK : Reboot sur version précédente" },
        { "AT+OTAINFO?",   at_handle_ota_info,    "AT+OTAINFO? : Infos sur la partition OTA courante" }
    };
    for (size_t i = 0; i < sizeof(cmds)/sizeof(cmds[0]); ++i)
        at_server_register_command(&cmds[i]);
}


// Need to make a test function to verify OTA commands

void at_ota_test_all(void)
{
    printf("OTA TEST...\n");

    // Test version
    printf("STEP 1: OTAVERSION?\n");
    at_handle_ota_version(NULL);
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Test status
    printf("STEP 2: OTASTATUS?\n");
    at_handle_ota_status(NULL);
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Test info
    printf("STEP 3: OTAINFO?\n");
    at_handle_ota_info(NULL);
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Note: Cannot automatically test OTA and ROLLBACK without a valid URL and risking bricking the device.
    printf("OTA TEST COMPLETE. Manual testing required for OTA and ROLLBACK commands.\n");
}