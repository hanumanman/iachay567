/*
 *  Header file for modifying SD Card using SPI
 *  
 * 
 * 
 */

#ifndef _SD_CARD_H_
#define _SD_CARD_H_

//Private include

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "sdkconfig.h"

#include "driver/sdmmc_host.h"

#define MOUNT_POINT "/sdcard"

//Note that ESP can use SDMMC or SPI peripherals for communicating with SD CARD
//By default, SD MMC is used
//If SPI is used, uncomment the next line

#define USE_SPI_MODE

// ESP32-S2 and ESP32-C3 doesn't have an SD Host peripheral, always use SPI:
#if CONFIG_IDF_TARGET_ESP32S2 ||CONFIG_IDF_TARGET_ESP32C3
#ifndef USE_SPI_MODE
#define USE_SPI_MODE
#endif // USE_SPI_MODE
// on ESP32-S2, DMA channel must be the same as host id
#define SPI_DMA_CHAN    host.slot
#endif //CONFIG_IDF_TARGET_ESP32S2

// DMA channel to be used by the SPI peripheral
#ifndef SPI_DMA_CHAN
#define SPI_DMA_CHAN    1
#endif //SPI_DMA_CHAN

// When testing SD and SPI modes, keep in mind that once the card has been
// initialized in SPI mode, it can not be reinitialized in SD mode without
// toggling power to the card.

#ifdef USE_SPI_MODE
// Pin mapping when using SPI mode.
// With this mapping, SD card can be used both in SPI and 1-line SD mode.
// Note that a pull-up on CS line is required in SD mode.
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
#define PIN_NUM_MISO 2
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   13

#elif CONFIG_IDF_TARGET_ESP32C3
#define PIN_NUM_MISO 18
#define PIN_NUM_MOSI 9
#define PIN_NUM_CLK  8
#define PIN_NUM_CS   19

#endif //CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
#endif //USE_SPI_MODE

/**
 * @brief Initialize MMC peripheral to connect to SD card
 * 
 * @param card store information about SD card
 * @param host store properties of SD card
 * @return esp_err_t 
 */
esp_err_t user_sdmmc_card_init(sdmmc_card_t** card, sdmmc_host_t host);

/**
 * @brief Initialize SPI peripheral to connect to SD card
 * 
 * @param card store information about SD card
 * @param host store properties of SD card
 * @return esp_err_t 
 */
esp_err_t user_sdspi_card_init(sdmmc_card_t** card, sdmmc_host_t host);

void user_sd_card_umount(sdmmc_card_t* card, sdmmc_host_t* host);

bool user_file_check_exist(const char* path);

void user_file_delete(const char* path);

bool user_file_rename(const char* old_path, const char* new_path);

void user_file_move_pointer(FILE* f, int pos);

void user_file_read(FILE* f, int size, char* buf);

uint64_t user_file_get_size(FILE* f);

void user_file_record_value(FILE* f, uint32_t* adc_val, uint8_t digital_val);

#endif