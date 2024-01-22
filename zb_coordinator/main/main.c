#include <stdio.h>
#include <time.h>
#include "ble_custom.h"
#include "wifi_custom.h"
#include "zb_custom.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "Main: ESP32-C6";

int wifi_status = 0;

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

    wifi_init();
    init_ble();
    int i = wifi_connect();
    if (i == 1)
    {
        ESP_LOGI(TAG, "Connected to AP");
    }
    else
    {
        ESP_LOGI(TAG, "Error connecting to AP");
    }
    //init_wifi();
    //wifi_init_sta();
    //app_main_zb();

    while(1)
    {
        //ESP_LOGI(TAG, "lasdlk");
        //vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    // Additional application code goes here
}
