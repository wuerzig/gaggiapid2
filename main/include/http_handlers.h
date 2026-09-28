
#include <esp_http_server.h>

esp_err_t root_get_handler(httpd_req_t *req);
esp_err_t increase_handler(httpd_req_t *req);
esp_err_t decrease_handler(httpd_req_t *req);
esp_err_t get_temp_handler(httpd_req_t *req);
esp_err_t get_graph_handler(httpd_req_t *req);
