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
#include "zb_custom.h"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

static const char *TAG = "WiFi Sta";

static int s_retry_num = 0;

WifiCredentials wifiCredentials;  // Define the global variable
SemaphoreHandle_t wifiCredentialsSemaphore;

static esp_netif_t *sta_handler = NULL;
static esp_event_handler_instance_t instance_any_id;
static esp_event_handler_instance_t instance_got_ip;

static bool wifi_initialized = false;
static bool wifi_connected = false;
static bool scanning_aps = false;

static void
event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        if (scanning_aps) {
            ESP_LOGI(TAG, "WiFi already scanning");
            return;
        }
        ESP_LOGI(TAG, "Connecting to WiFi AP!!!!!");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        wifi_connected = true;
        ESP_LOGI(TAG, "Connected to AP");
        send_confirmation("CONNECTED");  // Send confirmation to client
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_connected = false;
        /*if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
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
        }*/
        ESP_LOGI(TAG,"Disconnected from AP");
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        send_confirmation("DISCONNECTED");  // Send confirmation to client
        //wifi_connect();
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

/* Disconnect from WiFi network */
void wifi_disconnect() {
    if (wifi_connected) {
        ESP_ERROR_CHECK(esp_wifi_disconnect());
        wifi_connected = false;
        ESP_LOGI(TAG, "Disconnected from WiFi");
    }
}

/* Deinitialize WiFi station */
void wifi_deinit_sta() {
    if (wifi_initialized) {
        esp_wifi_stop();
        esp_wifi_deinit();
        esp_netif_destroy(sta_handler);
        sta_handler = NULL;
        wifi_initialized = false;
        ESP_LOGI(TAG, "WiFi deinitialized");
    }
}

/* Function to get list of access points */
int wifi_get_ap_list(wifi_ap_record_t **ap_list)
{
    if (!wifi_initialized) {
        ESP_LOGE(TAG, "WiFi not initialized");
        return -1;
    }

    scanning_aps = true;

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

    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
    
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

    scanning_aps = false;

    return 1;
}

// Function for wifi initialization without starting the station, just to enable scanning and connecting
int wifi_init(void)
{
    if (!wifi_initialized) {
        ESP_ERROR_CHECK(esp_netif_init());
        sta_handler = esp_netif_create_default_wifi_sta();

        ESP_ERROR_CHECK(esp_event_loop_create_default());

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

        // Register event handlers
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

        

        wifi_initialized = true;
        ESP_LOGI(TAG, "WiFi initialized");
        
        init_credentials_semaphore();
    }

    return 1;

    /* Start WiFi */
    //ESP_ERROR_CHECK(esp_wifi_start());
}

// Function for wifi connection without scanning
int wifi_connect()
{
    ESP_LOGI(TAG, "Waiting for WiFi credentials...");
    xSemaphoreTake(wifiCredentialsSemaphore, portMAX_DELAY);
    ESP_LOGI(TAG, "Got WiFi credentials");

    if (!wifi_initialized) {
        ESP_LOGE(TAG, "WiFi not initialized");
        return -1;
    }

    if (wifi_connected) {
        ESP_LOGI(TAG, "WiFi already connected");
        return 1;
    }

    wifi_config_t wifi_config = {};
    strcpy((char *)wifi_config.sta.ssid, wifiCredentials.ssid);
    strcpy((char *)wifi_config.sta.password, wifiCredentials.password);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi started");
    ESP_LOGI(TAG, "Waiting for WiFi Event Group bits...");

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
        //send_confirmation("CONNECTED");  // Send confirmation to client
        //app_main_zb();
        return 1;
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
        //send_confirmation("DISCONNECTED");  // Send confirmation to client
        return -1;
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        return 0;
    }
    
}

int get_wifi_status() {
    return wifi_connected;
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

