#pragma once

#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/inet.h"

#include "esp_http_server.h"

#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_MAX_STA_CONN CONFIG_ESP_MAX_STA_CONN

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err);
httpd_handle_t start_webserver(void);
void wifi_init_softap(void);
void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);
