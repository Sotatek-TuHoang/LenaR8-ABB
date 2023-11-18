/**
 * @file bee_App_main.c
 * @author nguyen__viet_hoang
 * @date 21 September 2023
 * @brief app_main for ublox project
 */
#include <nvs_flash.h>

#include "bee_Uart.h"
#include "bee_rs485.h"
#include "bee_Button.h"
#include "bee_Pairing_ble.h"
#include "bee_Lena_r8.h"
#include "bee_FLash.h"

esp_err_t flag_config_flash;
nvs_handle_t my_handle_flash;

uint8_t u8Flag_config = 0;

static void button_vEventCallback(uint8_t pin)
{

    if (pin == BUTTON_ADVERTISE_BLE)
    {
        wifi_vRetrySmartConfig();
    }
}

void app_main()
{
    // flash_vFlashInit(&flag_config_flash);

    // button_vCreateInput(BUTTON_ADVERTISE_BLE, HIGH_TO_LOW);
    // button_vSetCallback(button_vEventCallback);

    // flash_vFlashOpen(&flag_config_flash, &my_handle_flash);
    // if (flash_u8FlashReadU8(&flag_config_flash, &my_handle_flash, &u8Flag_config) == 1)
    // {
    //     printf("%d\n", u8Flag_config);
    //     mqtt_vLena_r8_start();
    // }

    uart_vCreate();
    rs485_init();
    rs485_start();
    mqtt_vLena_r8_start();
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/