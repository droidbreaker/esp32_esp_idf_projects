
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_netif.h"

#include "esp_http_client.h"


#define WIFI_SSID      "FirseStart"
#define WIFI_PASS      "braker@123"

#define WEATHER_URL \
"http://api.openweathermap.org/data/2.5/weather?q=Mumbai,Maharashtra,IN&APPID=ed3d757c7d232b52e1a0b1bd5df8a30b"

static const char *TAG = "WEATHER";

static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

/*----------------------------------------------------------*/
/* HTTP EVENT HANDLER */
/*----------------------------------------------------------*/
esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id)
    {
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client))
            {
                printf("%.*s", evt->data_len, (char *)evt->data);
            }
            break;

        default:
            break;
    }

    return ESP_OK;
}

/*----------------------------------------------------------*/
/* WIFI EVENT HANDLER */
/*----------------------------------------------------------*/
static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "Reconnecting...");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP)
    {
        xEventGroupSetBits(wifi_event_group,
                           WIFI_CONNECTED_BIT);
    }
}

/*----------------------------------------------------------*/
/* WIFI INIT */
/*----------------------------------------------------------*/
static void wifi_init_sta(void)
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(
        esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg =
        WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(
        esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL,
            &instance_any_id));

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifi_event_handler,
            NULL,
            &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK(
        esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config));

    ESP_ERROR_CHECK(
        esp_wifi_start());

    ESP_LOGI(TAG, "Waiting for WiFi...");

    xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdTRUE,
        portMAX_DELAY);

    ESP_LOGI(TAG, "WiFi Connected");
}

/*----------------------------------------------------------*/
/* WEATHER REQUEST */
/*----------------------------------------------------------*/
static void get_weather(void)
{
    esp_http_client_config_t config = {
        .url = WEATHER_URL,
        .event_handler = http_event_handler,
    };

    esp_http_client_handle_t client =
        esp_http_client_init(&config);

    esp_err_t err =
        esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG,
                 "HTTP GET Status = %d, content_length = %lld",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGE(TAG,
                 "HTTP GET failed: %s",
                 esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

/*----------------------------------------------------------*/
/* APP MAIN */
/*----------------------------------------------------------*/
void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    wifi_init_sta();

    while (1)
    {
        get_weather();

        printf("\n\n");
        printf("====================================\n");
        printf("Waiting 30 seconds...\n");
        printf("====================================\n");
       
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}

