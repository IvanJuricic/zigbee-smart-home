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

static const char *TAG = "Main: ESP32-C6";
int wifi_status = 0;

void bluetooth_task(void *pvParameter) {
    // Initialize and handle Bluetooth
    ESP_LOGI(TAG, "Starting Bluetooth");
    init_ble();
    ESP_LOGI(TAG, "Bluetooth initialized");
    for(;;) {
        xSemaphoreTake(confirmationSemaphore, portMAX_DELAY);
        send_confirmation("CONNECTED");
    }
}

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
    for(;;) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
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

    init_semaphores();
    
    ESP_LOGI(TAG, "Starting Zigbee");
    esp_zb_platform_config_t config = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_ZB_DEFAULT_HOST_CONFIG(),
    };
    //ESP_ERROR_CHECK(nvs_flash_init());
    /* load Zigbee switch platform config to initialization */
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));
    /* hardware related and device init */
    //switch_driver_init(button_func_pair, PAIR_SIZE(button_func_pair), esp_zb_buttons_handler);
    xTaskCreate(&esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);
    //xTaskCreate(&memory_monitor_task, "memory_monitor_task", 2048, NULL, 1, NULL);
    xTaskCreate(&wifi_task, "wifi_mqtt_task", 4096, NULL, 5, NULL);
    xTaskCreate(&mqtt_init_task, "mqtt_task", 4096, NULL, 4, NULL);
    xTaskCreate(&bluetooth_task, "bluetooth_task", 4096, NULL, 4, NULL);
    //xTaskCreate(&mqtt_publish_task, "bluetooth_task", 2048, NULL, 4, NULL);
    // Additional application code goes here
}


//##############################################################################################################

