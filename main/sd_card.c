
/* Source file for SD card */
#include "sd_card.h"

static const char* TAG = "SD Card";

/*
 *  @brief: function used to mount SD card to ESP32  
 *  @param:
 *      card: pointer   store information returned by SD card
 *      host:           Pointer to structure describing SDMMC host. When using
 *                      SDMMC peripheral, this structure can be initialized using
 *                      SDMMC_HOST_DEFAULT() macro. When using SPI peripheral,
 *                      this structure can be initialized using SDSPI_HOST_DEFAULT() macro.
 */ 

esp_err_t user_sdmmc_card_init(sdmmc_card_t** card, sdmmc_host_t host)
{
    ESP_LOGI(TAG, "Initialize SD card using SDMMC peripheral");
    esp_err_t ret;
    sdmmc_card_t* temp_card;                //temp value to store information about SD card
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,                    //do not format SD card if unable to mount
        .max_files = 5,                                     //can only open 5 files at the same time
        .allocation_unit_size = 16 * 1024                   //dont care if format_if_mount_failed is false
    };

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    // To use 1-line SD mode, uncomment the following line:
    // slot_config.width = 1;

    // GPIOs 15, 2, 4, 12, 13 (used for SDMMC) should have external 10k pull-ups.
    // Internal pull-ups are not sufficient. However, enabling internal pull-ups
    // does make a difference some boards, so we do that here.
    gpio_set_pull_mode(15, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
    gpio_set_pull_mode(2, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
    gpio_set_pull_mode(4, GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
    gpio_set_pull_mode(12, GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
    gpio_set_pull_mode(13, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes

    ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &(temp_card));
    if(ret != ESP_OK) {
        if(ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount file system");
        }
        else {
            ESP_LOGE(TAG, "Fail to initialize the card (%s). "
                "Make sure SD card lines have pull-up resistors in place", esp_err_to_name(ret));
        }
    }
    *card = temp_card;
    return ret;
}

esp_err_t user_sdspi_card_init(sdmmc_card_t** card, sdmmc_host_t host)
{
    ESP_LOGI(TAG, "Initialize SD card using SPI peripheral");
    esp_err_t ret;

    sdmmc_card_t* temp_card;
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,                    //do not format SD card if unable to mount
        .max_files = 5,                                     //can only open 5 files at the same time
        .allocation_unit_size = 16 * 1024                   //dont care if format_if_mount_failed is false
    };
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,                                //write protection line (WP) = -1 => not used
        .quadhd_io_num = -1,                                //hold line (HD)
        .max_transfer_sz = 4000
    };

    ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CHAN);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus");
        return ret;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &(temp_card));
    if(ret != ESP_OK) {
        if(ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount file system");
        }
        else {
            ESP_LOGE(TAG, "Fail to initialize the card (%s). "
                "Make sure SD card lines have pull-up resistors in place", esp_err_to_name(ret));
        }
    }
    *card = temp_card;
    // sdmmc_card_print_info(stdout, &card);        //print card properties
    return ret;    
}

void user_sd_card_umount(sdmmc_card_t* card, sdmmc_host_t* host) {
    //unmount partition and disable SDMMC or SPI peripheral
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    ESP_LOGI(TAG, "Card umounted");
#ifndef USE_SPI_MODE
    //deinitialize SPI bus
    spi_bus_free(host->slot);
#endif
}

/**** File operation ****/

bool user_file_check_exist(const char* path) {
    struct stat st;
    return (bool)(stat(path, &st));
}

void user_file_delete(const char* path) {
    unlink(path);
}

bool user_file_rename(const char* old_path, const char* new_path) {
    if(rename(old_path, new_path) == 0) {
        ESP_LOGE(TAG, "Rename failed");
        return false;
    }
    else {
        ESP_LOGI(TAG, "Rename success");
        return true;
    }
}

void user_file_move_pointer(FILE* f, int pos) {
    fseek(f, pos, SEEK_SET);
}

void user_file_read(FILE* f, int size, char* buf) {
    fgets(buf, size, f);
}

uint64_t user_file_get_size(FILE* f) {
    fseek(f, 0, SEEK_END);
    uint64_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    return size;
}

void user_file_record_value(FILE* f, uint32_t* adc_val, uint8_t digital_val) {
    fprintf(f, "Analog: Chan 0:%dmV, Chan 1:%dmV, Chan 2:%dmV, Chan 3:%d mV.\n", adc_val[0], adc_val[1], adc_val[2], adc_val[3]);
    fprintf(f, "Digtal: Chan 0: %d, Chan 1: %d, Chan 2: %d, Chan 3: %d\n", (digital_val&0x01), (digital_val>>1)&0x01, (digital_val>>2)&0x01, (digital_val>>3)&0x01);
    /*  Note that digital value store state of digital input in bit 0 - 3
     *  we must shift each corresponding bit to bit 0 and read that bit only    */
}
