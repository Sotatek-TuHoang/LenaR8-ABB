/**
 * @file bee_Led.c
 * @author nguyen__viet_hoang
 * @date 28 September 2023
 * @brief module for output with GPIO, API "create", "set level" for others functions
 */
#include <driver/gpio.h>
#include "bee_Led.h"

static void led_vCreate(gpio_num_t gpio_num)
{
    esp_rom_gpio_pad_select_gpio(gpio_num);
    // Set the GPIO as a push/pull output
    gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT);
}

void led_vSetLevel(gpio_num_t gpio_num, int level)
{
    gpio_set_level(gpio_num, level);
}

void led_vCreate_status()
{
    led_vCreate(LED_RED);
    led_vCreate(LED_GREEN);
    led_vCreate(LED_BLUE);

    led_vCreate(IO_POWER_ON);
    led_vCreate(IO_RESET_LENA);
}

void led_rgb_red()
{
    led_vSetLevel(LED_RED, HIGH_LEVEL);
    led_vSetLevel(LED_GREEN, LOW_LEVEL);
    led_vSetLevel(LED_BLUE, LOW_LEVEL);
}

void led_rgb_green()
{
    led_vSetLevel(LED_RED, LOW_LEVEL);
    led_vSetLevel(LED_GREEN, HIGH_LEVEL);
    led_vSetLevel(LED_BLUE, LOW_LEVEL);
}    

void led_rgb_blue()
{
    led_vSetLevel(LED_RED, LOW_LEVEL);
    led_vSetLevel(LED_GREEN, LOW_LEVEL);
    led_vSetLevel(LED_BLUE, HIGH_LEVEL);
} 

void led_rgb_off()
{
    led_vSetLevel(LED_RED, LOW_LEVEL);
    led_vSetLevel(LED_GREEN, LOW_LEVEL);
    led_vSetLevel(LED_BLUE, LOW_LEVEL);    
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/