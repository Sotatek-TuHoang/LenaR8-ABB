/**
 * @file bee_Button.h
 * @author nguyen__viet_hoang
 * @date 25 June 2023
 * @brief module for input button with GPIO, API "create", "call back" for others functions
 */
#ifndef BEE_BUTTON_H
#define BEE_BUTTON_H

#include "esp_err.h"
#include "hal/gpio_types.h"

#define BUTTON_ADVERTISE_BLE GPIO_NUM_0

typedef enum
{
    LOW_TO_HIGH = 1,
    HIGH_TO_LOW,
    ANY_EDGE
} interrupt_type_edge_t;

typedef void (*input_callback_t)(int);

void button_vCreateInput(gpio_num_t gpio_num, interrupt_type_edge_t type);
void button_vSetCallback(void *callbackk);
#endif

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/