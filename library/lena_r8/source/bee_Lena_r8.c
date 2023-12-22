/**
 * @file bee_Lena_r8.c
 * @author nguyen__viet_hoang
 * @date 21 September 2023
 * @brief module SIM LENA-R8 with API, init, connect MQTT broker, configure parameters and publish/subscribe data sensor
 */
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_system.h"
#include "esp_mac.h"
#include <string.h>
#include "esp_log.h"
#include "cJSON.h"
#include <time.h>
#include <stdio.h>

#include "bee_Uart.h"
#include "bee_rs485.h"
#include "bee_Led.h"
#include "bee_Lena_r8.h"
#include "bee_ota.h"
#include "bee_nvs.h"

QueueHandle_t queue_message_response; // queue for task subscribe
TaskHandle_t mqtt_vPublish_task_handle = NULL;

extern bool check_data_flag;
extern uint8_t trans_code;

static const char *TAG = "LENA-R8";

static char BEE_TOPIC_SUBSCRIBE[100];
static char BEE_TOPIC_PUBLISH[100];

static bool main_tain_connected = 0;

static uint8_t u8Connect_fail = 0;

static uint8_t u8Mac_address[6] = {0xb8, 0xd6, 0x1a, 0x6b, 0x2d, 0xe8};
static char mac_address[13];
static bool reg = false;

static char message_publish[BEE_LENGTH_AT_COMMAND];
static char message_response[BEE_LENGTH_MESSAGE_RESPONSE];
static char message_publish_content_for_publish_mqtt_binary_rs485[BEE_LENGTH_AT_COMMAND_RS485];
static char message_publish_content_for_publish_mqtt_binary_keep_alive[BEE_LENGTH_AT_COMMAND];

static char *cRandomMqttClientId(void)
{
    char *str_rd = (char *)malloc(13 * sizeof(char));
    char *str_return = (char *)malloc(28 * sizeof(char));
    const char char_pool[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    srand(time(NULL));
    for (int i = 0; i < 12; ++i)
    {
        int idx = rand() % (sizeof(char_pool) - 1);
        str_rd[i] = char_pool[idx];
    }
    str_rd[12] = '\0';
    snprintf(str_return, 28, "\"%s%s\"", mac_address, str_rd);
    free(str_rd);
    return str_return;
}

static void lena_vConfigure_credential()
{
    char command_AT[BEE_LENGTH_AT_COMMAND] = {};

    char *MQTT_client = cRandomMqttClientId();
    // config client Id
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+UMQTT=0,%s\r\n", MQTT_client);
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    free(MQTT_client);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // config IP broker and port
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+UMQTT=3,%s,%s\r\n", BEE_MQTT_BROKER_URL, BEE_BROKER_PORT);
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    vTaskDelay(pdMS_TO_TICKS(2000));

    // config broker user name and password
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+UMQTT=4,%s,%s\r\n", BEE_USER_NAME, BEE_USER_PASSWORD);
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    vTaskDelay(pdMS_TO_TICKS(2000));
}

static void lena_vConnect_mqtt_broker()
{
    char command_AT[BEE_LENGTH_AT_COMMAND] = {};

    // Query MQTT's credentials
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+UMQTT?\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    vTaskDelay(pdMS_TO_TICKS(5000));

    // CGACT
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+CGACT=1,1\r\n");
    uart_flush_input(EX_UART_NUM);
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    uart_read_bytes(EX_UART_NUM, message_response, BEE_LENGTH_MESSAGE_RESPONSE, (TickType_t)TICK_TIME_TO_SUBSCRIBE_FULL_MESSAGE);
    ESP_LOGI(TAG, "Response CGACT: %s", message_response);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // AT connect
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+UMQTTC=1\r\n");
    uart_flush_input(EX_UART_NUM);
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    uart_read_bytes(EX_UART_NUM, message_response, BEE_LENGTH_MESSAGE_RESPONSE, (TickType_t)TICK_TIME_TO_SUBSCRIBE_FULL_MESSAGE);

    // confirm connect broker through led
    ESP_LOGI(TAG, "response %s", message_response);
    if (strstr(message_response, "OK") != NULL)
    {
        led_rgb_blue();
        main_tain_connected = 1;
    }

    else if (strstr(message_response, "parameters are invalid") != NULL)
    {
        // reset Lena-R8
        led_vSetLevel(IO_POWER_ON, 0);
        led_vSetLevel(IO_RESET_LENA, 0);
        vTaskDelay(pdMS_TO_TICKS(60000));

        esp_restart();
    }

    ESP_LOGI(TAG, "AT connect: %s", message_response);
    vTaskDelay(pdMS_TO_TICKS(5000));

    // create AT command to subscribe topic on broker
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+UMQTTC=4,0,%s\r\n", BEE_TOPIC_SUBSCRIBE);
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    vTaskDelay(pdMS_TO_TICKS(2000));
}

static void mqtt_vCheck_error()
{
    char *find_error;
    find_error = strstr(message_response, "invalid command");
    if (find_error != NULL)
    {
        u8Connect_fail++;
        ESP_LOGE(TAG, "Fail %s", message_response);
    }
    find_error = strstr(message_response, "ERROR");
    if (find_error != NULL)
    {
        u8Connect_fail++;
        ESP_LOGE(TAG, "Fail %s", message_response);
    }
}

static void lena_vPublish_data_rs485()
{
    // Create AT command to publish json message rs485
    char *message_json_rs485 = (char *)calloc(BEE_LENGTH_AT_COMMAND_RS485, sizeof(char));
    message_json_rs485 = pack_json_3pha_data();

    snprintf(message_publish, BEE_LENGTH_AT_COMMAND, "AT+UMQTTC=9,0,0,%s,%d\r\n", BEE_TOPIC_PUBLISH, strlen(message_json_rs485) + 1);
    snprintf(message_publish_content_for_publish_mqtt_binary_rs485, BEE_LENGTH_AT_COMMAND_RS485, "%s\r\n", message_json_rs485);

    // Send AT command
    uart_write_bytes(EX_UART_NUM, message_publish, strlen(message_publish));
    // Send content to publish
    uart_write_bytes(EX_UART_NUM, message_publish_content_for_publish_mqtt_binary_rs485, strlen(message_publish_content_for_publish_mqtt_binary_rs485) + 1);
    free(message_json_rs485);
}

static char *cCreate_message_json_keep_alive()
{
    cJSON *json_keep_alive, *values;
    json_keep_alive = cJSON_CreateObject();

    cJSON_AddItemToObject(json_keep_alive, "thing_token", cJSON_CreateString(mac_address));
    cJSON_AddItemToObject(json_keep_alive, "values", values = cJSON_CreateObject());
    cJSON_AddItemToObject(values, "eventType", cJSON_CreateString("refresh"));
    cJSON_AddItemToObject(values, "status", cJSON_CreateString("ONLINE"));
    cJSON_AddItemToObject(json_keep_alive, "trans_code", cJSON_CreateNumber(trans_code++));
    char *message_keep_alive_json = cJSON_Print(json_keep_alive);
    cJSON_Delete(json_keep_alive);

    return message_keep_alive_json;
}

static void lena_vPublish_keep_alive()
{
    // Create AT command to publish keep alive
    char *message_json_keep_alive = (char *)calloc(BEE_LENGTH_AT_COMMAND_RS485, sizeof(char));
    message_json_keep_alive = cCreate_message_json_keep_alive();

    snprintf(message_publish, BEE_LENGTH_AT_COMMAND, "AT+UMQTTC=9,0,0,%s,%d\r\n", BEE_TOPIC_PUBLISH, strlen(message_json_keep_alive) + 1);
    snprintf(message_publish_content_for_publish_mqtt_binary_keep_alive, BEE_LENGTH_AT_COMMAND, "%s\r\n", message_json_keep_alive);

    // Send AT command
    uart_write_bytes(EX_UART_NUM, message_publish, strlen(message_publish));
    // Send content to publish
    uart_write_bytes(EX_UART_NUM, message_publish_content_for_publish_mqtt_binary_keep_alive, strlen(message_publish_content_for_publish_mqtt_binary_keep_alive) + 1);
    free(message_json_keep_alive);
}

static bool checkRegistration(char *response)
{
    // Tìm vị trí của dấu phẩy thứ hai trong chuỗi
    char *secondComma = strchr(response, ',');
    if (secondComma != NULL)
    {
        // Tìm vị trí của dấu phẩy tiếp theo
        char *thirdComma = strchr(secondComma + 1, ',');
        if (thirdComma != NULL)
        {
            // Tạo một chuỗi con từ vị trí sau dấu phẩy thứ hai đến trước dấu phẩy thứ ba
            char subString[250];
            strncpy(subString, secondComma + 1, thirdComma - secondComma - 1);
            subString[thirdComma - secondComma - 1] = '\0';

            // Kiểm tra xem "1" hoặc "5" có xuất hiện trong chuỗi con hay không
            if (strchr(subString, '1') != NULL || strchr(subString, '5') != NULL)
            {
                return true;
            }
        }
    }
    return false;
}

static void check_module_sim()
{
    char command_AT[BEE_LENGTH_AT_COMMAND] = {};
    //// URCs initialization
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+CMEE=2\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+CREG=2\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+CGREG=2\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+CEREG=2\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+CGEREP=2,1\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+CSCON=1\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+CFUN=1\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    vTaskDelay(pdMS_TO_TICKS(5000));

    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+UPSV?\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+CPIN?\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+CCLK?\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+CGDCONT?\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Check registration
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+CEREG?\r\n");
    uart_flush_input(EX_UART_NUM);
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    uart_read_bytes(EX_UART_NUM, message_response, BEE_LENGTH_MESSAGE_RESPONSE, (TickType_t)300);
    ESP_LOGI(TAG, "CEREG: %s", message_response);
    if (checkRegistration(message_response) == true)
    {
        reg = true;
    }
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+CGREG?\r\n");
    uart_flush_input(EX_UART_NUM);
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    uart_read_bytes(EX_UART_NUM, message_response, BEE_LENGTH_MESSAGE_RESPONSE, (TickType_t)300);
    ESP_LOGI(TAG, "CGREG: %s", message_response);
    if (checkRegistration(message_response) == true)
    {
        reg = true;
    }
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+COPS?\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    // additional AT+CGACT command is necessary
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+CGACT=1\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
}

static void mqtt_vPublish_task()
{
    static TickType_t last_time_publish = 0;
    static TickType_t last_time_keep_alive = 0;
    static TickType_t last_time_blink_green_led = 0;
    static TickType_t last_time_retry_connect = 0;
    for (;;)
    {
        if (main_tain_connected == 1)
        {
            if (xTaskGetTickCount() - last_time_blink_green_led >= pdMS_TO_TICKS(BEE_TIME_BLINK_GREEN_LED))
            {
                // blink led to perform publish data
                led_rgb_green();
                last_time_blink_green_led = xTaskGetTickCount();
            }

            if (xTaskGetTickCount() - last_time_publish >= pdMS_TO_TICKS(BEE_TIME_PUBLISH_DATA_RS485))
            {
                if (check_data_flag == 1) // new data
                {
                    lena_vPublish_data_rs485();

                    // blink led to perform publish data
                    led_rgb_off();
                    check_data_flag = 0; // reset data's status
                }
                last_time_publish = xTaskGetTickCount();
                last_time_blink_green_led = xTaskGetTickCount();
            }

            if (xTaskGetTickCount() - last_time_keep_alive >= pdMS_TO_TICKS(BEE_TIME_PUBLISH_DATA_KEEP_ALIVE))
            {
                lena_vPublish_keep_alive();

                // confirm disconnect broker through led
                led_rgb_off();
                last_time_keep_alive = xTaskGetTickCount();
                last_time_blink_green_led = xTaskGetTickCount();
            }
        }
        else
        {
            if (xTaskGetTickCount() - last_time_retry_connect >= pdMS_TO_TICKS(10000))
            {
                last_time_retry_connect = xTaskGetTickCount();
                u8Connect_fail++;
            }
            led_rgb_red();
        }
        // reset if LENA-R8 can't connect broker
        if (u8Connect_fail >= BEE_COUNT_MAX_CONNECTED_FAIL)
        {
            ESP_LOGE(TAG, "Reload connect");

            // confirm disconnect broker through led
            led_rgb_red();

            // reset status connect when retry connect broker
            u8Connect_fail = 0;
            main_tain_connected = 0;
            led_vSetLevel(IO_POWER_ON, 0);
            vTaskDelay(pdMS_TO_TICKS(10000));
            led_vSetLevel(IO_POWER_ON, 1);

            reg = false;

            while (reg == false)
            {
                check_module_sim();
            }

            lena_vConfigure_credential();
            lena_vConnect_mqtt_broker();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void mqtt_vParse_json(char *mqtt_str)
{
    //printf("mqtt_str: %s\n", mqtt_str);
    cJSON *root = cJSON_Parse(mqtt_str); // Parse the received MQTT message as a JSON object
    if (root != NULL)
    {
        char *Thing_token = cJSON_GetObjectItemCaseSensitive(root, "thing_token")->valuestring;
        char *Cmd_name = cJSON_GetObjectItemCaseSensitive(root, "cmd_name")->valuestring;
        char *Object_type = cJSON_GetObjectItemCaseSensitive(root, "object_type")->valuestring;
        // blink led to confirm subscribe message json
        led_rgb_off();

        if ((strcmp(Thing_token, mac_address) == 0) && (strcmp(Cmd_name, "Bee.Nag_cmd") == 0) && (strcmp(Object_type, "Bee.Nag_vrf") == 0))
        {
            int Slave_addr = cJSON_GetObjectItemCaseSensitive(root, "slave_addr")->valueint;
            char *Cmd_type = cJSON_GetObjectItemCaseSensitive(root, "cmd_type")->valuestring;
            if ((strcmp(Cmd_type, "RESET_LOGS") == 0))
            {
                reset_data(Slave_addr, RESET_LOGS);
            }
            else if ((strcmp(Cmd_type, "RESET_HISTORICAL_FUNCTIONALITY") == 0))
            {
                reset_data(Slave_addr, RESET_HISTORICAL_FUNCTIONALITY);
            }
            else if ((strcmp(Cmd_type, "RESET_TIMER") == 0))
            {
                reset_data(Slave_addr, RESET_TIMER);
            }
            else if ((strcmp(Cmd_type, "RESET_ENERGY") == 0))
            {
                reset_data(Slave_addr, RESET_ENERGY);
            }
            else if ((strcmp(Cmd_type, "RESET_FACTORY") == 0))
            {
                reset_data(Slave_addr, RESET_FACTORY);
            }
            else if ((strcmp(Cmd_type, "RESET_MAX_MIN_AVR") == 0))
            {
                reset_data(Slave_addr, RESET_MAX_MIN_AVR);
            }
        }
        else if ((strcmp(Thing_token, mac_address) == 0) && (strcmp(Cmd_name, "Bee.Nag_config") == 0) && (strcmp(Object_type, "Bee.Nag_vrf") == 0))
        {
            char *type_config = cJSON_GetObjectItemCaseSensitive(root, "type_config")->valuestring;
            int value = cJSON_GetObjectItemCaseSensitive(root, "value")->valueint;

            if ((strcmp(type_config, "CO2_ratio") == 0))
            {
                CO2_ratio = value;
                save_config_to_nvs("CO2_ratio", value);
            }
            else if ((strcmp(type_config, "CUR_ratio") == 0))
            {
                CUR_ratio = value;
                save_config_to_nvs("CUR_ratio", value);
            }
        }
        else if ((strcmp(Thing_token, mac_address) == 0) && (strcmp(Cmd_name, "Bee.Nag_ota") == 0) && (strcmp(Object_type, "Bee.Nag_vrf") == 0))
        {
            double get_version = cJSON_GetObjectItemCaseSensitive(root, "version")->valuedouble;
            ESP_LOGI(TAG, "version: %f\n", get_version);
            if(get_version > VERSION)
            {
                vTaskDelete(mqtt_vPublish_task_handle);
                char *ssid_ap = cJSON_GetObjectItemCaseSensitive(root, "ssid_ap")->valuestring;
                char *pass_ap = cJSON_GetObjectItemCaseSensitive(root, "pass_ap")->valuestring;
                char *url_ota = cJSON_GetObjectItemCaseSensitive(root, "url_ota")->valuestring;

                nvs_init_ota();
                wifi_init_sta(ssid_ap, pass_ap);
                start_ota(url_ota);
            }
            else
            {
                ota_status_flag = LASTEST_VERSION;
            }
            lena_vPublish_ota_status();
            esp_restart();
        }
        cJSON_Delete(root);
    }
}

static void mqtt_vSubscribe_command_server_task()
{
    uart_event_t uart_event;
    char *dtmp = (char *)malloc(1024);
    char command_AT[BEE_LENGTH_AT_COMMAND] = {};
    snprintf(command_AT, 16, "AT+UMQTTC=6,1\r\n");
    for (;;)
    {
        if (xQueueReceive(queue_message_response, (void *)&uart_event, (TickType_t)50))
        {
            bzero(dtmp, 1024);
            switch (uart_event.type)
            {
                case UART_DATA:
                    // Read message AT command from broker
                    uart_read_bytes(EX_UART_NUM, dtmp, uart_event.size * 5, (TickType_t)50);
                    ESP_LOGI(TAG,"dtmp: %s", dtmp);
                    if (strstr(dtmp, "+UUMQTTC: 6") != NULL)
                    {
                        uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));

                        if (xQueueReceive(queue_message_response, (void *)&uart_event, (TickType_t)50))
                        {
                            uart_read_bytes(EX_UART_NUM, dtmp, uart_event.size * 5, (TickType_t)50);
                            // Filter message json
                            dtmp[strlen(dtmp) - 9] = '\0';
                            char *message_json_subscribe;
                            message_json_subscribe = strstr(dtmp, "{");
                            // parse json
                            mqtt_vParse_json(message_json_subscribe);
                        }
                    }
                    else if (strlen(dtmp) == 0)
                    {

                    }
                    else
                    {
                        mqtt_vCheck_error();
                    }
                    break;
                case UART_FIFO_OVF:
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(queue_message_response);
                    break;
                case UART_BUFFER_FULL:
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(queue_message_response);
                    break;
                case UART_BREAK:
                    // Handle UART_BREAK case
                    break;
                case UART_FRAME_ERR:
                    // Handle UART_FRAME_ERR case
                    break;
                case UART_PARITY_ERR:
                    // Handle UART_PARITY_ERR case
                    break;
                case UART_DATA_BREAK:
                    // Handle UART_DATA_BREAK case
                    break;
                case UART_PATTERN_DET:
                    // UART pattern detect interrupt
                    break;
                case UART_EVENT_MAX:
                    // Handle UART_EVENT_MAX case
                    break;
                default:
                    // Handle other cases or ignore them if not needed
                    break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void lena_vPublish_ota_status()
{
    char ota_status_mess[BEE_LENGTH_AT_COMMAND_RS485];

    // Create AT command to publish json message ota status
    char *message_json_ota_status = (char *)calloc(BEE_LENGTH_AT_COMMAND_RS485, sizeof(char));

    if (ota_status_flag == OTA_FAILED)
    {
        message_json_ota_status = pub_ota_status("Failed");
    }
    else if (ota_status_flag == OTA_SUCCEED)
    {
        message_json_ota_status = pub_ota_status("Succeed");
    }
    else if (ota_status_flag == LASTEST_VERSION)
    {
        message_json_ota_status = pub_ota_status("Latest version");
    }
    else if (ota_status_flag == CONNECT_AP_FAIL)
    {
        message_json_ota_status = pub_ota_status("CONNECT AP FAIL");
    }
    
    snprintf(message_publish, BEE_LENGTH_AT_COMMAND, "AT+UMQTTC=9,0,0,%s,%d\r\n", BEE_TOPIC_PUBLISH, strlen(message_json_ota_status) + 1);
    snprintf(ota_status_mess, BEE_LENGTH_AT_COMMAND_RS485, "%s\r\n", message_json_ota_status);

    // Send AT command
    uart_write_bytes(EX_UART_NUM, message_publish, strlen(message_publish));
    // Send content to publish
    uart_write_bytes(EX_UART_NUM, ota_status_mess, strlen(ota_status_mess) + 1);
    free(message_json_ota_status);
}

void mqtt_vLena_r8_start()
{
    // Get mac address
    esp_efuse_mac_get_default(u8Mac_address);
    snprintf(mac_address, sizeof(mac_address), "%02x%02x%02x%02x%02x%02x", u8Mac_address[0], u8Mac_address[1], u8Mac_address[2], u8Mac_address[3], u8Mac_address[4], u8Mac_address[5]);
    snprintf(BEE_TOPIC_PUBLISH, sizeof(BEE_TOPIC_PUBLISH), "\"VB/DMP/VBEEON/BEE/SMH/%s/telemetry\"", mac_address);
    snprintf(BEE_TOPIC_SUBSCRIBE, sizeof(BEE_TOPIC_SUBSCRIBE), "\"VB/DMP/VBEEON/BEE/SMH/%s/command\"", mac_address);

    // confirm disconnect broker through led
    led_rgb_red();
    // supply power for LENA-R8
    led_vSetLevel(IO_POWER_ON, 1);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Reg led connection status for module sim
    char command_AT[BEE_LENGTH_AT_COMMAND] = {};
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+UGPIOC=16,2\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));

    while (reg == false)
    {
        check_module_sim();
    }

    // config credential and connect broker
    lena_vConfigure_credential();
    lena_vConnect_mqtt_broker();

    xTaskCreate(mqtt_vPublish_task, "mqtt_vPublish_task", 1024 * 6, NULL, 3, &mqtt_vPublish_task_handle);
    xTaskCreate(mqtt_vSubscribe_command_server_task, "mqtt_vSubscribe_command_server_task", 1024 * 12, NULL, 4, NULL);
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/