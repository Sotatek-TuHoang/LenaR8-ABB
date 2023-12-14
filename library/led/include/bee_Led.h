/**
 * @file bee_Led.h
 * @author nguyen__viet_hoang
 * @date 25 June 2023
 * @brief module for output with GPIO, API "create", "set level" for others functions
 */
#ifndef BEE_LED_H
#define BEE_LED_H

#include "esp_err.h"
#include "hal/gpio_types.h"

#define LED_RED GPIO_NUM_26
#define LED_GREEN GPIO_NUM_25
#define LED_BLUE GPIO_NUM_32

#define IO_POWER_ON GPIO_NUM_19
#define IO_RESET_LENA GPIO_NUM_18

#define HIGH_LEVEL 0
#define LOW_LEVEL 1

void led_vSetLevel(gpio_num_t gpio_num, int level);
void led_vCreate_status();

void led_rgb_red();
void led_rgb_green();
void led_rgb_blue();
void led_rgb_off();

#endif