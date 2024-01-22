/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

#include "wifi_custom.h"
#include "ble_custom.h"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

static const char *TAG = "WiFi Sta";

static int s_retry_num = 0;
static int wifi_status = 0;

WifiCredentials wifiCredentials;  // Define the global variable
SemaphoreHandle_t wifiCredentialsSemaphore;

static esp_netif_t *sta_handler = NULL;
static esp_event_handler_instance_t instance_any_id;
static esp_event_handler_instance_t instance_got_ip;

static void
event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } 
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGI(TAG,"connect to the AP fail");
            s_retry_num = 0;
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Successfully connected to WiFi network");
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


void wifi_deinit_sta(void)
{
    esp_wifi_stop();  // Stop the WiFi driver
    esp_netif_destroy(sta_handler);  // Destroy the network interface
    esp_wifi_deinit(); // Deinitialize the WiFi driver

    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &instance_any_id);
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &instance_got_ip);
    esp_event_loop_delete_default();
    vEventGroupDelete(s_wifi_event_group);  // Delete the event group

    sta_handler = NULL;
    s_wifi_event_group = NULL;
}


int wifi_get_ap_list(wifi_ap_record_t **ap_list)
{
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    //wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    *ap_list = malloc(sizeof(wifi_ap_record_t) * DEFAULT_SCAN_LIST_SIZE);
    if (*ap_list == NULL) {
        // Handle memory allocation failure
        return 0; // or an appropriate error code
    }
    uint16_t ap_count = 0;
    memset(*ap_list, 0, sizeof(wifi_ap_record_t) * DEFAULT_SCAN_LIST_SIZE);

    //ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());
    //esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    //assert(sta_netif);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_ERROR_CHECK(esp_wifi_start());
    
    esp_wifi_scan_start(NULL, true);
    
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, *ap_list));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);
    for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
        ESP_LOGI(TAG, "SSID \t\t%s", (*ap_list)[i].ssid);
        ESP_LOGI(TAG, "RSSI \t\t%d", (*ap_list)[i].rssi);
        //print_auth_mode(ap_info[i].authmode);
        /*if (ap_info[i].authmode != WIFI_AUTH_WEP) {
            print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
        }
        ESP_LOGI(TAG, "Channel \t\t%d", ap_info[i].primary);*/
    }

    ESP_ERROR_CHECK(esp_wifi_clear_ap_list());
    ESP_ERROR_CHECK(esp_wifi_scan_stop());
    ESP_ERROR_CHECK(esp_wifi_stop());

    return 1;
}

// Function for wifi initialization without starting the station, just to enable scanning and connecting
int wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    /*Initialize WiFi */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    init_credentials_semaphore();

    return 1;

    /* Start WiFi */
    //ESP_ERROR_CHECK(esp_wifi_start());
}

// Function for wifi connection without scanning
int wifi_connect()
{
    while (1)
    {
        ESP_LOGI(TAG, "Ready to configure WiFi...");
        
        xSemaphoreTake(wifiCredentialsSemaphore, portMAX_DELAY);
        
        if (strcmp(wifiCredentials.ssid, "") == 0 || strcmp(wifiCredentials.password, "") == 0)
        {
            ESP_LOGE(TAG, "WiFi credentials not set");
            continue;
        }

        ESP_ERROR_CHECK(esp_event_loop_create_default());

        esp_netif_t *sta_handler = esp_netif_create_default_wifi_sta();
        assert(sta_handler);

        /* Initialize event group */
        s_wifi_event_group = xEventGroupCreate();

        /* Register Event handler */
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                        ESP_EVENT_ANY_ID,
                        &event_handler,
                        NULL,
                        NULL));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                        IP_EVENT_STA_GOT_IP,
                        &event_handler,
                        NULL,
                        NULL));


        wifi_config_t wifi_config = {
            .sta = {
                /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
                * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
                * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
                * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
                */
                .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
                .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
                .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
            },
        };

        strcpy((char *)wifi_config.sta.ssid, wifiCredentials.ssid);
        strcpy((char *)wifi_config.sta.password, wifiCredentials.password);

        //ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI(TAG, "wifi_init_sta finished.");

        /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
        * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                pdFALSE,
                pdFALSE,
                portMAX_DELAY);

        /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
        * happened. */
        if (bits & WIFI_CONNECTED_BIT)
        {
            ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
            send_confirmation();  // Send confirmation to client
            return 1;
        }
        else if (bits & WIFI_FAIL_BIT)
        {
            ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
            wifi_deinit_sta();
            continue;
        }
        else
        {
            ESP_LOGE(TAG, "UNEXPECTED EVENT");
            continue;
        }
    }
    
}


int wifi_init_sta(void)
{
    wifi_status = 0;

    if (s_wifi_event_group == NULL) {
        s_wifi_event_group = xEventGroupCreate();
    }

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    sta_handler = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };

    strcpy((char *)wifi_config.sta.ssid, wifiCredentials.ssid);
    strcpy((char *)wifi_config.sta.password, wifiCredentials.password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
        return 1;
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
        return -1;
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        return 0;
    }

}


void init_credentials_semaphore() {
    // Create a binary semaphore
    wifiCredentialsSemaphore = xSemaphoreCreateBinary();

    if (wifiCredentialsSemaphore == NULL) {
        // Handle semaphore creation failure
        // For simplicity, printing an error message
        printf("Failed to create the semaphore.\n");
    } else {
        // Semaphore created successfully
        printf("Semaphore created successfully.\n");
    }
}


void init_wifi()
{
    init_credentials_semaphore();
    while (1)
    {
        ESP_LOGI(TAG, "Ready to configure WiFi...");
        xSemaphoreTake(wifiCredentialsSemaphore, portMAX_DELAY);
        int ret = wifi_init_sta();
        if (ret == 1)
        {
            ESP_LOGI(TAG, "Successfully connected to WiFi network");
            send_confirmation();  // Send confirmation to client
            break;  // Break the loop on successful connection
        }
        else
        {
            ESP_LOGE(TAG, "Failed to reconnect. Retrying...");
            wifi_deinit_sta();  // Properly deinitialize before attempting to reconnect
        }
    }
}