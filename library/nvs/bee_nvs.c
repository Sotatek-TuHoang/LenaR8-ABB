/*****************************************************************************
 *
 * @file 	bee_nvs.c
 * @author 	tuha
 * @date 	3 July 2023
 * @brief	module for save data, status into nvs flash
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include file                                                  ***/
/****************************************************************************/

#include "bee_nvs.h"
#include "esp_system.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "stdint.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "NVS";
uint16_t CO2_ratio = 1;
uint16_t CUR_ratio = 1;

/****************************************************************************/
/***        Exported functions                                            ***/
/****************************************************************************/

void nvs_flash_func_init()
{
    esp_err_t err = nvs_flash_init(); // Initialize NVS
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    load_config_from_nvs();
}

void save_config_to_nvs(char *type_config, uint16_t value)
{
    nvs_handle_t nvs_config_handle;
    esp_err_t err = nvs_open(NVS_CONFIG, NVS_READWRITE, &nvs_config_handle);

    if (err == ESP_OK)
    {
        if (strcmp(type_config, "CO2_ratio") == 0)
        {
            err = nvs_set_u16(nvs_config_handle, NVS_CO2_RATIO, value);
        }
        else if (strcmp(type_config, "CUR_ratio") == 0)
        {
            err = nvs_set_u16(nvs_config_handle, NVS_CUR_RATIO, value);
        }
            
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error saving wifi credentials to NVS! (%s)\n", esp_err_to_name(err));
        }
        nvs_close(nvs_config_handle);
    }
    else
    {
        ESP_LOGE(TAG, "Error opening NVS handle! (%s)\n", esp_err_to_name(err));
    }
}

void load_config_from_nvs()
{
    nvs_handle_t nvs_config_handle;
    esp_err_t err = nvs_open(NVS_CONFIG, NVS_READONLY, &nvs_config_handle);

    if (err == ESP_OK)
    {
        err = nvs_get_u16(nvs_config_handle, NVS_CO2_RATIO, &CO2_ratio);
        err |= nvs_get_u16(nvs_config_handle, NVS_CUR_RATIO, &CUR_ratio);

        if (err == ESP_OK)
        {
            // Process loaded data, if necessary
        }
        else if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Wifi cred not found in NVS\n");
        }
        else
        {
            ESP_LOGE(TAG, "Error reading wifi credentials from NVS! (%s)\n", esp_err_to_name(err));
        }
        nvs_close(nvs_config_handle);
    }
    else
    {
        ESP_LOGE(TAG, "Error opening NVS handle! (%s)\n", esp_err_to_name(err));
    }
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/