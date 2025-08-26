// at_ble.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_err.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "at_server.h"

#define TAG "AT_BLE"
#define BLE_MAX_UUID16 8

static bool ble_inited = false;
static char g_current_name[32] = "DemoESP";
static uint8_t g_manu_data[32];
static uint8_t g_manu_data_len = 0;

static struct ble_gatt_svc_def *gatt_svcs = NULL;

// Demo: 1 service, 2 characs (battery style)
static uint8_t demo_value1[4] = { 0x42, 0x13, 0x37, 0x21 };
static uint8_t demo_value2[1] = { 0xAA };

static int gatt_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        os_mbuf_append(ctxt->om, demo_value1, sizeof(demo_value1));
        ESP_LOGI(TAG, "GATT: READ char1 0x2A19 (Battery Level)");
        return 0;
    } else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        int len = OS_MBUF_PKTLEN(ctxt->om);
        if (len > sizeof(demo_value1)) len = sizeof(demo_value1);
        os_mbuf_copydata(ctxt->om, 0, len, demo_value1);
        ESP_LOGI(TAG, "GATT: WRITE char1 (len=%d)", len);
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}
static int gatt_chr2_access_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        os_mbuf_append(ctxt->om, demo_value2, sizeof(demo_value2));
        ESP_LOGI(TAG, "GATT: READ char2 0x2A1B");
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static struct ble_gatt_svc_def demo_gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(0x180F),
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID16_DECLARE(0x2A19),
                .access_cb = gatt_chr_access_cb,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            },
            {
                .uuid = BLE_UUID16_DECLARE(0x2A1B),
                .access_cb = gatt_chr2_access_cb,
                .flags = BLE_GATT_CHR_F_READ,
            },
            { 0 } // end
        }
    },
    { 0 } // end
};

// ----------- BLE Stack Init/Deinit ------------

static void ble_host_task(void *param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void ble_gap_on_sync(void) {
    ESP_LOGI(TAG, "NimBLE synced! Ready.");
}

static void at_ble_init_stack(void) {
    if (ble_inited) return;
    ESP_ERROR_CHECK(nimble_port_init());
    ble_hs_cfg.sync_cb = ble_gap_on_sync;
    ble_svc_gap_init();
    ble_svc_gatt_init();

    ESP_ERROR_CHECK(ble_gatts_count_cfg(demo_gatt_svcs));
    ESP_ERROR_CHECK(ble_gatts_add_svcs(demo_gatt_svcs));

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

// ----------- ADV, STOP, RENAME, etc ------------

static void start_ble_advertising(void) {
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
    ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, NULL, NULL);
    ESP_LOGI(TAG, "BLE Advertising started! Name: %s", g_current_name);
}

// --------------- AT COMMANDS -----------------

void at_handle_blebegin(const char *params) {
    // Format: "name";"manu_ascii"
    char name[32] = "DemoESP";
    char manu_ascii[32] = "";
    sscanf(params, "\"%31[^\"]\";\"%31[^\"]\"", name, manu_ascii);
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
}

void at_handle_blemfg(const char *params) {
    // Accept direct ASCII string!
    char manu_ascii[32] = "";
    sscanf(params, "\"%31[^\"]\"", manu_ascii);

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
}

void at_handle_bleadvstop(const char *params) {
    ble_gap_adv_stop();
    ESP_LOGI(TAG, "BLE advertising stopped.");
}

void at_handle_bleadvstart(const char *params) {
    start_ble_advertising();
}

void at_handle_blestop(const char *params) {
    at_ble_deinit_stack();
    ESP_LOGI(TAG, "BLE stack stopped.");
}

void at_handle_bleclear(const char *params) {
    at_ble_deinit_stack();
    ESP_LOGI(TAG, "Conf BLE cleared/reset.");
}

void at_handle_blesetname(const char *params) {
    char name[32]="";
    sscanf(params, "\"%31[^\"]\"", name);
    strncpy(g_current_name, name, sizeof(g_current_name)-1);
    g_current_name[sizeof(g_current_name)-1]=0;
    start_ble_advertising();
    ESP_LOGI(TAG, "BLE name set to: %s", g_current_name);
}

// ----------- BLE SCAN (scan d'autres périphériques) -----------

static int ble_scan_cb(struct ble_gap_event *event, void *arg) {
    if (event->type == BLE_GAP_EVENT_DISC) {
        char addr_str[18] = {0};
        sprintf(addr_str, "%02X:%02X:%02X:%02X:%02X:%02X",
                event->disc.addr.val[5], event->disc.addr.val[4], event->disc.addr.val[3],
                event->disc.addr.val[2], event->disc.addr.val[1], event->disc.addr.val[0]);
        ESP_LOGI(TAG, "Found BLE: %s (RSSI %d)", addr_str, event->disc.rssi);
    }
    return 0;
}
void at_handle_blescan(const char *params) {
    int duration = 5;
    if (params && *params) sscanf(params, "%d", &duration);
    struct ble_gap_disc_params scan_params = {0};
    scan_params.passive = 1;
    scan_params.itvl = 0x0010;
    scan_params.window = 0x0010;
    scan_params.filter_duplicates = 1;
    ble_gap_disc(BLE_OWN_ADDR_PUBLIC, duration * 1000, &scan_params, ble_scan_cb, NULL);
    ESP_LOGI(TAG, "BLE scan started for %ds...", duration);
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
        {"AT+BLEMFG",     at_handle_blemfg,     "Set Manufacturer Data: AT+BLEMFG=\"01020304\""},
        {"AT+BLESCAN",    at_handle_blescan,    "Scan for BLE devices: AT+BLESCAN=secs"},
    };
    for (size_t i=0; i<sizeof(ble_cmds)/sizeof(ble_cmds[0]); i++)
        at_server_register_command(&ble_cmds[i]);
    ESP_LOGI(TAG, "All AT BLE commands registered.");
}

void at_ble_test_all(void)
{
    printf("BLE FAT TEST...\n");

    printf("STEP 1: BLEBEGIN (TestAT/HELLO123)\n");
    at_handle_blebegin("\"TestAT\";\"HELLO123\"");
    vTaskDelay(pdMS_TO_TICKS(30000)); // 8s pour scan initial

    printf("STEP 2: SETNAME (Test2)\n");
    at_handle_blesetname("\"Test2\"");
    vTaskDelay(pdMS_TO_TICKS(30000)); // 5s

    printf("STEP 3: SET MANUFACTURER (WORLD456)\n");
    at_handle_blemfg("\"WORLD456\"");
    vTaskDelay(pdMS_TO_TICKS(30000)); // 5s

    printf("STEP 4: ADV STOP\n");
    at_handle_bleadvstop(NULL);
    vTaskDelay(pdMS_TO_TICKS(3000)); // 3s

    printf("STEP 5: ADV RESTART\n");
    at_handle_bleadvstart(NULL);
    vTaskDelay(pdMS_TO_TICKS(5000)); // 5s

    printf("STEP 6: BLE SCAN (3s)\n");
    at_handle_blescan("3");
    vTaskDelay(pdMS_TO_TICKS(8000)); // 8s

    printf("STEP 7: BLE STOP\n");
    at_handle_blestop(NULL);
    vTaskDelay(pdMS_TO_TICKS(2000));

    printf("BLE FAT TEST DONE!\n");
}
