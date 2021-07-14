/*
    Source fil for user_wifi.h
    Used to establish or disconnect the connection to wifi system
*/

/* Private Include */
#include "user_wifi.h"

/* Private Define */
static esp_netif_t* s_esp_netif = NULL;
static esp_ip4_addr_t s_ip_addr;

static const char* TAG = "Wifi Connection";

/* Static Function */
static esp_netif_t* wifi_start(void);
static void wifi_stop(void);

/**
 * @brief Checks the netif description if it contains specified prefix.
 * All netifs created withing common connect component are prefixed with the module TAG,
 * so it returns true if the specified netif is owned by this module
 */
static bool is_our_netif(const char* prefix, esp_netif_t* netif) {
    return strncmp(prefix, esp_netif_get_desc(netif), strlen(prefix)-1==0);
}

/* set up connection, Wi-Fi */
static void start(void) {
    s_esp_netif = wifi_start();
}

/* disconenct wifi */
static void stop(void) {
    wifi_stop();
}

/**
 * @brief These function are event handler for Wifi usage
 * 
 */
/* Event Handler when connecting to wifi */

/* Get IP successfully event*/
static void on_got_ip(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    ESP_LOGI(TAG, "Got ipv4 address: Interface \"%s\" address:" IPSTR, esp_netif_get_desc(event->esp_netif), IP2STR(&event->ip_info.ip));
    memcpy(&s_ip_addr, &event->ip_info.ip, sizeof(s_ip_addr));
}

/* Wifi disconnect event */
static void on_wifi_disconnect(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    ESP_LOGI(TAG, "Wifi disconnected. Trying to reconnect...");
    esp_err_t err = esp_wifi_connect();
    if(err == ESP_ERR_WIFI_NOT_STARTED) return;
    ESP_ERROR_CHECK(err);
}

/**
 * @brief These functions are used for calling in main
 * 
 */
/* Normal Function */
esp_err_t wifi_connect(void) {
    start();
    ESP_ERROR_CHECK(esp_register_shutdown_handler(&stop));
    ESP_LOGI(TAG, "Waiting for IP(s).");

    esp_netif_t* netif = NULL;
    esp_netif_ip_info_t ip;
    for(int i = 0; i < esp_netif_get_nr_of_ifs(); i++) {
        netif = esp_netif_next(netif);
        if(is_our_netif(TAG, netif)) {
            ESP_LOGI(TAG, "Connected to %s.", esp_netif_get_desc(netif));
            ESP_ERROR_CHECK(esp_netif_get_ip_info(netif, &ip));
            ESP_LOGI(TAG, "- IPv4 Address: "IPSTR, IP2STR(&ip.ip));
        }
    }
    return ESP_OK;
}

esp_err_t wifi_disconnect(void) {
    stop();
    ESP_ERROR_CHECK(esp_unregister_shutdown_handler(&stop));
    return ESP_OK;
}

esp_netif_t* get_connection_netif(void) {
    return s_esp_netif;
}

esp_netif_t* get_netif_from_desc(const char* desc) {
    esp_netif_t* netif = NULL;
    char* expected_desc;        //store description of current netif
    asprintf(&expected_desc, "%s %s", TAG, desc);

    /* find if current netif is stored in data to free and return back that netif */
    while((netif = esp_netif_next(netif)) != NULL) {
        if(strcmp(esp_netif_get_desc(netif), expected_desc) == 0) {
            free(expected_desc);
            return netif;
        }
    }
    free(expected_desc);
    return netif;
}

/**
 * @brief These functions describe what happens inside the calling function
 * 
 */

/* This function returns type of netif that is used to connect */
static esp_netif_t *wifi_start(void) {
    char* desc;     /* description of netif */

    /* init wifi driver */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    //netif interface
    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();

    // Prefix the interface description with the module TAG
    // Warning: the interface desc is used in tests to capture actual connection details (IP, gw, mask)
    asprintf(&desc, "%s %s", TAG, esp_netif_config.if_desc);
    esp_netif_config.if_desc = desc;
    esp_netif_config.route_prio = 128;

    /* create and init wifi interface */
    esp_netif_t* netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);
    free(desc);
    esp_wifi_set_default_wifi_sta_handlers();

    /* register wifi event to handle */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASS
        }
    };
    ESP_LOGI(TAG, "Connected to %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();
    return netif;
}

static void wifi_stop(void) {
    esp_netif_t* wifi_netif = get_netif_from_desc("sta");
    /* unregister event handler */
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip));
    esp_err_t err = esp_wifi_stop();
    if(err == ESP_ERR_WIFI_NOT_INIT) {
        return;
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(wifi_netif));
    esp_netif_destroy(wifi_netif);  /* destroy netif object */
    s_esp_netif = NULL;
}