/**
 * @file bee_App_main.c
 * @author nguyen__viet_hoang
 * @date 21 September 2023
 * @brief app_main for ublox project
 */
#include "bee_Uart.h"
#include "bee_rs485.h"
#include "bee_Lena_r8.h"
#include "bee_Led.h"
#include "bee_nvs.h"
#include "bee_ota.h"

void app_main()
{
    led_vCreate_status();
    uart_vCreate();
    nvs_flash_func_init();
    rs485_init();
    rs485_start();
    mqtt_vLena_r8_start();

    //wifi_init_sta("Tupeo", "tu123456");
    //start_ota("https://raw.githubusercontent.com/Sotatek-TuHoang/LenaR8-ABB/main/project_ublox_rs485.bin");
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/