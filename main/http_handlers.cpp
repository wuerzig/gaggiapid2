

#include <cmath>
#include <algorithm>
#include <string>

#include <esp_http_server.h>
#include "driver/gpio.h"
#include "esp_log.h"

#include "network.h"
#include "http_handlers.h"

#include "global.h"

extern const char root_start[] asm("_binary_root_html_start");
extern const char root_end[] asm("_binary_root_html_end");

void str_replace(std::string& str,
               const std::string& oldStr,
               const std::string& newStr)
{
  std::string::size_type pos = 0u;
  while((pos = str.find(oldStr, pos)) != std::string::npos){
     str.replace(pos, oldStr.length(), newStr);
     pos += newStr.length();
  }
}

esp_err_t root_get_handler(httpd_req_t *req)
{
    const uint32_t root_len = root_end - root_start;

    ESP_LOGI(TAG, "Serve root");
    httpd_resp_set_type(req, "text/html");
    
    char *root_string = (char *)calloc(root_len + 1, sizeof(char)); // we want zeros
    
    char *temp_str = (char *)calloc(20, sizeof(char));
    sprintf(temp_str, "%.1f", tempHistory.back());

    strncpy(root_string, root_start, root_len);
    std::string target = root_string;
    str_replace(target, "###TEMP###", temp_str);
    

    //httpd_resp_send(req, root_start, root_len);
    httpd_resp_send(req, target.c_str(), target.length());
    free(temp_str);
    free(root_string);

    return ESP_OK;
}

esp_err_t led_on_handler(httpd_req_t *req)
{
    const char *str = "LED IS ON";
    ESP_LOGI(TAG, "Serve led on");

    s_led_state = 1;
    gpio_set_level(LED_GPIO, s_led_state);
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, str, strlen(str));

    return ESP_OK;
}

esp_err_t led_off_handler(httpd_req_t *req)
{
    const char *str = "LED IS off";
    ESP_LOGI(TAG, "Serve led off");

    s_led_state = 0;
    gpio_set_level(LED_GPIO, s_led_state);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, str, strlen(str));

    return ESP_OK;
}

esp_err_t get_graph_handler(httpd_req_t *req)
{

    const char *svg_pre1 = "<?xml version='1.0' encoding='utf-8'?>\
<svg version='1.1' xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' viewBox='0 0 1000 400'>\
<defs>\
  <pattern id='grid' patternUnits='userSpaceOnUse' x='0' y='0' width='50' height='25' viewBox='0 0 50 25'>\
      <rect x='0' y='0' width='50' height='25' fill='none' stroke-width='1' stroke='#7f8c8d' stroke-opacity='0.25' />\
    </pattern>\
</defs>\
<g><rect x='0' y='0' width='1000' height='400' fill='url(#grid)' /></g>\
<g>\
  <line x1='50' y1='0' x2='50' y2='400' fill='none' stroke='#7f8c8d' stroke-width='2' />\
  <line x1='0' y1='350' x2='1000' y2='350' fill='none' stroke='#7f8c8d' stroke-width='2' />\
  <line x1='0' y1='100' x2='1000' y2='100' fill='none' stroke='#7f8c8d' stroke-width='1' />\
  <line x1='0' y1='225' x2='1000' y2='225' fill='none' stroke='#7f8c8d' stroke-width='1' stroke-dasharray='5 5' />\
  <text x='970' y='390' fill='#7f8c8d' font-family='Helvetica, Arial' font-style='italic' font-size='40'>t</text>";
    const char *svg_labels_s = "<text x='10' y='380' fill='#4f4c4d' font-family='Helvetica, Arial' font-style='italic' font-size='25'>%d</text>\
        <text x='10' y='130' fill='#4f4c4d' font-family='Helvetica, Arial' font-style='italic' font-size='25'>%d</text>";
    
    const char *svg_pre2 = "</g><g>";
    const char *svg_post = "</g></svg>";

    char *svg_labels = (char *)malloc(strlen(svg_labels_s) + 2*15); // plus 2 integers
    
    int lower = (int)(std::floor(*std::min_element(tempHistory.begin(), tempHistory.end()) / 10.f) * 10.f);
    int upper = (int)(std::ceil(*std::max_element(tempHistory.begin(), tempHistory.end()) / 10.f) * 10.f);
    int range = upper - lower;
    if (range <= 11.) {
        // go for 5 deg. range
        lower = (int)(std::floor(*std::min_element(tempHistory.begin(), tempHistory.end()) / 5.f) * 5.f);
        upper = (int)(std::ceil(*std::max_element(tempHistory.begin(), tempHistory.end()) / 5.f) * 5.f);
        range = upper - lower;
    }

    sprintf(svg_labels, svg_labels_s, lower, upper);

    int x = 50;
    int y;
    
    char plot_commands[1024];
    strcpy(plot_commands, "");
    bool first = true;
    for (auto &i : tempHistory) {
        char point_command[30];
        y = 350 - (250 / range) * (i - lower);
        sprintf(point_command, "%c%d,%d", first ? 'M' : 'L', x, y);
        strcat(plot_commands, point_command);
        x += 10;
        first = false;
    }
    const char *svg_plot_start = "<path fill='none' stroke-width='7' stroke='#2980b9' d='";
    const char *svg_plot_end = "' />";

    char *out_buffer = nullptr;
    out_buffer = (char *)malloc(strlen(svg_pre1) + strlen(svg_labels) + strlen(svg_pre2) + strlen(svg_plot_start) + strlen(plot_commands) + strlen(svg_plot_end) + strlen(svg_post) + 1);
    strcpy(out_buffer, svg_pre1);
    strcat(out_buffer, svg_labels);
    strcat(out_buffer, svg_pre2);
    strcat(out_buffer, svg_plot_start);
    strcat(out_buffer, plot_commands);
    strcat(out_buffer, svg_plot_end);
    strcat(out_buffer, svg_post);
    httpd_resp_set_type(req, "image/svg+xml");
    httpd_resp_send(req, out_buffer, strlen(out_buffer));
    free(svg_labels);
    free(out_buffer);
    return ESP_OK;

}

esp_err_t get_temp_handler(httpd_req_t *req)
{
    char res[128];
    //uint16_t rtd;
    ESP_LOGI(TAG, "Serve temperature");

    max31865_clear_fault_status(&tempSensorDevice);
    max31865_start_measurement(&tempSensorDevice);
    vTaskDelay(pdMS_TO_TICKS(500));
    //uint16_t rtdValue;
    //bool fault;
    //uint8_t faultStatusByte;
    float temp;
    //max31865_read_raw(&tempSensorDevice, &rtdValue, &fault);
    max31865_read_temperature(&tempSensorDevice, &temp);
    //printf("Read %d (%Xh) fault %d faultStatusByte %d (%Xh)\n", rtdValue, rtdValue, fault, faultStatusByte, faultStatusByte);
    snprintf(res, 127, "Read temp %f\n", temp);
   
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, res, strlen(res));

    vTaskDelay(pdMS_TO_TICKS(500));

    return ESP_OK;
}