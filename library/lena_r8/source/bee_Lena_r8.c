/**
 * @file bee_Lena_r8.c
 * @author nguyen__viet_hoang
 * @date 21 September 2023
 * @brief module SIM LENA-R8 with API, init, connect MQTT broker, configure parameters and publish/subscribe data sensor
 */
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_system.h"
#include "esp_mac.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"

#include "bee_cJSON.h"
#include "bee_Uart.h"
#include "bee_rs485.h"
#include "bee_Lena_r8.h"

// QueueHandle_t queue_message_response; // queue for task subscribe

extern bool check_data_flag;
extern uint8_t trans_code;

static char BEE_TOPIC_SUBSCRIBE[100];
static char BEE_TOPIC_PUBLISH[100];

static bool flag_connect_fail = 0;

static uint8_t u8Connect_fail = 0;

static uint8_t u8Mac_address[6] = {0xb8, 0xd6, 0x1a, 0x6b, 0x2d, 0xe8};
static char mac_address[13];

static char message_publish[BEE_LENGTH_AT_COMMAND];
static char message_response[BEE_LENGTH_MESSAGE_RESPONSE];
// static char message_publish_content_for_publish_mqtt_binary[BEE_LENGTH_AT_COMMAND];
static char message_publish_content_for_publish_mqtt_binary_rs485[BEE_LENGTH_AT_COMMAND_RS485];
static char message_publish_content_for_publish_mqtt_binary_keep_alive[BEE_LENGTH_AT_COMMAND];

static void lena_vConfigure_credential()
{
    char command_AT[BEE_LENGTH_AT_COMMAND] = {};

    // config client Id
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+UMQTT=0,%s\r\n", BEE_MQTT_CLIENT_ID);
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
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
    uart_read_bytes(EX_UART_NUM, message_response, BEE_LENGTH_MESSAGE_RESPONSE, (TickType_t)TICK_TIME_TO_SUBSCRIBE_FULL_MESSAGE);
    printf("Query: %s\n", message_response);
    vTaskDelay(pdMS_TO_TICKS(5000));

    // CGACT
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+CGACT=1,1\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    uart_read_bytes(EX_UART_NUM, message_response, BEE_LENGTH_MESSAGE_RESPONSE, (TickType_t)TICK_TIME_TO_SUBSCRIBE_FULL_MESSAGE);
    printf("CGACT: %s\n", message_response);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // AT connect
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+UMQTTC=1\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    uart_read_bytes(EX_UART_NUM, message_response, BEE_LENGTH_MESSAGE_RESPONSE, (TickType_t)TICK_TIME_TO_SUBSCRIBE_FULL_MESSAGE);
    printf("AT connect: %s\n", message_response);
    vTaskDelay(pdMS_TO_TICKS(5000));

    // create AT command to subscribe topic on broker
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+UMQTTC=4,0,%s\r\n", BEE_TOPIC_SUBSCRIBE);
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    uart_read_bytes(EX_UART_NUM, message_response, BEE_LENGTH_MESSAGE_RESPONSE, (TickType_t)TICK_TIME_TO_SUBSCRIBE_FULL_MESSAGE);
    printf("AT subscribe: %s\n", message_response);
    vTaskDelay(pdMS_TO_TICKS(2000));
}

static void lena_vPublish_data_rs485()
{
    char *find_error;

    // Create AT command to publish json message rs485
    char *message_json_rs485 = (char *)calloc(BEE_LENGTH_AT_COMMAND_RS485, sizeof(char));
    message_json_rs485 = pack_json_3pha_data();

    snprintf(message_publish, BEE_LENGTH_AT_COMMAND, "AT+UMQTTC=9,0,0,%s,%d\r\n", BEE_TOPIC_PUBLISH, strlen(message_json_rs485) + 1);
    snprintf(message_publish_content_for_publish_mqtt_binary_rs485, BEE_LENGTH_AT_COMMAND_RS485, "%s\r\n", message_json_rs485);

    // Send AT command
    uart_write_bytes(EX_UART_NUM, message_publish, strlen(message_publish));

    uart_read_bytes(EX_UART_NUM, message_response, BEE_LENGTH_MESSAGE_RESPONSE, (TickType_t)TICK_TIME_TO_SUBSCRIBE_FULL_MESSAGE);
    find_error = strstr(message_response, "invalid command");
    if (find_error != NULL)
    {
        u8Connect_fail++;
    }

    // Send content to publish
    uart_write_bytes(EX_UART_NUM, message_publish_content_for_publish_mqtt_binary_rs485, strlen(message_publish_content_for_publish_mqtt_binary_rs485) + 1);
    // printf("%s\n", message_publish_content_for_publish_mqtt_binary_rs485);
    uart_read_bytes(EX_UART_NUM, message_response, BEE_LENGTH_MESSAGE_RESPONSE, (TickType_t)TICK_TIME_TO_SUBSCRIBE_FULL_MESSAGE);
    find_error = strstr(message_response, "invalid command");
    if (find_error != NULL)
    {
        u8Connect_fail++;
    }

    // reset if LENA-R8 can't connect broker
    if (u8Connect_fail >= BEE_COUNT_MAX_CONNECTED_FAIL)
    {
        flag_connect_fail = 1;
        u8Connect_fail = 0;
        mqtt_vLena_r8_start();
    }
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
    uart_read_bytes(EX_UART_NUM, message_response, BEE_LENGTH_MESSAGE_RESPONSE, (TickType_t)TICK_TIME_TO_SUBSCRIBE_FULL_MESSAGE);
    printf("Response AT keep alive: %s\n", message_response);

    // Send content to publish
    uart_write_bytes(EX_UART_NUM, message_publish_content_for_publish_mqtt_binary_keep_alive, strlen(message_publish_content_for_publish_mqtt_binary_keep_alive) + 1);
    uart_read_bytes(EX_UART_NUM, message_response, BEE_LENGTH_MESSAGE_RESPONSE, (TickType_t)TICK_TIME_TO_SUBSCRIBE_FULL_MESSAGE);
    printf("Response content keep alive: %s\n", message_response);
}

static void mqtt_vPublish_task()
{
    static TickType_t last_time_publish = 0;
    static TickType_t last_time_keep_alive = 0;

    for (;;)
    {
        if (xTaskGetTickCount() - last_time_publish >= pdMS_TO_TICKS(BEE_TIME_PUBLISH_DATA_RS485))
        {
            if (check_data_flag == 1) // new data
            {
                lena_vPublish_data_rs485();
                u8Connect_fail++;
                check_data_flag = 0; // reset data's status
            }
            last_time_publish = xTaskGetTickCount();
        }
        if (xTaskGetTickCount() - last_time_keep_alive >= pdMS_TO_TICKS(BEE_TIME_PUBLISH_DATA_KEEP_ALIVE))
        {
            lena_vPublish_keep_alive();
            last_time_keep_alive = xTaskGetTickCount();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

#if (0) // parse json for task subscribe
static void mqtt_vParse_json(char *rxBuffer)
{
    cJSON *root = cJSON_Parse(rxBuffer);
    if (root != NULL)
    {
        char *device_id;
        char *cmd_name;
        char *object_type;

        device_id = cJSON_GetObjectItemCaseSensitive(root, "thing_token")->valuestring;
        cmd_name = cJSON_GetObjectItemCaseSensitive(root, "cmd_name")->valuestring;

        cJSON_Delete(root);
    }
}
#endif

#if (0) // Add task subscribe
static void mqtt_vSubscribe_command_server_task()
{

    char list_message_subscribe[BEE_LENGTH_AT_COMMAND] = {};
    char *dtmp = (char *)malloc(200);
    char command_AT[BEE_LENGTH_AT_COMMAND] = {};
    snprintf(command_AT, 16, "AT+UMQTTC=6,1\r\n");
    uart_event_t uart_event;

    for (;;)
    {
        // If broker publish message for module
        uart_read_bytes(EX_UART_NUM, list_message_subscribe, BEE_LENGTH_AT_COMMAND, (TickType_t)0);

        if (strstr(list_message_subscribe, "+UUMQTTC: 6") != NULL)
        {
            uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));

            if (xQueueReceive(queue_message_response, (void *)&uart_event, (TickType_t)portMAX_DELAY))
            {
                // Read message AT command from broker
                uart_read_bytes(EX_UART_NUM, list_message_subscribe, BEE_LENGTH_AT_COMMAND, (TickType_t)TICK_TIME_TO_SUBSCRIBE_FULL_MESSAGE);

                // Filter message json
                list_message_subscribe[strlen(list_message_subscribe) - 9] = '\0';
                char *message_json_subscribe;
                message_json_subscribe = strstr(list_message_subscribe, "{");

                // parse json
                mqtt_vParse_json(message_json_subscribe);

                // Publish message json's data sensor
                snprintf(message_publish, BEE_LENGTH_AT_COMMAND, "AT+UMQTTC=9,0,0,%s,%d\r\n", BEE_TOPIC_PUBLISH, strlen(message_publish_content_for_publish_mqtt_binary));
                uart_write_bytes(EX_UART_NUM, message_publish, strlen(message_publish));
                uart_write_bytes(EX_UART_NUM, message_publish_content_for_publish_mqtt_binary, BEE_LENGTH_AT_COMMAND);

                for (uint16_t u8Index = 0; u8Index < BEE_LENGTH_AT_COMMAND; u8Index++)
                {
                    list_message_subscribe[u8Index] = '\0';
                    message_publish_content_for_publish_mqtt_binary[u8Index] = '\0';
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    free(dtmp);
    dtmp = NULL;
}
#endif

bool checkRegistration(char* response) 
{
    // Tìm vị trí của dấu phẩy thứ hai trong chuỗi
    char* secondComma = strchr(response, ',');
    if (secondComma != NULL) {
        // Tìm vị trí của dấu phẩy tiếp theo
        char* thirdComma = strchr(secondComma + 1, ',');

        if (thirdComma != NULL) {
            // Tạo một chuỗi con từ vị trí sau dấu phẩy thứ hai đến trước dấu phẩy thứ ba
            char subString[250];
            strncpy(subString, secondComma + 1, thirdComma - secondComma - 1);
            subString[thirdComma - secondComma - 1] = '\0';

            // Kiểm tra xem "1" hoặc "5" có xuất hiện trong chuỗi con hay không
            if (strchr(subString, '1') != NULL || strchr(subString, '5') != NULL) {
                return true;
            }
        }
    }
    return false;
}

bool reg = false;
void check_module_sim()
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

    // Collect module status
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+UPSV?\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));

    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+CPIN?\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));

    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+CCLK?\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));

    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+CGDCONT?\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));

    // Check registration
    vTaskDelay(pdMS_TO_TICKS(2000));

    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+CEREG?\r\n");
    uart_flush(EX_UART_NUM);
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    uart_read_bytes(EX_UART_NUM, message_response, BEE_LENGTH_MESSAGE_RESPONSE, (TickType_t)300);
    printf("CEREG: %s\n", message_response);
    if (checkRegistration(message_response)==true)
    {
        reg = true;
    }

    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+CGREG?\r\n");
    uart_flush(EX_UART_NUM);
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    uart_read_bytes(EX_UART_NUM, message_response, BEE_LENGTH_MESSAGE_RESPONSE, (TickType_t)300);
    printf("CGREG: %s\n", message_response);
    if (checkRegistration(message_response)==true)
    {
        reg = true;
    }

    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+COPS?\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
    
    // additional AT+CGACT command is necessary
    snprintf(command_AT, BEE_LENGTH_AT_COMMAND, "AT+CGACT=1\r\n");
    uart_write_bytes(EX_UART_NUM, command_AT, strlen(command_AT));
}

void mqtt_vLena_r8_start()
{
    // Get mac address
    esp_efuse_mac_get_default(u8Mac_address);
    snprintf(mac_address, sizeof(mac_address), "%02x%02x%02x%02x%02x%02x", u8Mac_address[0], u8Mac_address[1], u8Mac_address[2], u8Mac_address[3], u8Mac_address[4], u8Mac_address[5]);
    snprintf(BEE_TOPIC_PUBLISH, sizeof(BEE_TOPIC_PUBLISH), "\"VB/DMP/VBEEON/BEE/SMH/%s/telemetry\"", mac_address);
    snprintf(BEE_TOPIC_SUBSCRIBE, sizeof(BEE_TOPIC_SUBSCRIBE), "\"VB/DMP/VBEEON/BEE/SMH/%s/command\"", mac_address);
    check_module_sim();
    if (reg == true)
    {
        // config credential and connect broker
        lena_vConfigure_credential();
        lena_vConnect_mqtt_broker();
    }

    if (flag_connect_fail == 0)
        xTaskCreate(mqtt_vPublish_task, "mqtt_vPublish_task", 1024 * 3, NULL, 3, NULL);

    // xTaskCreate(mqtt_vSubscribe_command_server_task, "mqtt_vSubscribe_command_server_task", 1024 * 3, NULL, 4, NULL);
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/