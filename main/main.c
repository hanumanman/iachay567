#include <stdio.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include "esp_netif.h"
#include <esp_http_server.h>
#include "user_wifi.h"
#include "driver/gpio.h"
#include "sd_card.h"
#include "user_adc.h"
#include "user_timer.h"

#include "thingspeak.h"

static const char* TAG = "Main Tag";
/* Sempahore used to transfer data from adc_measure_task to thingspea_task */
static SemaphoreHandle_t xSemaphoreADC;
static uint16_t xSemaphoreADC_value[4];
/* Event which is used for timer to control the frequency of adc */
static EventGroupHandle_t xEventGroupADC;

/*********** Timer Function *********************/

/**
 * @brief Timer group0 ISR handler
 *
 * @note:
 * We don't call the timer API here because they are not declared with IRAM_ATTR.
 * If we're okay with the timer irq not being serviced while SPI flash cache is disabled,
 * we can allocate this interrupt without the ESP_INTR_FLAG_IRAM flag and use the normal API.
 * 
 * Using IRAM_ATTR to define code in RAM. Note that ESP has MMU (Memory Mapping Unit) to
 * map some address from RAM that can be read by instruction bus.
 * 
 * Code will be stored in .dram.text, not .text or .data and is copied into RAM instead of flash.
 * Flash access is slower than RAM access  => store interrupt in RAM to make it execute quickly
 */

void IRAM_ATTR timer_group0_isr(void* para) {
    /* Set Bit in Event group to notify that timer has expired */
    xEventGroupSetBits(xEventGroupADC, TIMER_FINISH_BIT);
    //Put timer into protect
    timer_spinlock_take(TIMER_GROUP_0);
    //Clear flag status
    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);

    /* After the alarm has been triggered
      we need enable it again, so it is triggered the next time */
    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);
    timer_spinlock_give(TIMER_GROUP_0);
}

void group0_timer_init(int timer_idx, double time_interval_sec) {
    /* Select and initialize basic parameters of the timer */
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = 1            //enable auto reload
    };
    //default clock source is APB, if want to select xtal => set .clk_src = TIMER_SRC_CLK_XTAL
    timer_init(TIMER_GROUP_0, timer_idx, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(TIMER_GROUP_0, timer_idx, 0);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(TIMER_GROUP_0, timer_idx, time_interval_sec*TIMER_SCALE);
    timer_enable_intr(TIMER_GROUP_0, timer_idx);
    timer_isr_register(TIMER_GROUP_0, timer_idx, timer_group0_isr, (void*) timer_idx, ESP_INTR_FLAG_IRAM, NULL);

    //Start timer
    timer_start(TIMER_GROUP_0, timer_idx);
}

/************* END TIMER FUNCTION ********************/

/*
 *  @brief: this task will read adc value and write to sd card
 *
 */

void adc_measure_task(void* pvParameters) {
    /*  Create Semaphore */
    xSemaphoreADC = xSemaphoreCreateBinary();
    if(xSemaphoreADC == NULL) {
        ESP_LOGE(TAG, "There is not enough Heap Memory for Semaphore");
    }

    /* Create Event Group Bit */
    xEventGroupADC = xEventGroupCreate();
    if(xEventGroupADC == NULL) {
        ESP_LOGE(TAG, "There is not enough heap memory for Event Group");
    }

    uint32_t voltage[4];            //stored value of 4 analog channel
    uint8_t digital_value;          //bit 0: channel 0 ...

    esp_adc_cal_characteristics_t characteristic;       //store description of adc
    FILE* file;
    sdmmc_card_t* card;                                 //store information about sd card
    static EventBits_t bits;           //bit used to mark event of adc timer period

    /* Start init ADC and DI */    
    user_adc_init(&characteristic);
    user_digital_input_init();
    /* Finish init ADC and DI*/

    /* Start init SD card */
#ifndef USE_SPI_MODE
    //if use SDMMC
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    user_sdmmc_card_init(&card, host);
#else
    //use SPI
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    user_sdspi_card_init(&card, host);
#endif

    sdmmc_card_print_info(stdout, card);        //print card properties
//     /* Finish init SD card */

    group0_timer_init(0, ADC_PERIOD);
    while(1) {
        /* Start Measure ADC */
    
        //waiting until timer has expired => TIMER_FINISH_BIT is set
        bits = xEventGroupWaitBits(xEventGroupADC, TIMER_FINISH_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
        /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
         * happened. */
        if(bits & TIMER_FINISH_BIT) {
            //finish one period => get adc value
            esp_adc_cal_get_voltage(ADC_CHAN_0, &characteristic, &voltage[0]);
            esp_adc_cal_get_voltage(ADC_CHAN_1, &characteristic, &voltage[1]);
            esp_adc_cal_get_voltage(ADC_CHAN_2, &characteristic, &voltage[2]);
            esp_adc_cal_get_voltage(ADC_CHAN_3, &characteristic, &voltage[3]);
            ESP_LOGI(TAG, "ADC Channel 0 measures: %d mV", voltage[0]);
            ESP_LOGI(TAG, "ADC Channel 1 measures: %d mV", voltage[1]);
            ESP_LOGI(TAG, "ADC Channel 2 measures: %d mV", voltage[2]);
            ESP_LOGI(TAG, "ADC Channel 3 measures: %d mV", voltage[3]);

            //now read digital value
            digital_value = user_read_digital_channel();
            ESP_LOGI(TAG, "Digital Value: %x", digital_value);
        }
        else continue;          //if TIMER_FINISH_BIT is not set, return to start

        //Checking semaphore
        if(xSemaphoreADC != NULL) {
            if(uxSemaphoreGetCount(xSemaphoreADC) == 0) {
                //Semaphore has not been taken yet => take semaphore to edit value
                xSemaphoreTake(xSemaphoreADC, 0);
            }
            /*  Start access to resources safely  */
            for(uint8_t i = 0; i < 4; i++) {
                xSemaphoreADC_value[i] = voltage[i];
            }
            /*  Finish editing value  */

            xSemaphoreGive(xSemaphoreADC);          //release Semaphore
        }
        //After finish, clear bits and restart timer
        xEventGroupClearBits(xEventGroupADC, TIMER_FINISH_BIT);
        /* Finish Measure ADC */


        file = fopen(MOUNT_POINT"/record.txt", "a+");
        if(file == NULL) {
            ESP_LOGE(TAG, "Cannot open file.");
            continue;
        }
        else ESP_LOGI(TAG, "Open file successfully.");
        /* Start writing to file */
        user_file_record_value(file, &voltage[0], digital_value);
        /* Finish writing to file */
        fclose(file);

        
        //vTaskDelay(200/portTICK_RATE_MS);
    }
}

void update_thingspeak(void* pvParameters) {
    vTaskDelay(10000/portTICK_RATE_MS);
    uint8_t new_val = 0;            //used to mark that new data has been received from Semaphore
    field_value_t thingspeak_data = {
        .field_val1 = 0,
        .field_val2 = 0,
        .field_val3 = 0,
        .field_val4 = 0
    };

    while(1) {
        /*   Try to access to semaphore   */
        if(xSemaphoreADC != NULL) {
            /* Check if Semaphore can be taken, if not, wait for 10 ticks before taking */
            if(xSemaphoreTake(xSemaphoreADC, (TickType_t)10) == pdTRUE) {
                /* Semaphore is taken, resource is able to be accessed */
                thingspeak_data.field_val1 = xSemaphoreADC_value[0];
                thingspeak_data.field_val2 = xSemaphoreADC_value[1];
                thingspeak_data.field_val3 = xSemaphoreADC_value[2];
                thingspeak_data.field_val4 = xSemaphoreADC_value[3];
                ESP_LOGI(TAG, "Received data in semaphore");
                new_val = 1;
                xSemaphoreGive(xSemaphoreADC);              //finish taking value, release semaphore
            }
        }
        if(new_val) {
            // ESP_LOGI(TAG,"Start post data to thingspeak");
            printf("ThingSpeak data: %d %d %d %d\n", thingspeak_data.field_val1,thingspeak_data.field_val2,thingspeak_data.field_val3,thingspeak_data.field_val4);
            esp_thingspeak_post(&thingspeak_data);
            new_val = 0;
        }
        // esp_thingspeak_post(thingspeak_data);
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(wifi_connect());
    xTaskCreatePinnedToCore(&adc_measure_task, "adc task", 4096, NULL, ESP_TASKD_EVENT_PRIO-1, NULL, 0);
    xTaskCreatePinnedToCore(&update_thingspeak, "thingspeak task", 8192, NULL, ESP_TASK_PRIO_MAX, NULL, 1);
}
