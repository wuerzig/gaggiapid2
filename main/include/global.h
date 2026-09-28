#ifndef __GLOBALS_H
#define __GLOBALS_H 1

#include "max31865.h"
#include <vector>


#define LED_GPIO (gpio_num_t)2

static const char *TAG = "gaggiapid2";
extern max31865_t tempSensorDevice;
extern std::vector<float> tempHistory;

#define TEMP_TARGET_MAX 120.f
#define TEMP_TARGET_MIN 0.f

extern float temp_target;

static uint8_t s_led_state = 0;

#endif