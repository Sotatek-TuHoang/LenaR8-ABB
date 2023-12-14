/**
 * @file bee_Uart.c
 * @author nguyen__viet_hoang
 * @date 25 June 2023
 * @brief module uart, API create for others functions
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/portmacro.h"
#include "driver/uart.h"

#include "bee_Uart.h"

extern QueueHandle_t queue_message_response;

void uart_vCreate()
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Install UART driver, and get the queue.
    uart_driver_install(EX_UART_NUM, 1024, 1024, 30, &queue_message_response, 0);
    uart_param_config(EX_UART_NUM, &uart_config);

    // Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(EX_UART_NUM, UART_TX, UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/