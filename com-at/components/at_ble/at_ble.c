#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "esp_log.h"
#include "esp_err.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "esp_mac.h"
#include "at_server.h"

#define TAG "AT_BLE"
#define MAX_DYNAMIC_CHARS    8
#define MAX_GATT_STR         256
#define DYN_BUF_SIZE         32

// -------- Prototypes --------
static void ble_host_task(void *param);
static void ble_gap_on_sync(void);
static void at_ble_init_stack(void);
static void at_ble_deinit_stack(void);
static void start_ble_advertising(void);

void at_handle_blebegin(const char *params);
void at_handle_blemfg(const char *params);
void at_handle_bleadvstop(const char *params);
void at_handle_bleadvstart(const char *params);
void at_handle_blestop(const char *params);
void at_handle_bleclear(const char *params);
void at_handle_blesetname(const char *params);
void at_handle_blescan(const char *params);
void at_handle_gattbuild(const char *params);
void at_handle_blemaddr(const char *params);
void ble_at_register(void);

// ------ BLE globals ------
static bool ble_inited = false;
static char g_current_name[32] = "DVID_BLE";
static uint8_t g_manu_data[32] = {0};
static uint8_t g_manu_data_len = 0;

// ------ Dynamic GATT ------
static struct ble_gatt_svc_def *gatt_svcs = NULL;
static struct ble_gatt_chr_def *gatt_chars = NULL;
static int dyn_n_chars = 0;
static uint8_t dyn_gatt_vals[MAX_DYNAMIC_CHARS][DYN_BUF_SIZE] = {0};
static uint16_t dyn_gatt_val_lens[MAX_DYNAMIC_CHARS] = {0};

// ------ MAC Override ------
static uint8_t g_mac_override[6] = {0};
static bool g_mac_override_enabled = false;

// ------- Dynamic GATT management -------
int parse_flags(const char *s) {
    int flags = 0;
    if (strchr(s, 'r')) flags |= BLE_GATT_CHR_F_READ;
    if (strchr(s, 'w')) flags |= BLE_GATT_CHR_F_WRITE;
    if (strchr(s, 'n')) flags |= BLE_GATT_CHR_F_NOTIFY;
    return flags;
}
void free_gatt_tables() {
    if (gatt_svcs) { ESP_LOGI(TAG, "free gatt_svcs %p", gatt_svcs); free(gatt_svcs); gatt_svcs = NULL; }
    if (gatt_chars) { ESP_LOGI(TAG, "free gatt_chars %p", gatt_chars); free(gatt_chars); gatt_chars = NULL; }
    dyn_n_chars = 0;
    memset(dyn_gatt_vals, 0, sizeof(dyn_gatt_vals));
    memset(dyn_gatt_val_lens, 0, sizeof(dyn_gatt_val_lens));
}
static int generic_access_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    int idx = (int)(intptr_t)arg;
    if(idx < 0 || idx >= MAX_DYNAMIC_CHARS) return BLE_ATT_ERR_UNLIKELY;
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        os_mbuf_append(ctxt->om, dyn_gatt_vals[idx], dyn_gatt_val_lens[idx]);
        ESP_LOGI(TAG, "GATT: READ idx=%d len=%d", idx, dyn_gatt_val_lens[idx]);
        return 0;
    } else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        int len = OS_MBUF_PKTLEN(ctxt->om);
        if (len > DYN_BUF_SIZE) len = DYN_BUF_SIZE;
        os_mbuf_copydata(ctxt->om, 0, len, dyn_gatt_vals[idx]);
        dyn_gatt_val_lens[idx] = len;
        ESP_LOGI(TAG, "GATT: WRITE idx=%d len=%d", idx, len);
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

// ----------- BLE Stack Init/Deinit ------------
static void at_ble_init_stack(void) {
    if (ble_inited) return;
    // --- Override MAC si demandé ---
    if (g_mac_override_enabled) {
        esp_err_t ret = esp_base_mac_addr_set(g_mac_override);
        ESP_LOGI(TAG, "esp_base_mac_addr_set: %s", esp_err_to_name(ret));
    }
    ESP_ERROR_CHECK(nimble_port_init());
    ble_hs_cfg.sync_cb = ble_gap_on_sync;
    ble_svc_gap_init();
    ble_svc_gatt_init();
    // --- NE PAS AJOUTER DE SERVICE GATT si aucun GATT n'est défini !
    if (gatt_svcs) {
        int err = 0;
        err = ble_gatts_count_cfg(gatt_svcs);
        ESP_LOGI(TAG, "count_cfg err=%d", err);
        err = ble_gatts_add_svcs(gatt_svcs);
        ESP_LOGI(TAG, "add_svcs err=%d", err);
        if (err != 0)
            ESP_LOGE(TAG, "Failed to add dynamic GATT table, err=%d", err);
        else
            ESP_LOGI(TAG, "NimBLE using DYNAMIC GATT");
    }
    // Pas de dummy service : SANS GATT => BLE pub only = OK !
    nimble_port_freertos_init(ble_host_task);
    ble_inited = true;
    ESP_LOGI(TAG, "NimBLE initialized OK!");
}
static void at_ble_deinit_stack(void) {
    if (!ble_inited) return;
    nimble_port_stop();
    nimble_port_deinit();
    ble_inited = false;
    ESP_LOGI(TAG, "NimBLE stopped/deinit OK!");
}
static void ble_host_task(void *param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}
static void ble_gap_on_sync(void) {
    ESP_LOGI(TAG, "NimBLE synced! Ready.");
}

// ------------ Extract name from ADV data --------------
static void ble_adv_extract_name(const uint8_t *adv_data, uint8_t adv_len, char *name, size_t name_len) {
    uint8_t len, type;
    uint8_t idx = 0;
    while (idx < adv_len) {
        len = adv_data[idx++];
        if (len == 0) break;
        type = adv_data[idx++];
        if (type == 0x09) { // Complete Local Name
            size_t to_copy = len-1;
            if (to_copy >= name_len) to_copy = name_len-1;
            memcpy(name, &adv_data[idx], to_copy);
            name[to_copy] = '\0';
            return;
        }
        idx += len-1;
    }
    name[0] = 0;
}

// ----------- BLE GAP Callbacks (scan optionnel) ------------
static int ble_scan_cb(struct ble_gap_event *event, void *arg) {
    if (event->type == BLE_GAP_EVENT_DISC) {
        char addr_str[18] = {0};
        sprintf(addr_str, "%02X:%02X:%02X:%02X:%02X:%02X",
                event->disc.addr.val[5], event->disc.addr.val[4], event->disc.addr.val[3],
                event->disc.addr.val[2], event->disc.addr.val[1], event->disc.addr.val[0]);
        char name[32] = "";
        ble_adv_extract_name((const uint8_t*)event->disc.data, event->disc.length_data, name, sizeof(name));
        char line[128];
        if (strlen(name) > 0)
            snprintf(line, sizeof(line), "+BLESCAN:%s,%d,\"%s\"\r\n", addr_str, event->disc.rssi, name);
        else
            snprintf(line, sizeof(line), "+BLESCAN:%s,%d\r\n", addr_str, event->disc.rssi);
        uart_write_bytes(UART_NUM, line, strlen(line));
        ESP_LOGI(TAG, "Found BLE: %s (RSSI %d, name='%s')", addr_str, event->disc.rssi, name);
    }
    if (event->type == BLE_GAP_EVENT_DISC_COMPLETE) {
        uart_write_bytes(UART_NUM, "OK\r\n", 4);
    }
    return 0;
}

// ----------- ADV, STOP, RENAME, etc ------------
static void start_ble_advertising(void)   {
    struct ble_hs_adv_fields fields = {0};
    fields.name = (uint8_t*)g_current_name;
    fields.name_len = strlen(g_current_name);
    fields.name_is_complete = 1;
    if (g_manu_data_len > 0) {
        fields.mfg_data = g_manu_data;
        fields.mfg_data_len = g_manu_data_len;
    }
    ble_gap_adv_set_fields(&fields);
    struct ble_gap_adv_params adv_params = {0};
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    adv_params.itvl_min = 0x20;    
    adv_params.itvl_max = 0x40;  
    adv_params.channel_map = 7;  // tous canaux
    
    ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, NULL, NULL);
    
    ESP_LOGI(TAG, "BLE Advertising started! Name: %s", g_current_name);
}

// --------------- AT COMMANDS -----------------

void at_handle_blebegin(const char *params) {
    char name[32] = "DemoESP";
    char manu_ascii[32] = "";
    int n = sscanf(params, "\"%31[^\"]\";\"%31[^\"]\"", name, manu_ascii);
    if (n < 1) { uart_write_bytes(UART_NUM, "ERROR\r\n", 7); return; }
    strncpy(g_current_name, name, sizeof(g_current_name)-1);
    g_current_name[sizeof(g_current_name)-1]=0;
    g_manu_data_len = strlen(manu_ascii);
    if (g_manu_data_len > 0 && g_manu_data_len < sizeof(g_manu_data)) {
        memcpy(g_manu_data, manu_ascii, g_manu_data_len);
    } else if (g_manu_data_len >= sizeof(g_manu_data)) {
        memcpy(g_manu_data, manu_ascii, sizeof(g_manu_data));
        g_manu_data_len = sizeof(g_manu_data);
    } else {
        g_manu_data_len = 0;
    }
    at_ble_deinit_stack();
    at_ble_init_stack();
    start_ble_advertising();
    uart_write_bytes(UART_NUM, "OK\r\n", 4);
}

void at_handle_blemfg(const char *params) {
    char manu_ascii[32] = "";
    int n = sscanf(params, "\"%31[^\"]\"", manu_ascii);
    if (n < 1) { uart_write_bytes(UART_NUM, "ERROR\r\n", 7); return; }
    g_manu_data_len = strlen(manu_ascii);
    if (g_manu_data_len > 0 && g_manu_data_len < sizeof(g_manu_data)) {
        memcpy(g_manu_data, manu_ascii, g_manu_data_len);
    } else if (g_manu_data_len >= sizeof(g_manu_data)) {
        memcpy(g_manu_data, manu_ascii, sizeof(g_manu_data));
        g_manu_data_len = sizeof(g_manu_data);
    } else {
        g_manu_data_len = 0;
    }
    start_ble_advertising();
    ESP_LOGI(TAG, "BLE manufacturer data (ASCII) set: '%.*s'", g_manu_data_len, g_manu_data);
    uart_write_bytes(UART_NUM, "OK\r\n", 4);
}

void at_handle_bleadvstop(const char *params) {
    int ret = ble_gap_adv_stop();
    if (ret == 0) {
        ESP_LOGI(TAG, "BLE advertising stopped.");
        uart_write_bytes(UART_NUM, "OK\r\n", 4);
    } else {
        uart_write_bytes(UART_NUM, "ERROR\r\n", 7);
    }
}

void at_handle_bleadvstart(const char *params) {
    start_ble_advertising();
    uart_write_bytes(UART_NUM, "OK\r\n", 4);
}

void at_handle_blestop(const char *params) {
    at_ble_deinit_stack();
    ESP_LOGI(TAG, "BLE stack stopped.");
    uart_write_bytes(UART_NUM, "OK\r\n", 4);
}

void at_handle_bleclear(const char *params) {
    at_ble_deinit_stack();
    free_gatt_tables();
    ESP_LOGI(TAG, "Conf BLE cleared/reset.");
    uart_write_bytes(UART_NUM, "OK\r\n", 4);
}

void at_handle_blesetname(const char *params) {
    char name[32]="";
    int n = sscanf(params, "\"%31[^\"]\"", name);
    if (n < 1) { uart_write_bytes(UART_NUM, "ERROR\r\n", 7); return; }
    strncpy(g_current_name, name, sizeof(g_current_name)-1);
    g_current_name[sizeof(g_current_name)-1]=0;
    start_ble_advertising();
    ESP_LOGI(TAG, "BLE name set to: %s", g_current_name);
    uart_write_bytes(UART_NUM, "OK\r\n", 4);
}

void at_handle_blescan(const char *params) {
    int duration = 5;
    if (params && *params) sscanf(params, "%d", &duration);
    if (duration <= 0 || duration > 60) { 
        uart_write_bytes(UART_NUM, "ERROR\r\n", 7); 
        return; 
    }
    struct ble_gap_disc_params scan_params = {0};
    scan_params.passive = 1;
    scan_params.itvl = 0x0010;
    scan_params.window = 0x0010;
    scan_params.filter_duplicates = 1;
    int ret = ble_gap_disc(BLE_OWN_ADDR_PUBLIC, duration * 1000, &scan_params, ble_scan_cb, NULL);
    if (ret != 0) {
        uart_write_bytes(UART_NUM, "ERROR\r\n", 7);
    }
    // pas de OK ici ! C'est le callback qui l'enverra en fin de scan
}

// --- MAC OVERRIDE Handler
void at_handle_blemaddr(const char *params) {
    uint32_t mac_bytes[6];
    int n = sscanf(params, "\"%02lX:%02lX:%02lX:%02lX:%02lX:%02lX\"",
        &mac_bytes[0], &mac_bytes[1], &mac_bytes[2], &mac_bytes[3], &mac_bytes[4], &mac_bytes[5]);
    if (n != 6) { uart_write_bytes(UART_NUM, "ERROR\r\n", 7); return; }
    for (int i = 0; i < 6; i++) g_mac_override[i] = (uint8_t)mac_bytes[i];
    g_mac_override_enabled = true;
    ESP_LOGI(TAG, "BLE MAC override set: %02X:%02X:%02X:%02X:%02X:%02X",
        g_mac_override[0],g_mac_override[1],g_mac_override[2],g_mac_override[3],g_mac_override[4],g_mac_override[5]);
    uart_write_bytes(UART_NUM, "OK\r\n", 4);
}

// --- Dynamic GATT build ---
void at_handle_gattbuild(const char *params) {
    if (!params || strlen(params) == 0) { uart_write_bytes(UART_NUM, "ERROR\r\n", 7); return; }
    ESP_LOGI(TAG, "GATTBUILD start: params='%s'", params);
    at_ble_deinit_stack();
    free_gatt_tables();

    char copy[MAX_GATT_STR]; strncpy(copy, params, sizeof(copy)-1); copy[sizeof(copy)-1]=0;
    char *token, *saveptr;
    char *fields[1 + MAX_DYNAMIC_CHARS] = {0};
    int count = 0;
    for (token = strtok_r(copy, ";", &saveptr); token && count < MAX_DYNAMIC_CHARS+1; token = strtok_r(NULL, ";", &saveptr))
        fields[count++] = token;

    if (count < 2) { uart_write_bytes(UART_NUM, "ERROR\r\n", 7); return; }

    char svc_str[8]; sscanf(fields[0], "\"%4[^\"]\"", svc_str);
    uint16_t svc_uuid = (uint16_t)strtol(svc_str, NULL, 16);

    int n_chars = count-1;
    if(n_chars > MAX_DYNAMIC_CHARS) n_chars = MAX_DYNAMIC_CHARS;
    dyn_n_chars = n_chars;
    gatt_chars = calloc(n_chars+1, sizeof(struct ble_gatt_chr_def));
    if (!gatt_chars) { uart_write_bytes(UART_NUM, "ERROR\r\n", 7); return; }
    ESP_LOGI(TAG, "calloc gatt_chars %p (n=%d)", gatt_chars, n_chars+1);
    for (int i=0; i<n_chars; ++i) {
        char uuid_str[8]="", flag_str[8]="";
        int got = sscanf(fields[1+i], "\"%4[^\"]:%7[^\"]\"", uuid_str, flag_str);
        if (got != 2) { uart_write_bytes(UART_NUM, "ERROR\r\n", 7); free_gatt_tables(); return; }
        uint16_t uuid = (uint16_t)strtol(uuid_str, NULL, 16);
        int flags = parse_flags(flag_str);
        gatt_chars[i].uuid = BLE_UUID16_DECLARE(uuid);
        gatt_chars[i].access_cb = generic_access_cb;
        gatt_chars[i].arg = (void*)(intptr_t)i;
        gatt_chars[i].flags = flags;
        dyn_gatt_vals[i][0] = 0;
        dyn_gatt_val_lens[i] = 1;
        ESP_LOGI(TAG, "char #%d: uuid=0x%04X flags=0x%02X ptr=%p", i, uuid, flags, &gatt_chars[i]);
    }
    memset(&gatt_chars[n_chars], 0, sizeof(struct ble_gatt_chr_def));
    ESP_LOGI(TAG, "gatt_chars[%d] = END", n_chars);

    gatt_svcs = calloc(2, sizeof(struct ble_gatt_svc_def));
    if (!gatt_svcs) { free(gatt_chars); uart_write_bytes(UART_NUM, "ERROR\r\n", 7); return; }
    ESP_LOGI(TAG, "calloc gatt_svcs %p", gatt_svcs);
    gatt_svcs[0].type = BLE_GATT_SVC_TYPE_PRIMARY;
    gatt_svcs[0].uuid = BLE_UUID16_DECLARE(svc_uuid);
    gatt_svcs[0].characteristics = gatt_chars;
    memset(&gatt_svcs[1], 0, sizeof(struct ble_gatt_svc_def));
    ESP_LOGI(TAG, "gatt_svcs[0]: uuid=0x%04X chars=%p", svc_uuid, gatt_chars);

    ESP_LOGI(TAG, "About to init BLE stack (dynamic gatt_svcs=%p)...", gatt_svcs);
    at_ble_init_stack();
    start_ble_advertising();
    printf("GATT table rebuilt OK : service 0x%04X, %d characs\n", svc_uuid, n_chars);
    uart_write_bytes(UART_NUM, "OK\r\n", 4);
}

// ------------ REGISTRATION (à appeler dans app_main/init) ------------
void ble_at_register(void) {
    static const at_command_t ble_cmds[] = {
        {"AT+BLEBEGIN",   at_handle_blebegin,   "Start BLE: AT+BLEBEGIN=\"Name\";\"ManuHex\""},
        {"AT+BLEADVSTOP", at_handle_bleadvstop, "Stop Advertising BLE"},
        {"AT+BLEADVSTART",at_handle_bleadvstart,"(Re)Start Advertising BLE"},
        {"AT+BLESTOP",    at_handle_blestop,    "Stop BLE stack"},
        {"AT+BLECLEAR",   at_handle_bleclear,   "Clear BLE stack/config"},
        {"AT+BLESETNAME", at_handle_blesetname, "Set BLE adv name: AT+BLESETNAME=\"Name\""},
        {"AT+BLEMFG",     at_handle_blemfg,     "Set Manufacturer Data: AT+BLEMFG=\"DVID_AT\""},
        {"AT+BLESCAN",    at_handle_blescan,    "Scan for BLE devices: AT+BLESCAN=secs"},
        {"AT+GATTBUILD",  at_handle_gattbuild,  "Build custom GATT: AT+GATTBUILD=\"svcUUID\";\"charUUID:flags\";..."},
        {"AT+BLEMADDR",   at_handle_blemaddr,   "Change BLE MAC: AT+BLEMADDR=\"AA:BB:CC:DD:EE:FF\""},
    };
    for (size_t i=0; i<sizeof(ble_cmds)/sizeof(ble_cmds[0]); i++)
        at_server_register_command(&ble_cmds[i]);
    ESP_LOGI(TAG, "All AT BLE commands registered.");
}
