#include "main.h"

static const char *TAG = "Zigbee-Wifi Gateway";

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    int wifi_status = 0;

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_status = wifi_init_sta();
    if(wifi_status) {
        xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);
    } else if(wifi_status == -1) {
        ESP_LOGI(TAG, "Error connecting to wifi!\n");
    }
}

void tcp_client_task(void *pvParameters) {

    for(;;) {
        ESP_LOGI(TAG, "TCP task!\n");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }   
}