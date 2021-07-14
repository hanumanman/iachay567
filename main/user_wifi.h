/*
 *   Header file for set up wifi connection. This mode is set in Station mode
 */

#ifndef _USER_WIFI_H_
#define _USER_WIFI_H_

/* Private Include */
#include <string.h>
#include "sdkconfig.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "lwip/err.h"
#include "lwip/sys.h"

/* Private Define */
#define EXAMPLE_INTERFACE   get_connection_netif()
#define CONFIG_WIFI_SSID    "LeTao"
#define CONFIG_WIFI_PASS    "visualstd"

/* Function Prototype */
/**
 * @brief Configure Wi-Fi, connect, wait for IP
 *
 * @return ESP_OK on successful connection
 */
esp_err_t wifi_connect(void);

/**
 * Counterpart to wifi_connect, de-initializes Wi-Fi 
 */
esp_err_t wifi_disconnect(void);

/**
 * @brief Returns esp-netif pointer created by wifi_connect()
 *
 * @note If multiple interfaces active at once, this API return NULL
 * In that case the get_connection_netif_from_desc() should be used
 * to get esp-netif pointer based on interface description
 */
esp_netif_t *get_connection_netif(void);

/**
 * @brief Returns esp-netif pointer created by wifi_connect() described by
 * the supplied desc field
 *
 * @param desc Textual interface of created network interface, for example "sta"
 * indicate default WiFi station, "eth" default Ethernet interface.
 *
 */
esp_netif_t *get_netif_from_desc(const char *desc);

#endif