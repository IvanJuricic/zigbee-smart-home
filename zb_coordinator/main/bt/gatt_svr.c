#include "wifi_custom.h"
#include "ble_custom.h"
#include "zb_custom.h"

#include "modlog/modlog.h"

#define MIN_REQUIRED_MBUF         2 /* Assuming payload of 500Bytes and each mbuf can take 292Bytes.  */

static const char *manuf_name = "Apache Mynewt ESP32 devkitC";
static const char *model_num = "Mynewt HR Sensor demo";
uint16_t hrs_hrm_handle;

static uint16_t confirmation_handle, wifi_ap_list_handle, wifi_disconnect_handle, wifi_status_handle;

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

static int
gatt_svr_chr_get_wifi_access_points(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg);

static int
gatt_svr_chr_disconnect_wifi(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg);

static int
gatt_svr_chr_get_wifi_status(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg);

static int
gatt_svr_chr_toggle_light(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg);

static const 
struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
    /* Service: Wifi control */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(GATT_WIFI_UUID),
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
                .access_cb = gatt_svr_chr_access_heart_rate,
                .val_handle = &confirmation_handle,
                .flags = BLE_GATT_CHR_F_NOTIFY,
            },{
                /* Characteristic for sending list of WIFI access points */
                .uuid = BLE_UUID16_DECLARE(GATT_WIFI_AP_LIST),
                .access_cb = gatt_svr_chr_get_wifi_access_points,
                .val_handle = &wifi_ap_list_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            },{
                /* Characteristic for disconnecting from Wifi */
                .uuid = BLE_UUID16_DECLARE(GATT_WIFI_DISCONNECT),
                .access_cb = gatt_svr_chr_disconnect_wifi,
                .val_handle = &wifi_disconnect_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            },{
                /* Characteristic for sending list of WIFI access points */
                .uuid = BLE_UUID16_DECLARE(GATT_WIFI_GET_STATUS),
                .access_cb = gatt_svr_chr_disconnect_wifi,
                .val_handle = &wifi_status_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            },{
                /* Characteristic: Toggle light */
                .uuid = BLE_UUID16_DECLARE(GATT_TOGGLE_LIGHT),
                .access_cb = gatt_svr_chr_toggle_light,
                .flags = BLE_GATT_CHR_F_WRITE,
            },{
                0, /* No more characteristics in this service */
            },
        }
    },
    {
        0, /* No more services */
    },
};

static int gatt_svr_chr_get_wifi_access_points(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    wifi_ap_record_t *ap_list = NULL;
    uint8_t ap_num = 10;

    // Check if we have a valid connection handle
    if (conn_handle_global == 0) {
        ESP_LOGE(TAG, "No valid connection handle for sending notification");
        return 0;
    }

    int ret = wifi_get_ap_list(&ap_list);
    if (ret == 0) {
        ESP_LOGE(TAG, "Error while scanning for wifi access points");
        return 0;
    } else if (ret == 1) {
        ESP_LOGI(TAG, "Successfully scanned for wifi access points");
        if (ap_list != NULL) {
            // Extract only the SSIDs from the access point list
            char *ap_list_ssid[ap_num];
            int len, total_len = 0;
            for (int i = 0; i < ap_num; i++) {
                int len = strlen((char *)ap_list[i].ssid) + 1;
                total_len += len;
                ap_list_ssid[i] = malloc(sizeof(char) * len);
                strcpy(ap_list_ssid[i], (char *)ap_list[i].ssid);
            }
            // Print the SSIDs
            for (int i = 0; i < ap_num; i++) {
                ESP_LOGI(TAG, "SSID: %s", ap_list[i].ssid);
                os_mbuf_append(ctxt->om, ap_list[i].ssid, strlen((char *)ap_list[i].ssid));
                char delimiter = '\n';
                os_mbuf_append(ctxt->om, &delimiter, 1); // Append delimiter
            }

            for (int i = 0; i < ap_num; i++) {
                free(ap_list_ssid[i]);
            }
        } else {
            ESP_LOGE(TAG, "No access points found");
            return 0;
        }
    }
    
    free(ap_list);
    return 0;
}


static int gatt_svr_chr_get_wifi_status(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    // Check if we have a valid connection handle
    if (conn_handle_global == 0) {
        ESP_LOGE(TAG, "No valid connection handle for sending notification");
        return 0;
    }

    ESP_LOGI(TAG, "Sending wifi status to client");
    // Send confirmation to client
    if (get_wifi_status()) {
        send_confirmation("CONNECTED");
    } else {
        send_confirmation("DISCONNECTED");
    }

    return 0;

}

static int gatt_svr_chr_disconnect_wifi(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    // Check if we have a valid connection handle
    if (conn_handle_global == 0) {
        ESP_LOGE(TAG, "No valid connection handle for sending notification");
        return 0;
    }

    ESP_LOGI(TAG, "Disconnecting from wifi");
    // Disconnect from wifi
    //wifi_disconnect();
    ESP_ERROR_CHECK(example_wifi_sta_do_disconnect());

    // Send confirmation to client
    send_confirmation("DISCONNECTED");

    return 0;
}


static int
gatt_svr_chr_toggle_light(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    printf("AAAAAAAAAAAAAAAA");
    // Check uuid and toggle light using function from app_main_zb.c
    uint16_t uuid, len;
    int rc;

    uuid = ble_uuid_u16(ctxt->chr->uuid);
    len = ctxt->om->om_len;

    buff = malloc(sizeof(char) * (len + 1));
    if (uuid == GATT_TOGGLE_LIGHT)
    {
        toggle_lights();
    }

    // Send confirmation to client
    send_confirmation("LIGHT TOGGLED");

    return 0;
}

static int
gatt_svr_chr_wifi_credentials(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t uuid, len;
    int rc;

    uuid = ble_uuid_u16(ctxt->chr->uuid);
    len = ctxt->om->om_len;

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
        //wifi_set_config();
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
void send_confirmation(char *confirmation) {
    ESP_LOGI(TAG, "Sending confirmation to client");
    // Check if we have a valid connection handle
    if (conn_handle_global == 0) {
        ESP_LOGE(TAG, "No valid connection handle for sending confirmation");
        return;
    }

    char confirmation_copy[20] = {0};

    strcpy(confirmation_copy, confirmation);

    ESP_LOGI(TAG, "Confirmation: %s", confirmation_copy);

    // Prepare the data to send
    //static uint8_t confirmation = 1; // Payload for the notification
    struct os_mbuf *om;
    int rc;

    // Allocate a mbuf for the payload
    om = ble_hs_mbuf_from_flat(&confirmation_copy, strlen(confirmation_copy));
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
    //os_mbuf_free_chain(om);
}

void
gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];
/*
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
*/
    
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