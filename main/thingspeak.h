
#ifndef _THINGSPEAK_H_
#define _THINGSPEAK_H_

#include <string.h>
#include <stdlib.h>

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define WEB_SERVER "api.thingspeak.com"
#define WEB_PORT "80"
#define THINGSPEAK_API_KEY "J0YKWZFNQPENWSNR"

#define ESP_ERR_THINGSPEAK_BASE 0x60000
#define ESP_ERR_THINGSPEAK_POST_FAILED (ESP_ERR_THINGSPEAK_BASE+1)

#define ESP_ERR_HTTP_BASE 0x40000
#define ESP_ERR_HTTP_DNS_LOOKUP_FAILED          (ESP_ERR_HTTP_BASE + 1)
#define ESP_ERR_HTTP_FAILED_TO_ALLOCATE_SOCKET  (ESP_ERR_HTTP_BASE + 2)
#define ESP_ERR_HTTP_SOCKET_CONNECT_FAILED      (ESP_ERR_HTTP_BASE + 3)
#define ESP_ERR_HTTP_SOCKET_SEND_FAILED         (ESP_ERR_HTTP_BASE + 4)
#define ESP_ERR_HTTP_SOCKET_RECEIVE_TIMEOUT     (ESP_ERR_HTTP_BASE + 4)

#define THINGSPEAK_FIELD_NUMBER                 4

typedef void (*http_callback)(uint32_t* args);

typedef struct {
    char *recv_buf;     /*!< Pointer to a receive buffer, data from the socket are collected here */
    int recv_buf_size;  /*!< Size of the receive buffer */
    char *proc_buf;     /*!< Pointer to processing buffer,  chunks of data from receive buffer and collected here. */
    int proc_buf_size;  /*!< Size of processing buffer*/
} http_client_data_t;

typedef struct {
    uint32_t field_val1;
    uint32_t field_val2;
    uint32_t field_val3;
    uint32_t field_val4;
} field_value_t;

/**
 * @brief conenct ESP to web server
 * 
 * @param web_server name of web server
 * @param request_string request sent by ESP (REST API)
 * @return esp_err_t 
 */
esp_err_t http_client_request(const char *web_server, const char *request_string);
void esp_thingspeak_post(field_value_t* field_data);

#endif