#include "thingspeak.h"

static const char *TAG = "ThingSpeak";
static const char *start_request = "GET https://api.thingspeak.com/update?api_key="THINGSPEAK_API_KEY;
static const char *end_request = 
    " HTTP/1.1\n"
    "Host: "WEB_SERVER"\r\n"
    "Connection: close\r\n"
    "User-Agent: esp32 / esp-idf\r\n"
    "\r\n"; 

esp_err_t http_client_request(const char *web_server, const char *request_string) {
    //structure contains inputs value that set socket and protocol
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };

    struct addrinfo* res;       //response of server
    struct in_addr *addr;
    int s,r;
    char recv_buf[64];  
    
    int err = getaddrinfo(web_server, WEB_PORT, &hints, &res);
    if(err != 0 || res == NULL) {
        ESP_LOGE(TAG, "DNS lookup failed err=%d, res=%p", err, res);
        freeaddrinfo(res);
        vTaskDelay(1000/portTICK_RATE_MS);
        return ESP_ERR_HTTP_DNS_LOOKUP_FAILED;
    }
    /* Code to print the resolved IP.
    
    Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */

    addr = &((struct sockaddr_in*)res->ai_addr)->sin_addr;
    ESP_LOGI(TAG, "DNS lookup success. IP=%s", inet_ntoa(*addr));

    ESP_LOGI(TAG, "Start allocating socket");
    s = socket(res->ai_family, res->ai_socktype, 0);
    if(s<0) {
        ESP_LOGE(TAG, "...Failed to allocate socket.");
        freeaddrinfo(res);
        vTaskDelay(1000/portTICK_RATE_MS);
        return ESP_ERR_HTTP_FAILED_TO_ALLOCATE_SOCKET;
    }
    ESP_LOGI(TAG, "Trying to connect socket");
    if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
        ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
        close(s);
        freeaddrinfo(res);
        vTaskDelay(4000/portTICK_RATE_MS);
        return ESP_ERR_HTTP_SOCKET_CONNECT_FAILED;
    }

    ESP_LOGI(TAG, "...connected");
    freeaddrinfo(res);

    if(write(s, request_string, strlen(request_string)) < 0) {
        ESP_LOGE(TAG, "...socket send failed");
        close(s);
        vTaskDelay(4000/portTICK_RATE_MS);
        return ESP_ERR_HTTP_SOCKET_SEND_FAILED;
    }
    ESP_LOGI(TAG, "...socket send success");

    /*Do not use timeout because ThingSpeak respond too long*/

    /* Receiveing Timeout */ 
    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 60;
    receiving_timeout.tv_usec = 0;

    if(setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout, sizeof(receiving_timeout) < 0)) {
        ESP_LOGE(TAG, "...failed to set socket receiving timeout");
        close(s);
        vTaskDelay(60000/portTICK_RATE_MS);
        return ESP_ERR_HTTP_SOCKET_RECEIVE_TIMEOUT;
    }
    ESP_LOGI(TAG, "...set socket receiving timeout success");

    /*Read HTTP response*/
    do {
        bzero(recv_buf, sizeof(recv_buf));
        r = read(s, recv_buf, sizeof(recv_buf)-1);
        for(int i = 0; i < r; i++) {
            putchar(recv_buf[i]);
        }
    } while(r > 0);

    ESP_LOGI(TAG, "...done reading from socket. Last read return=%d, errno=%d", r, errno);
    close(s);

    vTaskDelay(60000/portTICK_RATE_MS);
    return ESP_OK;
}

void esp_thingspeak_post(field_value_t* field_data) {
    int n;
    printf("ThingSpeak data: %d %d %d %d\n", field_data->field_val1,field_data->field_val2,field_data->field_val3,field_data->field_val4);
    n = snprintf(NULL, 0, "%d", field_data->field_val1);
    char field1[n+1];
    sprintf(field1, "%d", field_data->field_val1);

    n = snprintf(NULL, 0, "%d", field_data->field_val2);
    char field2[n+1];
    sprintf(field2, "%d", field_data->field_val2);

    n = snprintf(NULL, 0, "%d", field_data->field_val3);
    char field3[n+1];
    sprintf(field3, "%d", field_data->field_val3);

    n = snprintf(NULL, 0, "%d", field_data->field_val4);
    char field4[n+1];
    sprintf(field4, "%d", field_data->field_val4);

    printf("Values of 4 fields: %s %s %s %s\n", field1, field2, field3, field4);
    //request string size calculation
    uint16_t string_size = strlen(start_request);

    string_size += strlen("&fieldN=")*THINGSPEAK_FIELD_NUMBER;        //multiply number of fields
    string_size += strlen(field1);

    string_size += strlen(field2);

    string_size += strlen(field3);

    string_size += strlen(field4);

    string_size += strlen(end_request);
    string_size += 1;                                                   //'\0' terminate character

    //assemble request string
    char* get_request = malloc(string_size);
    strcpy(get_request, start_request);

    strcat(get_request, "&field1=");
    strcat(get_request, field1);

    strcat(get_request, "&field2=");
    strcat(get_request, field2);

    strcat(get_request, "&field3=");
    strcat(get_request, field3);

    strcat(get_request, "&field4=");
    strcat(get_request, field4);

    strcat(get_request, end_request);

    ESP_LOGI(TAG, "Set request done. Start posting data to ThingSpeak");

    http_client_request(WEB_SERVER, get_request);
    free(get_request);
}
