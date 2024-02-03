#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOSConfig.h"
/* BLE */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "console/console.h"
#include "services/gap/ble_svc_gap.h"

#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "nimble/ble.h"
#include "modlog/modlog.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Heart-rate configuration */
#define GATT_HRS_UUID                           0x180D
#define GATT_HRS_MEASUREMENT_UUID               0x2A37
#define GATT_HRS_BODY_SENSOR_LOC_UUID           0x2A38
#define GATT_DEVICE_INFO_UUID                   0x180A
#define GATT_MANUFACTURER_NAME_UUID             0x2A29
#define GATT_MODEL_NUMBER_UUID                  0x2A24

#define GATT_WIFI_UUID                          0x2B36
#define GATT_WIFI_SSID                          0x3B40
#define GATT_WIFI_PASSWORD                      0x4240
#define GATT_WIFI_CONFIRMATION                  0x4840
#define GATT_WIFI_AP_LIST                       0x5040
#define GATT_WIFI_DISCONNECT                    0x5110
#define GATT_WIFI_GET_STATUS                    0x5210
#define GATT_TOGGLE_LIGHT                       0x6010

extern uint16_t hrs_hrm_handle;
extern uint16_t conn_handle_global;

struct ble_hs_cfg;
struct ble_gatt_register_ctxt;

void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
int gatt_svr_init(void);
void init_ble();
void blehr_host_task(void *param);
void send_confirmation(char *confirmation);
void send_notification(uint8_t *data, size_t data_len);