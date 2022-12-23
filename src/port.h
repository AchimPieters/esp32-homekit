#pragma once

#include <stdint.h>

uint32_t homekit_random();
void homekit_random_fill(uint8_t *data, size_t size);

void homekit_system_restart();
void homekit_overclock_start();
void homekit_overclock_end();

#ifdef ESP_OPEN_RTOS
#include <spiflash.h>
#define ESP_OK 0
#endif

#ifdef ESP_IDF
#include <esp_system.h>
//#include <esp_spi_flash.h>
#include "spi_flash_mmap.h" // V5
#include "esp_flash.h" // V5

//https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32c3/migration-guides/release-5.x/storage.html
//https://docs.espressif.com/projects/esp-idf/en/v5.0-beta1/esp32/api-reference/storage/spi_flash.html
#define ESP_FLASH_SECTOR_SIZE SPI_FLASH_SEC_SIZE
//esp_err_t esp_flash_read(esp_flash_t *chip, void *buffer, uint32_t address, uint32_t length)
#define esp_flash_read(NULL, buffer, addr, size) (esp_flash_read(NULL, (buffer), (addr), (size)) == ESP_OK)
//esp_err_t esp_flash_write(esp_flash_t *chip, const void *buffer, uint32_t address, uint32_t length)
#define esp_flash_write(NULL, buffer, addr, size) (esp_flash_write(NULL, (buffer), (addr), (size)) == ESP_OK)
//esp_err_t esp_flash_erase_region(esp_flash_t *chip, uint32_t start, uint32_t len)
#define esp_flash_erase_region(NULL, addr, size) (esp_flash_erase_region(NULL, (addr), (size) / ESP_FLASH_SECTOR_SIZE) == ESP_OK)
#endif


#ifdef ESP_IDF
#define SERVER_TASK_STACK 12288
#else
#define SERVER_TASK_STACK 1664
#endif


void homekit_mdns_init();
void homekit_mdns_configure_init(const char *instance_name, int port);
void homekit_mdns_add_txt(const char *key, const char *format, ...);
void homekit_mdns_configure_finalize();
