#include "ble_custom.h"
#include "wifi_custom.h"

#define MIN_REQUIRED_MBUF         2 /* Assuming payload of 500Bytes and each mbuf can take 292Bytes.  */

static const char *manuf_name = "Apache Mynewt ESP32 devkitC";
static const char *model_num = "Mynewt HR Sensor demo";
uint16_t hrs_hrm_handle;

static uint16_t confirmation_handle;

static uint16_t conn_handle_global = 0;

static const char *TAG = "ESP32-C6 GATT";

char *buff = NULL;
int _ssid = 0;
int _password = 0;

static int
gatt_svr_chr_access_heart_rate(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg);

static int
gatt_svr_chr_access_device_info(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg);

static int
gatt_svr_chr_wifi_credentials(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
    /* Service: get wifi credentials */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(GATT_WIFI_CREDENTIALS_UUID),
        .characteristics = (struct ble_gatt_chr_def[])
        { {
                /* Characteristic: SSID */
                .uuid = BLE_UUID16_DECLARE(GATT_WIFI_SSID),
                .access_cb = gatt_svr_chr_wifi_credentials,
                .flags = BLE_GATT_CHR_F_WRITE,
            },{
                /* Characteristic: Password */
                .uuid = BLE_UUID16_DECLARE(GATT_WIFI_PASSWORD),
                .access_cb = gatt_svr_chr_wifi_credentials,
                .flags = BLE_GATT_CHR_F_WRITE,
            },{
                /* Characteristic: Confirmation */
                .uuid = BLE_UUID16_DECLARE(GATT_WIFI_CONFIRMATION),
                .access_cb = gatt_svr_chr_wifi_credentials,
                .val_handle = &confirmation_handle,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            },{
                0, /* No more characteristics in this service */
            },
        }
    },
    {
        0, /* No more services */
    },
};



static int
gatt_svr_chr_wifi_credentials(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t uuid, len;
    int rc;

    conn_handle_global = conn_handle;

    uuid = ble_uuid_u16(ctxt->chr->uuid);
    len = ctxt->om->om_len;

    if (uuid == GATT_WIFI_CONFIRMATION)
    {
        confirmation_handle = attr_handle;
        printf("Confirmation handle: %d\n", confirmation_handle);
    }

    buff = malloc(sizeof(char) * (len + 1));

    rc = os_mbuf_append(ctxt->om, ctxt->om->om_data, len);
    if (rc != 0) {
        free(buff);
        return BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    strncpy(buff, (char *)ctxt->om->om_data, len);
    buff[len] = '\0';

    if (uuid == GATT_WIFI_SSID)
    {
        strcpy(wifiCredentials.ssid, buff);
        _ssid = 1;
        free(buff);
    }
    else if (uuid == GATT_WIFI_PASSWORD)
    {
        strcpy(wifiCredentials.password, buff);
        _password = 1;
        free(buff);
    }

    if (_ssid && _password)
    {
        printf("SSID: %s\n", wifiCredentials.ssid);
        printf("Password: %s\n", wifiCredentials.password);
        _ssid = 0;
        _password = 0;

        xSemaphoreGive(wifiCredentialsSemaphore);
    }

    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

    /*assert(0);
    return BLE_ATT_ERR_UNLIKELY;*/
}

static int
gatt_svr_chr_access_heart_rate(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    /* Sensor location, set to "Chest" */
    static uint8_t body_sens_loc = 0x01;
    uint16_t uuid;
    int rc;

    uuid = ble_uuid_u16(ctxt->chr->uuid);

    if (uuid == GATT_HRS_BODY_SENSOR_LOC_UUID) {
        rc = os_mbuf_append(ctxt->om, &body_sens_loc, sizeof(body_sens_loc));

        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

static int
gatt_svr_chr_access_device_info(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t uuid;
    int rc;

    uuid = ble_uuid_u16(ctxt->chr->uuid);

    if (uuid == GATT_MODEL_NUMBER_UUID) {
        rc = os_mbuf_append(ctxt->om, model_num, strlen(model_num));
        printf("Received from client: %s\n", model_num);
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (uuid == GATT_MANUFACTURER_NAME_UUID) {
        rc = os_mbuf_append(ctxt->om, manuf_name, strlen(manuf_name));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

// Function that I can call from main.c to send confirmation to client when wifi is connected when I write in a characteristic
void send_confirmation() {
    // Check if we have a valid connection handle
    if (conn_handle_global == 0) {
        ESP_LOGE(TAG, "No valid connection handle for sending confirmation");
        return;
    }

    // Prepare the data to send
    static uint8_t confirmation = 1; // Payload for the notification
    struct os_mbuf *om;
    int rc;

    // Allocate a mbuf for the payload
    om = ble_hs_mbuf_from_flat(&confirmation, sizeof(confirmation));
    if (om == NULL) {
        ESP_LOGE(TAG, "Failed to allocate mbuf for notification");
        return;
    }

    // Send the notification
    rc = ble_gatts_notify_custom(conn_handle_global, confirmation_handle, om);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error while sending notification; rc = %d", rc);
    } else {
        ESP_LOGI(TAG, "Confirmation sent to client");
    }

    // Free the mbuf after sending
    os_mbuf_free_chain(om);
}


void
gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        MODLOG_DFLT(DEBUG, "registered service %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                    ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        MODLOG_DFLT(DEBUG, "registering characteristic %s with "
                    "def_handle=%d val_handle=%d\n",
                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                    ctxt->chr.def_handle,
                    ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        MODLOG_DFLT(DEBUG, "registering descriptor %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                    ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}


int
gatt_svr_init(void)
{
    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}