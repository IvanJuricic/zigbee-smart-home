#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "esp_zb_switch.h"
#include "nvs_flash.h"
#include <string.h>

#include "driver/uart.h"
#include "driver/gpio.h"

#define TXD_PIN (GPIO_NUM_16)
#define RXD_PIN (GPIO_NUM_17)
#define BUF_SIZE (1024)

/**
 * @note Make sure set idf.py menuconfig in zigbee component as zigbee coordinator device!
*/
#if defined ZB_ED_ROLE
#error Define ZB_COORDINATOR_ROLE in idf.py menuconfig to compile light switch source code.
#endif

/* define a single remote device struct for managing */
typedef struct light_bulb_device_params_s {
    esp_zb_ieee_addr_t ieee_addr;
    uint8_t  endpoint;
    uint16_t short_addr;
} light_bulb_device_params_t;

/* define Button function currently only 1 switch define */
static switch_func_pair_t button_func_pair[] = {
    {GPIO_INPUT_IO_TOGGLE_SWITCH, SWITCH_ONOFF_TOGGLE_CONTROL}
};

static const char *TAG = "ESP_ZB_ON_OFF_SWITCH";
/* remote device struct for recording and managing node info */
light_bulb_device_params_t on_off_light[5];
int num_devices = 0;

uint8_t data[BUF_SIZE];
/********************* Define functions **************************/

/**
 * @brief Callback for button events, currently only toggle event available
 *
 * @param button_func_pair      Incoming event from the button_pair.
 */
static void esp_zb_buttons_handler(switch_func_pair_t *button_func_pair)
{
    if (button_func_pair->func == SWITCH_ONOFF_TOGGLE_CONTROL) {
        /* implemented light switch toggle functionality */
        esp_zb_zcl_on_off_cmd_t cmd_req;
        ESP_EARLY_LOGI(TAG, "send 'on_off toggle' command");
        for(int i = 0; i < num_devices; i++) {
            cmd_req.zcl_basic_cmd.dst_addr_u.addr_short = on_off_light[i].short_addr;
            cmd_req.zcl_basic_cmd.dst_endpoint = on_off_light[i].endpoint;
            cmd_req.zcl_basic_cmd.src_endpoint = HA_ONOFF_SWITCH_ENDPOINT;
            cmd_req.address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
            cmd_req.on_off_cmd_id = ESP_ZB_ZCL_CMD_ON_OFF_TOGGLE_ID;
            esp_zb_zcl_on_off_cmd_req(&cmd_req);
        }
        
    }
}

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    ESP_ERROR_CHECK(esp_zb_bdb_start_top_level_commissioning(mode_mask));
}

// Function to check for duplicates in on_off_light array
static int check_for_duplicates(light_bulb_device_params_t *arr, int n) {
    int i, j;
    for (i = 0; i < n; i++) {
        for (j = i+1; j < n; j++) {
            if (!memcmp(arr[i].ieee_addr, arr[j].ieee_addr, sizeof(esp_zb_ieee_addr_t))) {
                return 1;
            }
        }
    }
    return 0;
}

void user_find_cb(esp_zb_zdp_status_t zdo_status, uint16_t addr, uint8_t endpoint, void *user_ctx)
{
    ESP_LOGI(TAG, "User find cb: response_status:%d, address:0x%x, endpoint:%d", zdo_status, addr, endpoint);
    if (zdo_status == ESP_ZB_ZDP_STATUS_SUCCESS) {
        // Check for duplicates
        if (check_for_duplicates(on_off_light, num_devices)) {
            ESP_LOGI(TAG, "Duplicate found, not adding to array");
            return;
        }
        on_off_light[num_devices].endpoint = endpoint;
        on_off_light[num_devices].short_addr = addr;
        num_devices++;
    }
}

void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    uint32_t *p_sg_p       = signal_struct->p_app_signal;
    esp_err_t err_status = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type = *p_sg_p;
    esp_zb_zdo_signal_device_annce_params_t *dev_annce_params = NULL;
    switch (sig_type) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        ESP_LOGI(TAG, "Zigbee stack initialized");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (err_status == ESP_OK) {
            ESP_LOGI(TAG, "Start network formation");
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_FORMATION);
        } else {
            ESP_LOGE(TAG, "Failed to initialize Zigbee stack (status: %s)", esp_err_to_name(err_status));
        }
        break;
    case ESP_ZB_BDB_SIGNAL_FORMATION:
        if (err_status == ESP_OK) {
            esp_zb_ieee_addr_t extended_pan_id;
            esp_zb_get_extended_pan_id(extended_pan_id);
            ESP_LOGI(TAG, "Formed network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d)",
                     extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                     extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
                     esp_zb_get_pan_id(), esp_zb_get_current_channel());
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
        } else {
            ESP_LOGI(TAG, "Restart network formation (status: %s)", esp_err_to_name(err_status));
            esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_FORMATION, 1000);
        }
        break;
    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK) {
            ESP_LOGI(TAG, "Network steering started");
        }
        break;
    case ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        dev_annce_params = (esp_zb_zdo_signal_device_annce_params_t *)esp_zb_app_signal_get_params(p_sg_p);
        ESP_LOGI(TAG, "New device commissioned or rejoined (short: 0x%04hx)", dev_annce_params->device_short_addr);
        esp_zb_zdo_match_desc_req_param_t  cmd_req;
        cmd_req.dst_nwk_addr = dev_annce_params->device_short_addr;
        cmd_req.addr_of_interest = dev_annce_params->device_short_addr;
        esp_zb_zdo_find_on_off_light(&cmd_req, user_find_cb, NULL);
        break;
    default:
        ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type,
                 esp_err_to_name(err_status));
        break;
    }
}

static void esp_zb_task(void *pvParameters)
{
    /* initialize Zigbee stack with Zigbee coordinator config */
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZC_CONFIG();
    ESP_LOGI(TAG, "Initializing esp_zigbee");
    esp_zb_init(&zb_nwk_cfg);
    /* set the on-off switch device config */
    esp_zb_on_off_switch_cfg_t switch_cfg = ESP_ZB_DEFAULT_ON_OFF_SWITCH_CONFIG();
    esp_zb_ep_list_t *esp_zb_on_off_switch_ep = esp_zb_on_off_switch_ep_create(HA_ONOFF_SWITCH_ENDPOINT, &switch_cfg);
    esp_zb_device_register(esp_zb_on_off_switch_ep);

    esp_zb_set_primary_network_channel_set(ESP_ZB_PRIMARY_CHANNEL_MASK);
    ESP_LOGI(TAG, "Register zigbee app");
    ESP_ERROR_CHECK(esp_zb_start(false));
    ESP_LOGI(TAG, "Zigbee stack initialized");
    esp_zb_main_loop_iteration();
}

static void toggle_lights() {
    esp_zb_zcl_on_off_cmd_t cmd_req;
    ESP_EARLY_LOGI(TAG, "send 'on_off toggle' command");
    for(int i = 0; i < num_devices; i++) {
        cmd_req.zcl_basic_cmd.dst_addr_u.addr_short = on_off_light[i].short_addr;
        cmd_req.zcl_basic_cmd.dst_endpoint = on_off_light[i].endpoint;
        cmd_req.zcl_basic_cmd.src_endpoint = HA_ONOFF_SWITCH_ENDPOINT;
        cmd_req.address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
        cmd_req.on_off_cmd_id = ESP_ZB_ZCL_CMD_ON_OFF_TOGGLE_ID;
        esp_zb_zcl_on_off_cmd_req(&cmd_req);
    }
}

void app_main_zb(void)
{

    //xTaskCreate(esp_check_toggle_req, "Check_toggle_request", 4096, NULL, 4, NULL);
}

#include <stdio.h>
#include <time.h>
#include "ble_custom.h"
#include "wifi_custom.h"
#include "zb_custom.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <inttypes.h>
#include "mqtt_custom.h"

static void memory_monitor_task(void *pvParameter) {
    while (1) {
        printf("Free heap size: %ld bytes", esp_get_free_heap_size());
        vTaskDelay(pdMS_TO_TICKS(10000)); // Delay for 10 seconds
    }
}

//static const char *TAG = "Main: ESP32-C6";
int wifi_status = 0;

void bluetooth_task(void *pvParameter) {
    // Initialize and handle Bluetooth
    ESP_LOGI(TAG, "Starting Bluetooth");
    init_ble();
    ESP_LOGI(TAG, "Bluetooth initialized");
    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/*void mqtt_publish_task(void *pvParameter) {
    while (1) {
        // Wait for MQTT to be ready
        xSemaphoreTake(mqttPublishSemaphore, portMAX_DELAY);

        // Check Wi-Fi status before publishing
        if (get_wifi_status() == 1) {  // Assuming 1 means connected
            ESP_LOGI(TAG, "Publishing data to MQTT");
            mqtt_publish_data("test/topic", "mbrale");
            ESP_LOGI(TAG, "Data published to MQTT");
        } else {
            ESP_LOGI(TAG, "Waiting for Wi-Fi to reconnect");
            vTaskDelay(pdMS_TO_TICKS(1000)); // Wait before checking Wi-Fi status again
            continue; // Skip the publishing if Wi-Fi is not connected
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay between publishing attempts
    }
}*/

void mqtt_init_task(void *pvParameter) {
    // Initialize and handle MQTT
    for(;;) {
        if (get_wifi_status() == 0) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        xSemaphoreTake(mqttStartSemaphore, portMAX_DELAY);
        ESP_LOGI(TAG, "Starting MQTT");
        mqtt_app_start();
        ESP_LOGI(TAG, "MQTT initialized");
    }
}

void wifi_task(void *pvParameter) {
    // Initialize WiFi and connect to MQTT broker
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    ESP_LOGI(TAG, "Starting WiFi");
    
    example_wifi_start();

    ESP_LOGI(TAG, "WIFI started");
    for(;;) {
        // Waiting for new config
        wifi_set_config();
        //vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void
app_main()
{
    // Initialize NVS 
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    init_credentials_semaphore();
    
    ESP_LOGI(TAG, "Starting Zigbee");
    esp_zb_platform_config_t config = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_ZB_DEFAULT_HOST_CONFIG(),
    };
    //ESP_ERROR_CHECK(nvs_flash_init());
    /* load Zigbee switch platform config to initialization */
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));
    /* hardware related and device init */
    switch_driver_init(button_func_pair, PAIR_SIZE(button_func_pair), esp_zb_buttons_handler);
    //xTaskCreate(&esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);

    //xTaskCreate(&memory_monitor_task, "memory_monitor_task", 2048, NULL, 1, NULL);
    xTaskCreate(&wifi_task, "wifi_mqtt_task", 4096, NULL, 5, NULL);
    xTaskCreate(&mqtt_init_task, "mqtt_task", 4096, NULL, 4, NULL);
    xTaskCreate(&bluetooth_task, "bluetooth_task", 2048, NULL, 4, NULL);
    //xTaskCreate(&mqtt_publish_task, "bluetooth_task", 2048, NULL, 4, NULL);
    // Additional application code goes here
}


//##############################################################################################################

