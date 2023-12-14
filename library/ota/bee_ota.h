/*****************************************************************************
 *
 * @file 	bee_ota.h
 * @author 	tuha
 * @date 	3 July 2023
 * @brief	module for update ota
 *
 ***************************************************************************/

#ifndef BEE_OTA_H_
#define BEE_OTA_H_

#define VERSION 1.0
#define MAXIMUM_RETRY_CONNECT_AP 20
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define OTA_FAILED 0
#define OTA_SUCCEED 1
#define LASTEST_VERSION 2
#define CONNECT_AP_FAIL 3 

extern uint8_t ota_status_flag;

/**
 * @brief Start OTA (Over-The-Air) firmware update.
 * 
 * This function initiates the OTA firmware update process by configuring the
 * HTTP client with the provided URL and necessary settings. After downloading
 * the firmware update, the device is rebooted.
 * 
 * @param cUrl The URL to download the firmware update from.
 */
void start_ota(char *url_ota);

void wifi_init_sta(const char *ssid, const char *pass);

void nvs_init_ota();

char* pub_ota_status(char *values);

#endif