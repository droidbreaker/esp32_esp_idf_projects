idf_component_register(
    SRCS "Sync_Async_main.c"
    INCLUDE_DIRS "."
    REQUIRES
        esp_wifi
        esp_event
        esp_netif
        nvs_flash
        esp_http_server
        esp_driver_gpio
        esp_driver_ledc
        freertos
        log
)