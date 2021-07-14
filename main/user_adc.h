/*
 *  Header file for ADC in ESP32
 *  Note that: in this file, only ADC1 is configured
 *  Config for 3 channel: ADC1 channel 0, 1, 2
 * 
 */ 


#ifndef _USER_ADC_H_
#define _USER_ADC_H_

#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
// #include "esp_partition.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "driver/gpio.h"

#define VREF 1100

#define ADC_CHAN_0 ADC1_CHANNEL_0
#define ADC_CHAN_1 ADC1_CHANNEL_3
#define ADC_CHAN_2 ADC1_CHANNEL_6
#define ADC_CHAN_3 ADC1_CHANNEL_7 

#define DIGITAL_CHAN_0 23
#define DIGITAL_CHAN_1 22
#define DIGITAL_CHAN_2 19
#define DIGITAL_CHAN_3 18

void user_adc_check_efuse(void);
void user_adc_print_val_type(esp_adc_cal_value_t val_type);
void user_adc_init(esp_adc_cal_characteristics_t* characteristic);

void user_digital_input_init(void);
uint8_t user_read_digital_channel(void);

#endif
