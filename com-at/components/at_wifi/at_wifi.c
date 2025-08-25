// Eun0us - DVID - AT Wi-Fi module

#include "at_wifi.h"
#include "at_server.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "esp_event.h"
#include "esp_netif.h"
#include <string.h>
#include <stdlib.h>

#define UART_NUM UART_NUM_0

static const char *TAG = "AT_WIFI";

// Helper pour écrire sur l'uart proprement
static void uart_send(const char* s) {
    uart_write_bytes(UART_NUM, s, strlen(s));
}

// === HANDLERS ===

// AT+CWLAP (scan)
static void at_handle_cwlap(const char *params)
{
    ESP_LOGI(TAG, "CWLAP called, params: %s", params ? params : "(null)");
    wifi_scan_config_t scan_config = {0};
    uint16_t ap_num = 0;
    wifi_ap_record_t ap_records[20];

    esp_err_t err = esp_wifi_scan_start(&scan_config, true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_scan_start failed: %s", esp_err_to_name(err));
        uart_send("ERROR\r\n");
        return;
    }
    err = esp_wifi_scan_get_ap_num(&ap_num);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_scan_get_ap_num failed: %s", esp_err_to_name(err));
        ap_num = 0;
    }
    if (ap_num > 20) ap_num = 20;
    err = esp_wifi_scan_get_ap_records(&ap_num, ap_records);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_scan_get_ap_records failed: %s", esp_err_to_name(err));
        ap_num = 0;
    }
    ESP_LOGI(TAG, "Scan done, found %u APs", ap_num);

    for (int i = 0; i < ap_num; i++) {
        char line[128];
        snprintf(line, sizeof(line),
            "+CWLAP:(%d,\"%s\",%d,\"%02X:%02X:%02X:%02X:%02X:%02X\",%d)\r\n",
            ap_records[i].authmode,
            ap_records[i].ssid,
            ap_records[i].rssi,
            ap_records[i].bssid[0], ap_records[i].bssid[1], ap_records[i].bssid[2],
            ap_records[i].bssid[3], ap_records[i].bssid[4], ap_records[i].bssid[5],
            ap_records[i].primary
        );
        uart_send(line);
        ESP_LOGI(TAG, "AP[%d]: ssid=%s, rssi=%d, auth=%d", i, ap_records[i].ssid, ap_records[i].rssi, ap_records[i].authmode);
    }
    uart_send("OK\r\n");
}

// AT+CWJAP="SSID","PASS"
static bool parse_cwjap_params(const char *params, char *ssid, char *pass, size_t max_ssid, size_t max_pass)
{
    ESP_LOGI(TAG, "parse_cwjap_params called with params: %s", params ? params : "(null)");
    const char *p1 = strchr(params, '"');
    if (!p1) { ESP_LOGE(TAG, "SSID start quote not found"); return false; }
    const char *p2 = strchr(p1 + 1, '"');
    if (!p2) { ESP_LOGE(TAG, "SSID end quote not found"); return false; }
    size_t len_ssid = p2 - p1 - 1;
    if (len_ssid >= max_ssid) { ESP_LOGE(TAG, "SSID too long"); return false; }
    strncpy(ssid, p1 + 1, len_ssid);
    ssid[len_ssid] = 0;

    const char *p3 = strchr(p2 + 1, '"');
    if (!p3) { ESP_LOGE(TAG, "PASS start quote not found"); return false; }
    const char *p4 = strchr(p3 + 1, '"');
    if (!p4) { ESP_LOGE(TAG, "PASS end quote not found"); return false; }
    size_t len_pass = p4 - p3 - 1;
    if (len_pass >= max_pass) { ESP_LOGE(TAG, "PASS too long"); return false; }
    strncpy(pass, p3 + 1, len_pass);
    pass[len_pass] = 0;

    ESP_LOGI(TAG, "Parsed SSID: '%s', PASS: '%s'", ssid, pass);
    return true;
}

static void at_handle_cwjap(const char *params)
{
    ESP_LOGI(TAG, "CWJAP called, params: %s", params ? params : "(null)");
    char ssid[33], pass[65];
    if (!parse_cwjap_params(params, ssid, pass, sizeof(ssid), sizeof(pass))) {
        ESP_LOGE(TAG, "Param parse failed for: %s", params);
        uart_send("ERROR\r\n");
        return;
    }
    ESP_LOGI(TAG, "Try connect to SSID: '%s'", ssid);

    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));

    esp_wifi_disconnect();
    esp_err_t err;
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_mode failed: %s", esp_err_to_name(err));
        uart_send("ERROR\r\n");
        return;
    }
    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_config failed: %s", esp_err_to_name(err));
        uart_send("ERROR\r\n");
        return;
    }
    err = esp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_connect failed: %s", esp_err_to_name(err));
        uart_send("ERROR\r\n");
        return;
    }
    ESP_LOGI(TAG, "Connect command sent.");
    uart_send("OK\r\n");
}

// AT+CWJAP? : Affiche le SSID connecté
static void at_handle_cwjap_query(const char *params)
{
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        char buf[128];
        snprintf(buf, sizeof(buf), "+CWJAP:\"%s\"\r\nOK\r\n", ap_info.ssid);
        uart_send(buf);
    } else {
        uart_send("ERROR\r\n");
    }
}

// AT+CWQAP : Déconnexion
static void at_handle_cwqap(const char *params)
{
    ESP_LOGI(TAG, "CWQAP called");
    esp_err_t err = esp_wifi_disconnect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_disconnect failed: %s", esp_err_to_name(err));
        uart_send("ERROR\r\n");
    } else {
        uart_send("OK\r\n");
    }
}

// AT+CWSTATE? : Etat connexion
static void at_handle_cwstate(const char *params)
{
    wifi_ap_record_t ap_info;
    int connected = (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) ? 1 : 0;
    ESP_LOGI(TAG, "CWSTATE? connected=%d", connected);
    char resp[32];
    snprintf(resp, sizeof(resp), "+CWSTATE:%d\r\nOK\r\n", connected);
    uart_send(resp);
}

// AT+CIFSR : Adresse IP locale
static void at_handle_cifsr(const char *params)
{
    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        char resp[64];
        uint32_t ip = ip_info.ip.addr;
        snprintf(resp, sizeof(resp), "+CIFSR:\"%d.%d.%d.%d\"\r\nOK\r\n",
                 (int)(ip & 0xFF),
                 (int)((ip >> 8) & 0xFF),
                 (int)((ip >> 16) & 0xFF),
                 (int)((ip >> 24) & 0xFF));
        ESP_LOGI(TAG, "CIFSR: Got IP: %d.%d.%d.%d",
                 (int)(ip & 0xFF),
                 (int)((ip >> 8) & 0xFF),
                 (int)((ip >> 16) & 0xFF),
                 (int)((ip >> 24) & 0xFF));
        uart_send(resp);
    } else {
        ESP_LOGE(TAG, "CIFSR: Could not get IP (netif=%p)", netif);
        uart_send("ERROR\r\n");
    }
}

// AT+CWMODE? : Query mode
static void at_handle_cwmode_query(const char *params)
{
    wifi_mode_t mode;
    if (esp_wifi_get_mode(&mode) == ESP_OK) {
        char buf[32];
        snprintf(buf, sizeof(buf), "+CWMODE:%d\r\nOK\r\n", mode);
        uart_send(buf);
    } else {
        uart_send("ERROR\r\n");
    }
}

// AT+CWMODE=x : Set mode
static void at_handle_cwmode_set(const char *params)
{
    int mode = atoi(params);
    if (mode < 1 || mode > 3) {
        uart_send("ERROR\r\n");
        return;
    }
    if (esp_wifi_set_mode((wifi_mode_t)mode) == ESP_OK) {
        uart_send("OK\r\n");
    } else {
        uart_send("ERROR\r\n");
    }
}

// AT+CWAP="ssid","pass",ch,auth : Crée un hotspot Wi-Fi SoftAP
static void at_handle_cwap(const char *params)
{
    // Format attendu : "ssid","pass",ch,authmode
    char ssid[33] = {0}, pass[65] = {0};
    int channel = 1, authmode = 0;
    // Parsing simple
    if (sscanf(params, "\"%32[^\"]\",\"%64[^\"]\",%d,%d", ssid, pass, &channel, &authmode) < 4) {
        uart_send("ERROR\r\n");
        return;
    }

    wifi_config_t ap_config = {0};
    strncpy((char*)ap_config.ap.ssid, ssid, sizeof(ap_config.ap.ssid));
    strncpy((char*)ap_config.ap.password, pass, sizeof(ap_config.ap.password));
    ap_config.ap.ssid_len = strlen(ssid);
    ap_config.ap.channel = channel;
    ap_config.ap.max_connection = 4;
    ap_config.ap.authmode = (wifi_auth_mode_t)authmode;
    if (authmode == 0) ap_config.ap.authmode = WIFI_AUTH_OPEN;

    esp_wifi_set_mode(WIFI_MODE_AP);

    if (esp_wifi_set_config(WIFI_IF_AP, &ap_config) == ESP_OK) {
        uart_send("OK\r\n");
    } else {
        uart_send("ERROR\r\n");
    }
}

// === REGISTRATION ===

void wifi_at_register(void)
{
    // Initialisation Wi-Fi si besoin (ne le fais qu'une fois !)
    static bool wifi_started = false;
    if (!wifi_started) {
        wifi_started = true;
        esp_netif_init();
        esp_event_loop_create_default();
        esp_netif_create_default_wifi_sta();
        esp_netif_create_default_wifi_ap();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_wifi_init(&cfg);
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_start();
        ESP_LOGI(TAG, "Wi-Fi initialized in STA mode");
    }

    static const at_command_t cmds[] = {
        {"AT+CWLAP",     at_handle_cwlap,   "Scan Wi-Fi"},
        {"AT+CWJAP",     at_handle_cwjap,   "Connexion Wi-Fi"},
        {"AT+CWJAP?",    at_handle_cwjap_query, "SSID connecté actuellement"},
        {"AT+CWQAP",     at_handle_cwqap,   "Déconnexion Wi-Fi"},
        {"AT+CWSTATE?",  at_handle_cwstate, "Etat connexion Wi-Fi"},
        {"AT+CIFSR",     at_handle_cifsr,   "Adresse IP locale"},
        {"AT+CWMODE?",   at_handle_cwmode_query, "Lit le mode Wi-Fi (1=STA,2=AP,3=AP+STA)"},
        {"AT+CWMODE",    at_handle_cwmode_set,   "Change le mode Wi-Fi (1=STA,2=AP,3=AP+STA)"},
        {"AT+CWAP",      at_handle_cwap,    "Configure le SoftAP: AT+CWAP=\"ssid\",\"pass\",ch,auth"},
    };
    for (size_t i = 0; i < sizeof(cmds)/sizeof(cmds[0]); i++)
        at_server_register_command(&cmds[i]);
    ESP_LOGI(TAG, "All AT Wi-Fi commands registered.");
}


void at_wifi_test_all(void)
{
    printf("TEST AT+CWLAP...\n");
    at_handle_cwlap(NULL);

    printf("TEST AT+CWJAP...\n");
    at_handle_cwjap("\"Livebox-3EC0\",\"LK7sHJ4RmpXdDySvKz\"");

    printf("TEST AT+CWJAP?...\n");
    at_handle_cwjap_query(NULL);

    printf("TEST AT+CWSTATE?...\n");
    at_handle_cwstate(NULL);

    printf("TEST AT+CIFSR...\n");
    at_handle_cifsr(NULL);

    printf("TEST AT+CWQAP...\n");
    at_handle_cwqap(NULL);

    printf("TEST AT+CWMODE?...\n");
    at_handle_cwmode_query(NULL);

    printf("TEST AT+CWMODE=2...\n");
    at_handle_cwmode_set("2");

    printf("TEST AT+CWAP...\n");
    at_handle_cwap("\"TestAP\",\"test1234\",6,3");

    printf("DONE.\n");
}
