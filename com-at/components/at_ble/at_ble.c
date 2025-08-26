#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "at_server.h"

#if CONFIG_IDF_TARGET_ESP32C6 || CONFIG_IDF_TARGET_ESP32H2 || CONFIG_IDF_TARGET_ESP32C5
#include "esp_nimble_hci.h"
#endif

#define AT_BLE_MAX_SVC   4
#define AT_BLE_MAX_CHAR  8

#define AT_BLE_CHR_F_READ   0x01
#define AT_BLE_CHR_F_WRITE  0x02
#define AT_BLE_CHR_F_NOTIFY 0x04

typedef struct {
    uint16_t uuid;
    uint8_t flags;  // bitmask: 1=READ, 2=WRITE, 4=NOTIFY
    uint8_t value[32];
    uint8_t value_len;
} at_ble_char_t;

typedef struct {
    uint16_t uuid;
    int char_count;
    at_ble_char_t chars[AT_BLE_MAX_CHAR];
} at_ble_svc_t;

static at_ble_svc_t at_ble_svcs[AT_BLE_MAX_SVC];
static int at_ble_svc_count = 0;
static bool ble_inited = false;

static const char *TAG = "AT_BLE";

// ====== CALLBACKS GATT/NimBLE ======

static int gatt_svr_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                  struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    at_ble_char_t *chr = (at_ble_char_t*)arg;

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        os_mbuf_append(ctxt->om, chr->value, chr->value_len); // renvoie la valeur stockée
        ESP_LOGI(TAG, "Read char UUID 0x%04X", chr->uuid);
        return 0;
    } else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        int len = OS_MBUF_PKTLEN(ctxt->om);
        chr->value_len = (len > sizeof(chr->value)) ? sizeof(chr->value) : len;
        os_mbuf_copydata(ctxt->om, 0, chr->value_len, chr->value);
        ESP_LOGI(TAG, "Write char UUID 0x%04X, len=%d", chr->uuid, chr->value_len);
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

// =================== BLE INIT/DEINIT =======================

static struct ble_gatt_svc_def *gatt_svcs = NULL; // Dynamique !

static void free_gatt_svcs(void) {
    if (gatt_svcs) {
        // Libère aussi les tableaux de chars associés
        for (int i = 0; i < at_ble_svc_count; i++)
            free((void*)gatt_svcs[i].characteristics);
        free(gatt_svcs);
        gatt_svcs = NULL;
    }
}

// Appelé à chaque BLEINIT
static void build_gatt_svcs(void) {
    free_gatt_svcs();

    int total_svcs = at_ble_svc_count;
    // On alloue les gatt_svc + 1 (fin)
    gatt_svcs = calloc(total_svcs + 1, sizeof(struct ble_gatt_svc_def));
    for (int i = 0; i < at_ble_svc_count; i++) {
        at_ble_svc_t *svc = &at_ble_svcs[i];

        // Alloue chars + 1 (fin)
        struct ble_gatt_chr_def *chars = calloc(svc->char_count + 1, sizeof(struct ble_gatt_chr_def));
        for (int j = 0; j < svc->char_count; j++) {
            at_ble_char_t *chr = &svc->chars[j];
            chars[j].uuid = BLE_UUID16_DECLARE(chr->uuid);
            chars[j].access_cb = gatt_svr_chr_access_cb;
            chars[j].arg = chr;

            int flags = 0;
            if (chr->flags & AT_BLE_CHR_F_READ) flags |= BLE_GATT_CHR_F_READ;
            if (chr->flags & AT_BLE_CHR_F_WRITE) flags |= BLE_GATT_CHR_F_WRITE;
            if (chr->flags & AT_BLE_CHR_F_NOTIFY) flags |= BLE_GATT_CHR_F_NOTIFY;

            chars[j].flags = flags;
        }
        gatt_svcs[i].type = BLE_GATT_SVC_TYPE_PRIMARY;
        gatt_svcs[i].uuid = BLE_UUID16_DECLARE(svc->uuid);
        gatt_svcs[i].characteristics = chars;
    }
}

static void ble_host_task(void *param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void ble_gap_on_sync(void) {
    ESP_LOGI(TAG, "NimBLE synced! Ready.");
}

static void at_ble_init_stack(void) {
    if (ble_inited) return;
    #if CONFIG_IDF_TARGET_ESP32C6 || CONFIG_IDF_TARGET_ESP32H2 || CONFIG_IDF_TARGET_ESP32C5
        ESP_ERROR_CHECK(esp_nimble_hci_and_controller_init());
    #endif
    ESP_ERROR_CHECK(nimble_port_init());

    ble_hs_cfg.sync_cb = ble_gap_on_sync;
    ble_svc_gap_init();
    ble_svc_gatt_init();

    ESP_ERROR_CHECK(ble_gatts_count_cfg(gatt_svcs));
    ESP_ERROR_CHECK(ble_gatts_add_svcs(gatt_svcs));

    nimble_port_freertos_init(ble_host_task);
    ble_inited = true;
    ESP_LOGI(TAG, "NimBLE initialized OK!");
}

static void at_ble_deinit_stack(void) {
    if (!ble_inited) return;
    nimble_port_stop();
    nimble_port_deinit();
    #if CONFIG_IDF_TARGET_ESP32C6 || CONFIG_IDF_TARGET_ESP32H2 || CONFIG_IDF_TARGET_ESP32C5
        esp_nimble_hci_and_controller_deinit();
    #endif
    free_gatt_svcs();
    ble_inited = false;
    ESP_LOGI(TAG, "NimBLE stopped/deinit OK!");
}

// ================== ADVERTISING SIMPLE (AT+BLEADV) ===================

void at_handle_bleadv(const char *params) {
    struct ble_gap_adv_params adv_params = {0};
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;  // connectable
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;  // general discoverable

    // Advertising data : juste le nom
    struct ble_hs_adv_fields fields = {0};
    const char *adv_name = "MYESP32";
    fields.name = (uint8_t*)adv_name;
    fields.name_len = strlen(adv_name);
    fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&fields);

    int rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, NULL, NULL);

    if (rc == 0) {
        ESP_LOGI(TAG, "BLE Advertising started! Name: %s", adv_name);
    } else {
        ESP_LOGE(TAG, "Error start adv: %d", rc);
    }
}


// ================== AT COMMANDS HANDLER ===================

void at_handle_bleclear(const char *params) {
    at_ble_svc_count = 0;
    memset(at_ble_svcs, 0, sizeof(at_ble_svcs));
    at_ble_deinit_stack();
    ESP_LOGI(TAG, "Conf BLE cleared.");
}

void at_handle_bleaddsvc(const char *params) {
    if (at_ble_svc_count >= AT_BLE_MAX_SVC) return;
    uint16_t svc_uuid = 0;
    sscanf(params, "\"%4hx\"", &svc_uuid);
    at_ble_svcs[at_ble_svc_count].uuid = svc_uuid;
    at_ble_svcs[at_ble_svc_count].char_count = 0;
    ESP_LOGI(TAG, "Ajout service UUID 0x%04X idx=%d", svc_uuid, at_ble_svc_count);
    at_ble_svc_count++;
}

void at_handle_bleaddchar(const char *params) {
    int svc_idx = 0; uint16_t char_uuid = 0; int flags = 0;
    sscanf(params, "\"%d\",\"%4hx\",%d", &svc_idx, &char_uuid, &flags);
    if (svc_idx < 0 || svc_idx >= at_ble_svc_count) return;
    at_ble_svc_t *svc = &at_ble_svcs[svc_idx];
    if (svc->char_count >= AT_BLE_MAX_CHAR) return;
    svc->chars[svc->char_count].uuid = char_uuid;
    svc->chars[svc->char_count].flags = flags;
    svc->chars[svc->char_count].value_len = 1; // init à 1 octet
    svc->chars[svc->char_count].value[0] = 0x42;
    svc->char_count++;
    ESP_LOGI(TAG, "Ajout char UUID 0x%04X to svc_idx %d flags=%d", char_uuid, svc_idx, flags);
}

void at_handle_blelist(const char *params) {
    printf("--- Conf BLE ---\n");
    for (int i = 0; i < at_ble_svc_count; i++) {
        printf("Service idx=%d UUID=0x%04X (%d char):\n", i, at_ble_svcs[i].uuid, at_ble_svcs[i].char_count);
        for (int j = 0; j < at_ble_svcs[i].char_count; j++) {
            printf("  Char idx=%d UUID=0x%04X flags=0x%02X\n", j, at_ble_svcs[i].chars[j].uuid, at_ble_svcs[i].chars[j].flags);
        }
    }
}

void at_handle_bleinit(const char *params) {
    build_gatt_svcs();
    at_ble_init_stack();
}

void at_handle_bledeinit(const char *params) {
    at_ble_deinit_stack();
}

// ========== AT BLE REGISTRATION ==========

void ble_at_register(void)
{
    static const at_command_t ble_cmds[] = {
        {"AT+BLECLEAR",   at_handle_bleclear,   "Efface toute la configuration BLE"},
        {"AT+BLEADDSVC",  at_handle_bleaddsvc,  "Ajoute un service BLE: AT+BLEADDSVC=\"0x180F\""},
        {"AT+BLEADDCHAR", at_handle_bleaddchar, "Ajoute une charac.: AT+BLEADDCHAR=\"svc_idx\",\"uuid\",flags"},
        {"AT+BLELIST",    at_handle_blelist,    "Affiche la conf BLE"},
        {"AT+BLEINIT",    at_handle_bleinit,    "Lance NimBLE avec la conf courante"},
        {"AT+BLEDEINIT",  at_handle_bledeinit,  "Stoppe et désinitialise le BLE"},
        {"AT+BLEADV",     at_handle_bleadv,     "Start advertising simple: AT+BLEADV=\"name\",uuid"},
    };
    for (size_t i = 0; i < sizeof(ble_cmds)/sizeof(ble_cmds[0]); i++)
        at_server_register_command(&ble_cmds[i]);
    ESP_LOGI(TAG, "All AT BLE commands registered.");
}

void at_ble_adv_test(void)
{
    printf("TEST AT+BLEADV...\n");
    at_handle_bleadv("\"MYESP32\",0xABCD");
    vTaskDelay(pdMS_TO_TICKS(1200000));
    printf("STOP ADV (manual)\n");
    // Tu peux aussi stop adv avec ble_gap_adv_stop(); si besoin
}


// ========== TEST ==========

void at_ble_test_all(void)
{
    printf("TEST AT+BLECLEAR...\n");
    at_handle_bleclear(NULL);
    vTaskDelay(pdMS_TO_TICKS(200));

    printf("TEST AT+BLEADDSVC...\n");
    at_handle_bleaddsvc("\"180F\"");
    vTaskDelay(pdMS_TO_TICKS(100));

    printf("TEST AT+BLEADDCHAR...\n");
    at_handle_bleaddchar("\"0\",\"2A19\",3");   // idx=0 (service 0), char 2A19, READ+WRITE
    at_handle_bleaddchar("\"0\",\"2A1B\",5");   // idx=0, char 2A1B, READ+NOTIFY
    vTaskDelay(pdMS_TO_TICKS(100));

    printf("TEST AT+BLELIST...\n");
    at_handle_blelist(NULL);

    printf("TEST AT+BLEINIT...\n");
    at_handle_bleinit(NULL);
    vTaskDelay(pdMS_TO_TICKS(6000));

    at_ble_adv_test();

    printf("TEST AT+BLEDEINIT...\n");
    at_handle_bledeinit(NULL);
    vTaskDelay(pdMS_TO_TICKS(200));
    printf("DONE.\n");


}