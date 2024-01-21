/* Captive Portal Example

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
*/

#include <sys/param.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/inet.h"

#include "esp_http_server.h"
#include "dns_server.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//#include "Max31865.h"
#include <max31865.h>

#include "network.h"
#include "global.h"
#include "driver/gptimer.h"

#define PIN_NUM_MISO CONFIG_PIN_NUM_MISO
#define PIN_NUM_MOSI CONFIG_PIN_NUM_MOSI
#define PIN_NUM_CLK  CONFIG_PIN_NUM_CLK
#define PIN_NUM_CS   CONFIG_PIN_NUM_CS

// HTTP GET Handler

max31865_t tempSensorDevice;
QueueHandle_t thermocouple_queue = NULL;
struct queueItem {
    float temperature;
};

std::vector<float> tempHistory;

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(LED_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
}

static void configure_tempsensor(void) {
    spi_bus_config_t busConfig;
    busConfig.mosi_io_num = CONFIG_PIN_NUM_MOSI;
    busConfig.miso_io_num = CONFIG_PIN_NUM_MISO;
    busConfig.sclk_io_num = CONFIG_PIN_NUM_CLK;
    busConfig.intr_flags = 0;
    busConfig.flags = 0;
    busConfig.quadwp_io_num = -1;
    busConfig.quadhd_io_num = -1;
    busConfig.data4_io_num = -1;
    busConfig.data4_io_num = -1;
    busConfig.data4_io_num = -1;
    busConfig.data4_io_num = -1;
    busConfig.max_transfer_sz = 8;
    busConfig.isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO;/* Which Core to run ISR on*/

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &busConfig, 0));//SPI_DMA_CH_AUTO /* also try _DISABLE*/));
        
    tempSensorDevice.standard = MAX31865_ITS90;//MAX31865_DIN43760;
    tempSensorDevice.r_ref = 430;
    tempSensorDevice.rtd_nominal = 100;
    ESP_ERROR_CHECK(max31865_init_desc(&tempSensorDevice, SPI2_HOST, 3000000, (gpio_num_t)PIN_NUM_CS));
    
    max31865_config_t tempSensorConfig;
    tempSensorConfig.mode = MAX31865_MODE_SINGLE;
    tempSensorConfig.connection = MAX31865_2WIRE;
    tempSensorConfig.v_bias = true;
    tempSensorConfig.filter = MAX31865_FILTER_50HZ;

    ESP_ERROR_CHECK(max31865_set_config(&tempSensorDevice, &tempSensorConfig));

}

void receiver_task(void *pvParameter) {
    ESP_LOGI("task", "Created: thermocouple_task");
    QueueHandle_t thermocouple_queue = (QueueHandle_t)pvParameter;
	
	queueItem item;
    if(thermocouple_queue == NULL){
	    ESP_LOGI("queue", "Not Ready: thermocouple_queue");
        return;
    }
    while(1){
        xQueueReceive(thermocouple_queue,&item,(TickType_t )(1000/portTICK_PERIOD_MS)); 
        printf("Struct Received on Queue:\nTemperature: %0.2f\n", item.temperature);
        if (tempHistory.size() > 90)
            tempHistory.erase(tempHistory.begin());
        tempHistory.push_back(item.temperature);
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}

void thermocouple_task(void *pvParameter) {
    ESP_LOGI("task", "Created: receiver_task");
    QueueHandle_t thermocouple_queue = (QueueHandle_t)pvParameter;

    // Init sensor
    //max31856_cfg_t max31856 = max31856_init();
    //thermocouple_set_type(&max31856, MAX31856_TCTYPE_K);

    if(thermocouple_queue == NULL){
	    ESP_LOGI("queue", "Not Ready: thermocouple_queue");
        return;
    }
    queueItem item;
    float temp;
    while(1){
        max31865_clear_fault_status(&tempSensorDevice);
        max31865_start_measurement(&tempSensorDevice);
        vTaskDelay(pdMS_TO_TICKS(500));
        max31865_read_temperature(&tempSensorDevice, &temp);
        item.temperature = temp;
        //printf("Read Temperature %f\n", temp);
        xQueueSend(thermocouple_queue,&item,(TickType_t )0);
        vTaskDelay(500/portTICK_PERIOD_MS);
    }
}

void create_tasks(void) {
    thermocouple_queue = xQueueCreate(5, sizeof(struct queueItem));
    if(thermocouple_queue != NULL){
	    ESP_LOGI("queue", "Created: thermocouple_queue");
        vTaskDelay(1000/portTICK_PERIOD_MS);
        xTaskCreate(&thermocouple_task, "thermocouple_task", 2048, (void *)thermocouple_queue, 5, NULL);
        xTaskCreate(&receiver_task, "receiver_task", 2048, (void *)thermocouple_queue, 5, NULL);
    } else{
	    ESP_LOGI("queue", "Failed to Create: thermocouple_queue");
    }  

}

gptimer_handle_t pwm_timer;

bool pwm_alarm(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
    //ESP_LOGI("muh", "alarm!!!");
    s_led_state = 1 - s_led_state;
    gpio_set_level(LED_GPIO, s_led_state);
    return true;
}


void init_pwm() {
    //timer_init(TIMER_GROUP_0, TIMER_0, true, 3);

    gptimer_config_t cfg;
    cfg.clk_src = GPTIMER_CLK_SRC_DEFAULT;
    cfg.direction = GPTIMER_COUNT_UP;
    cfg.resolution_hz = 32768;     // in Hz
    cfg.intr_priority = 0;  /*if set to 0, the driver will try to allocate an interrupt with a relative low priority (1,2,3) */
    cfg.flags.intr_shared = 0; // dont want to share interrupt

    ESP_ERROR_CHECK(gptimer_new_timer(&cfg, &pwm_timer));

    //together with resolution will get seconds
    //gptimer_set_raw_count
    //gptimer_get_raw_count
    //gptimer_get_resolution
    //
    //gptimer_get_captured_count // within ISR

    gptimer_event_callbacks_t cbs;
    cbs.on_alarm = pwm_alarm;
    gptimer_register_event_callbacks(pwm_timer, &cbs, nullptr /* user data */);

    gptimer_alarm_config_t alarm_config;    //given as ptr so change there?
    alarm_config.alarm_count = 2 * 32768;  // after 10s cause alarm
    alarm_config.reload_count = 0;  // what to set the timer register to
    alarm_config.flags.auto_reload_on_alarm = 1;         // 0... not auto reload 1... auto relaod

    gptimer_set_alarm_action(pwm_timer, &alarm_config);


    gptimer_enable(pwm_timer);
    gptimer_start(pwm_timer);
}

extern "C" void app_main(void)
{
    init_pwm();
    
    configure_tempsensor();
    
    //    Turn of warnings from HTTP server as redirecting traffic will yield
    //    lots of invalid requests
    esp_log_level_set("httpd_uri", ESP_LOG_ERROR);
    esp_log_level_set("httpd_txrx", ESP_LOG_ERROR);
    esp_log_level_set("httpd_parse", ESP_LOG_ERROR);


    // Initialize networking stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop needed by the  main app
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize NVS needed by Wi-Fi
    ESP_ERROR_CHECK(nvs_flash_init());

    // Initialize Wi-Fi including netif with default config
    esp_netif_create_default_wifi_ap();

    // Initialise ESP32 in SoftAP mode
    wifi_init_softap();

    // Start the server for the first time
    start_webserver();

    // Start the DNS server that will redirect all queries to the softAP IP
    //start_dns_server();

    configure_led();
    
    create_tasks();


}

