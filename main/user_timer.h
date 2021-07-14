/*
 *   Group hardware timer
 *   There are 2 groups of general purpose timer with 64 bits in esp32
 *   Each group contains 2 timer
 */

#ifndef _USER_TIMER_H_
#define _USER_TIMER_H_

#include <stdio.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "freertos/event_groups.h"

#define TIMER_DIVIDER   16      // Hardware timer clock divider
#define TIMER_SCALE     (TIMER_BASE_CLK / TIMER_DIVIDER)    // convert counter value to second
//TIMER_BASE_CLK = 40MHz
#define ADC_PERIOD      2       //seconds        
#define TIMER_FINISH_BIT BIT0

/*
 *  A sample structure to pass data back
 *  from timer interrupt to main program
 *  To use, create a data queue using xQueueCreate 
 *  and push back data from ISR using xQueueSendFromISR()
 *  and receive data from main program use xQueueReceive
 */

typedef struct {
    int type;           //type of timer events
    int timer_group;
    int timer_idx;
    uint64_t timer_counter_value;
} timer_event_t;

// xQueueHandle timer_queue;

/**
 * @brief Configure timer of group 0
 * 
 * @param timer_idx index of timer in group 0 (0 or 1)
 * @param timer_interval_sec time interval that timer will count, unit is second
 *
 */

void group0_timer_init(int timer_idx, double timer_interval_sec);

#endif
