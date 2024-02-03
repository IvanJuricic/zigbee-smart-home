#include "mqtt_client.h"
#include "esp_log.h"

extern SemaphoreHandle_t mqttStartSemaphore, mqttPublishSemaphore;

void mqtt_app_start(void);
void mqtt_publish_data(const char *, const char *);