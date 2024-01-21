
#include <esp_http_server.h>

esp_err_t root_get_handler(httpd_req_t *req);
esp_err_t led_on_handler(httpd_req_t *req);
esp_err_t led_off_handler(httpd_req_t *req);
esp_err_t get_temp_handler(httpd_req_t *req);
esp_err_t get_graph_handler(httpd_req_t *req);
