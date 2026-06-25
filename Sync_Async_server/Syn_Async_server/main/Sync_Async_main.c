#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_netif.h"

#define WIFI_SSID       "FirseStart"
#define WIFI_PASS       "braker@123"
#define LED_GPIO        GPIO_NUM_2
#define LEDC_CHANNEL    LEDC_CHANNEL_0
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_FREQ_HZ    5000
#define LEDC_RESOLUTION LEDC_TIMER_13_BIT   // 0–8191

static const char *TAG = "LED_SERVER";

/* ── HTML page ──────────────────────────────────────────────── */
static const char *HTML_PAGE =
"<!DOCTYPE html><html><head>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>LED Control</title>"
"<style>"
"  body{font-family:sans-serif;max-width:400px;margin:2rem auto;padding:1rem;}"
"  h1{font-size:1.4rem;margin-bottom:1.5rem;}"
"  .btn{display:inline-block;padding:.6rem 1.6rem;border:none;"
"       border-radius:8px;font-size:1rem;cursor:pointer;}"
"  .on {background:#22c55e;color:#fff;}"
"  .off{background:#ef4444;color:#fff;}"
"  .row{margin-bottom:1rem;display:flex;gap:.75rem;}"
"  label{display:block;margin-bottom:.4rem;font-weight:500;}"
"  input[type=range]{width:100%;}"
"  #val{font-size:.85rem;color:#666;}"
"</style></head><body>"
"<h1>ESP32 LED Control</h1>"
"<div class='row'>"
"  <button class='btn on'  onclick=\"fetch('/led/on') .then(r=>r.text()).then(t=>status.textContent=t)\">Turn ON</button>"
"  <button class='btn off' onclick=\"fetch('/led/off').then(r=>r.text()).then(t=>status.textContent=t)\">Turn OFF</button>"
"</div>"
"<label>Brightness <span id='val'>50%</span></label>"
"<input type='range' min='0' max='100' value='50' id='sl'"
"  oninput=\"document.getElementById('val').textContent=this.value+'%';"
"    fetch('/led/brightness?v='+this.value).then(r=>r.text()).then(t=>status.textContent=t)\">"
"<p id='status' style='color:#888;font-size:.85rem;'>Ready</p>"
"</body></html>";

/* ── LEDC init ──────────────────────────────────────────────── */
static void ledc_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_RESOLUTION,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t ch = {
        .gpio_num   = LED_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config(&ch);
}

static void set_brightness(int percent)   // 0–100 → 0–8191
{
    uint32_t duty = (percent * 8191) / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);
}

/* ── HTTP handlers ──────────────────────────────────────────── */
static esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, HTML_PAGE, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t led_on_handler(httpd_req_t *req)
{
    set_brightness(100);
    httpd_resp_sendstr(req, "LED ON");
    return ESP_OK;
}

static esp_err_t led_off_handler(httpd_req_t *req)
{
    set_brightness(0);
    httpd_resp_sendstr(req, "LED OFF");
    return ESP_OK;
}

static esp_err_t brightness_handler(httpd_req_t *req)
{
    char buf[16];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        char val[8];
        if (httpd_query_key_value(buf, "v", val, sizeof(val)) == ESP_OK) {
            int pct = atoi(val);
            if (pct < 0)   pct = 0;
            if (pct > 100) pct = 100;
            set_brightness(pct);
            char resp[32];
            snprintf(resp, sizeof(resp), "Brightness: %d%%", pct);
            httpd_resp_sendstr(req, resp);
            return ESP_OK;
        }
    }
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing ?v=");
    return ESP_OK;
}

/* Wi-Fi event handler */
static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "WiFi disconnected, retrying...");
        esp_wifi_connect();
    }

    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        ESP_LOGI(TAG, "Got IP: " IPSTR,
                 IP2STR(&event->ip_info.ip));
    }
}
/* ── Start server ────────────────────────────────────────────── */
static httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) != ESP_OK) return NULL;

    httpd_uri_t uris[] = {
        { .uri="/",               .method=HTTP_GET, .handler=root_handler       },
        { .uri="/led/on",         .method=HTTP_GET, .handler=led_on_handler     },
        { .uri="/led/off",        .method=HTTP_GET, .handler=led_off_handler    },
        { .uri="/led/brightness", .method=HTTP_GET, .handler=brightness_handler },
    };
    for (int i = 0; i < 4; i++) httpd_register_uri_handler(server, &uris[i]);

    ESP_LOGI(TAG, "Sync server started");
    return server;
}

/* ── WiFi ────────────────────────────────────────────────────── */
static void wifi_init_sta(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    
    esp_event_handler_register(WIFI_EVENT,
                               ESP_EVENT_ANY_ID,
                               &wifi_event_handler,
                               NULL);
    
    esp_event_handler_register(IP_EVENT,
                               IP_EVENT_STA_GOT_IP,
                               &wifi_event_handler,
                               NULL);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_cfg = {
        .sta = { .ssid = WIFI_SSID, .password = WIFI_PASS },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
    esp_wifi_start();
    esp_wifi_connect();
    esp_netif_ip_info_t ip;

    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

    esp_netif_get_ip_info(netif, &ip);
    ESP_LOGI(TAG, "IP: " IPSTR "\n", IP2STR(&ip.ip));
    ESP_LOGI(TAG, "SSID: %s", WIFI_SSID);
    ESP_LOGI(TAG, "WiFi connecting...");
}

void app_main(void)
{
    nvs_flash_init();
    ledc_init();
    wifi_init_sta();

    // Give WiFi ~3 s to connect before starting server
    vTaskDelay(pdMS_TO_TICKS(3000));
    start_webserver();
}