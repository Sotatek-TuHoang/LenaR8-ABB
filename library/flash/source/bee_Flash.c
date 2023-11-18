/**
 * @file bee_FLash.c
 * @author nguyen__viet_hoang
 * @date 25 June 2023
 * @brief module for process with flash memory, API "init", "open", "close", "read", "write" for others functions
 */
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bee_FLash.h"

#define ERROR_FLASH 6
#define ERROR_WRITE 0
#define WRITE_SUCCESS 1

uint8_t u8Flash_data;

void flash_vFlashInit(esp_err_t *pErr)
{
    (*pErr) = nvs_flash_init();
    if ((*pErr) == ESP_ERR_NVS_NO_FREE_PAGES || (*pErr) == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        *pErr = nvs_flash_init();
    }
    ESP_ERROR_CHECK(*pErr);
}
void flash_vFlashOpen(esp_err_t *pErr, nvs_handle_t *p_myHandle)
{
    *pErr = nvs_open("storage", NVS_READWRITE, p_myHandle);
}
void flash_vFlashClose(nvs_handle_t *p_myHandle)
{
    nvs_close(p_myHandle);
}
uint8_t flash_u8FlashReadU8(esp_err_t *pErr, nvs_handle_t *p_myHandle, uint8_t *pu8_Value)
{
    *pErr = nvs_get_u8(*p_myHandle, "gia tri", pu8_Value);
    uint8_t u8GiaTri = *pu8_Value;

    switch (*pErr)
    {
    case ESP_OK:
        return (u8GiaTri);
        break;
    default:
        break;
    }
    return ERROR_FLASH;
}
uint8_t flash_u8FlashWriteU8(esp_err_t *pErr, nvs_handle_t *p_myHandle, uint8_t *pu8_Value)
{
    *pErr = nvs_set_u8(*p_myHandle, "gia tri", *pu8_Value);
    uint8_t u8GiaTri = ERROR_WRITE;
    if (*pErr == ESP_OK)
        u8GiaTri = WRITE_SUCCESS;
    *pErr = nvs_commit(*p_myHandle);
    nvs_close(&p_myHandle);
    return u8GiaTri;
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
