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

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr("192.168.1.10"); // Replace with your server's local IP address
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(12345); // Replace with your server port

    int sock, recv_len;
    char recv_buf[64];
    
    int recv_timeout = 5000;
    struct timeval timeout;
    timeout.tv_sec = recv_timeout / 1000;
    timeout.tv_usec = (recv_timeout % 1000) * 1000;

    ESP_LOGI(TAG, "Initializing socket connection....!\n");

    while(1) {
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (sock < 0) {
            ESP_LOGE(TAG, "Failed to create socket");
            continue;
        }

        ESP_LOGI(TAG, "Socket created!\n");

        // Set the receive timeout
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            ESP_LOGE(TAG, "Failed to set receive timeout");
            // Handle error
        }

        if (connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in)) != 0) {
            ESP_LOGE(TAG, "Socket connection failed");
            close(sock);
            continue;
        }

        ESP_LOGI(TAG, "Socket created and connected!!!\n");
        break;
    }

    while(1) {
        send(sock, INITIAL_MESSAGE, strlen(INITIAL_MESSAGE), 0);
        ESP_LOGI(TAG, "Data sent to server: %s", INITIAL_MESSAGE);
        recv_len = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
        if (recv_len > 0) {
            //recv_buf[recv_len] = '\0';
            if(strcmp(recv_buf, CONNECTED_MESSAGE) == 0) {
                ESP_LOGI(TAG, "Received from server: %s", recv_buf);
                break;
            }
        }

        ESP_LOGI(TAG, "Not connected, trying again!\n");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "Connected to server!\n");
    ESP_LOGI(TAG, "Waiting for commands!\n");

    while (1) {
        //const char *data = "Hello, server from esp32 hahahaha!";
        //send(sock, data, strlen(data), 0);
        //ESP_LOGI(TAG, "Data sent to server: %s", data);

        recv_len = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
        if (recv_len > 0) {
            recv_buf[recv_len] = '\0';
            ESP_LOGI(TAG, "Received from server: %s", recv_buf);
            if(strcmp(recv_buf, TOGGLE_MESSAGE) == 0) {
                ESP_LOGI(TAG, "Toggling light");
            }
        }

    }

    close(sock);
}