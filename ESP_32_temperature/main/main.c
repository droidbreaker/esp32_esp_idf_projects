#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"



const char *TAG = "main";

void app_main(void)
{
    while (1)
    {
        ESP_LOGI(TAG, "Hello ESP-IDF!");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}