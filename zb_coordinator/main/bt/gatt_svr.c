#include "ble_custom.h"
#include "wifi_custom.h"

static const char *manuf_name = "Apache Mynewt ESP32 devkitC";
static const char *model_num = "Mynewt HR Sensor demo";
uint16_t hrs_hrm_handle;

char *ssid = NULL, *password = NULL;
int creds = 0;

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

    uuid = ble_uuid_u16(ctxt->chr->uuid);
    len = ctxt->om->om_len;

    if (uuid == GATT_WIFI_SSID) {
        ssid = malloc(sizeof(char) * (len + 1));
        rc = os_mbuf_append(ctxt->om, ctxt->om->om_data, len);
        if (rc != 0) {
            free(ssid);
            free(password);
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }

        strncpy(ssid, (char *)ctxt->om->om_data, len);
        ssid[len] = '\0';
        
        strcpy(wifiCredentials.ssid, ssid);
        printf("SSID: %s\n", wifiCredentials.ssid);

        free(ssid);

        creds++;
        if(creds == 2) {
            printf("postavljena oba\n");
            xSemaphoreGive(wifiCredentialsSemaphore);
        }
        
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

    } else if (uuid == GATT_WIFI_PASSWORD) {

        password = malloc(sizeof(char) * (len + 1));
        rc = os_mbuf_append(ctxt->om, ctxt->om->om_data, len);
        if (rc != 0) {
            free(ssid);
            free(password);
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }

        strncpy(password, (char *)ctxt->om->om_data, len);
        password[len] = '\0';

        strcpy(wifiCredentials.password, password);
        printf("Password: %s\n", wifiCredentials.password);

        free(password);

        creds++;
        if(creds == 2) {
            printf("postavljena oba\n");
            xSemaphoreGive(wifiCredentialsSemaphore);
        }

        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
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