
#include "user_adc.h"

static const char* TAG = "adc";

void user_adc_check_efuse(void) {
    //Check if two point is burned into eFuse
    if(esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        ESP_LOGE(TAG, "eFuse Two Point: Supported");
    } else {
        ESP_LOGE(TAG, "eFuse Two Point: Not Supported");
    }

    //Check if Vref is burned into eFuse
    if(esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        ESP_LOGE(TAG, "eFuse Vref: Supported");
    }
    else {
        ESP_LOGE(TAG, "eFuse Vref: Not Supported");
    }
}

void user_adc_print_val_type(esp_adc_cal_value_t val_type) {
    if(val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        ESP_LOGE(TAG, "Characterized using Two Point");
    }
    else if(val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        ESP_LOGE(TAG, "Characterized using Vref");
    }
    else {
        ESP_LOGE(TAG, "Characterized using Default Vref");
    }
}

void user_adc_init(esp_adc_cal_characteristics_t* characteristic) {
    adc1_config_width(ADC_WIDTH_12Bit);

    adc1_config_channel_atten(ADC_CHAN_0, ADC_ATTEN_11db);
    adc1_config_channel_atten(ADC_CHAN_1, ADC_ATTEN_11db);
    adc1_config_channel_atten(ADC_CHAN_2, ADC_ATTEN_11db);
    adc1_config_channel_atten(ADC_CHAN_3, ADC_ATTEN_11db);
    //adc1_config_channel_atten(ADC1_CHAN_1, ADC_ATTEN_11db);
    //adc1_config_channel_atten(ADC1_CHAN_2, ADC_ATTEN_11db);

    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, VREF, &(*characteristic));
    user_adc_print_val_type(val_type);
}

void user_digital_input_init(void) {
    gpio_pad_select_gpio(DIGITAL_CHAN_0);
    gpio_pad_select_gpio(DIGITAL_CHAN_1);
    gpio_pad_select_gpio(DIGITAL_CHAN_2);
    gpio_pad_select_gpio(DIGITAL_CHAN_3);

    gpio_set_direction(DIGITAL_CHAN_0, GPIO_MODE_INPUT);
    gpio_set_direction(DIGITAL_CHAN_1, GPIO_MODE_INPUT);
    gpio_set_direction(DIGITAL_CHAN_2, GPIO_MODE_INPUT);
    gpio_set_direction(DIGITAL_CHAN_3, GPIO_MODE_INPUT);

    gpio_set_pull_mode(DIGITAL_CHAN_0, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(DIGITAL_CHAN_1, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(DIGITAL_CHAN_2, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(DIGITAL_CHAN_3, GPIO_PULLDOWN_ONLY);
}

uint8_t user_read_digital_channel(void) {
    uint8_t val = 0;            //use bit 0 - bit 3 to store state of digital input
    val |= gpio_get_level(DIGITAL_CHAN_0) << 0;
    val |= gpio_get_level(DIGITAL_CHAN_1) << 1;
    val |= gpio_get_level(DIGITAL_CHAN_2) << 2;
    val |= gpio_get_level(DIGITAL_CHAN_3) << 3;
    return val;
}
