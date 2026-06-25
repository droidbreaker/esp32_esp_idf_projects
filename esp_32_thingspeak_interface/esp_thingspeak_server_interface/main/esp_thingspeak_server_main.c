/**  
 * ESP weather station with thingspeak server example.
 *
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "dht.h"
#include "cJSON.h"
#include "esp_crt_bundle.h"

#include "esp_http_client.h"

esp_err_t temperature_humidity_task(float* temperature, float* humidity);
esp_err_t thingspeak_send(float global_temperature,float local_temperature,float global_humidity,float local_humidity,int temp_difference);

#define WIFI_SSID      "BrAk3R's_N3T_2_4"
#define WIFI_PASS      "123guest@bhatt2"

#define DHT_GPIO GPIO_NUM_5

#define THINGSPEAK_API_KEY "FRSD2TJVHVZK8U5W"

#define WEATHER_URL \
"http://api.openweathermap.org/data/2.5/weather?q=Mumbai,Maharashtra,IN&APPID=ed3d757c7d232b52e1a0b1bd5df8a30b"

static const char *TAG = "WEATHER";

static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

/*----------------------------------------------------------*/
/* HTTP EVENT HANDLER */
/*----------------------------------------------------------*/
static char response_buffer[4096];
static uint32_t response_len = 0;

esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id)
    {
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client))
            {
                printf("%.*s", evt->data_len, (char *)evt->data);
                memcpy(response_buffer + response_len,
                        evt->data,
                        evt->data_len);

            response_len += evt->data_len;

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
    float temperature = 0;
    float humidity1 = 0;

    while(1)
    {

        get_weather();

        if (temperature_humidity_task(&temperature, &humidity1) == ESP_OK)
        {
            printf("sensor Temp = %.1f\n", temperature);
            printf("sensor Hum = %.1f\n", humidity1);
        }   
        else
        {
            printf("Failed to read data from DHT sensor\n");
        }
        cJSON *root = cJSON_Parse((char*)response_buffer);

        cJSON *main = cJSON_GetObjectItem(root, "main");

        cJSON *temp = cJSON_GetObjectItem(main, "temp");

        cJSON *humidity = cJSON_GetObjectItem(main, "humidity");
        double temp_kelvin = temp->valuedouble;
        double temp_celsius = temp_kelvin - 273.15;
        printf("global Temp = %.2f\n", temp_celsius);
        double humidity_value = humidity->valuedouble;
        printf("global Humidity = %.2f\n", humidity_value);
        int temp_difference = (int)(temp_celsius - temperature);
        thingspeak_send(temp_celsius, temperature, humidity_value, humidity1,temp_difference);          // temperature from openweathermap and temperature from dht11 sensor
        cJSON_Delete(root); 
    vTaskDelay(pdMS_TO_TICKS(30000));

   }
}


esp_err_t temperature_humidity_task(float* temperature, float* humidity)
{
    float t, h;
   
        esp_err_t res = dht_read_float_data(
                            DHT_TYPE_DHT11,
                            DHT_GPIO,
                            &h,
                            &t);

        if (res == ESP_OK)
        {
            *temperature = t;
            *humidity = h;
        }
        else
        {
           printf("DHT11 read failed: %s\n",
                  esp_err_to_name(res));
           *temperature = 0;
           *humidity = 0;       
        }
    return res;
}

esp_err_t thingspeak_send(float global_temperature,float local_temperature,float global_humidity,float local_humidity,int temp_difference)
{
    char url[256];

    snprintf(url,
             sizeof(url),
             "https://api.thingspeak.com/update?api_key=%s&field1=%.2f&field2=%.2f&field3=%.2f&field4=%.2f&field5=%d",
             THINGSPEAK_API_KEY,
             global_temperature,
             local_temperature,
             global_humidity,
             local_humidity,
             temp_difference);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client =
        esp_http_client_init(&config);

    esp_err_t err =
        esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        int status =
            esp_http_client_get_status_code(client);

        printf("ThingSpeak Status: %d\n",
               status);
    }

    esp_http_client_cleanup(client);

    return err;
}