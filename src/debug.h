#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdlib.h>
#include <stdio.h>

typedef unsigned char byte;

#ifdef HOMEKIT_DEBUG

#define DEBUG(message, ...) printf(">>> %s: " message "\n", __func__, ## __VA_ARGS__)

#else

#define DEBUG(message, ...)

#endif

#define INFO(message, ...) printf(">>> HomeKit: " message "\n", ## __VA_ARGS__)
#define ERROR(message, ...) printf("!!! HomeKit: " message "\n", ## __VA_ARGS__)

#ifdef ESP_IDF

#define DEBUG_HEAP() DEBUG("Free heap: %lu", esp_get_free_heap_size());

#else

#define DEBUG_HEAP() DEBUG("Free heap: %lu", xPortGetFreeHeapSize());

#endif

char *data_to_string(const byte *data, size_t size);
char *data_to_stringv(uint8_t n, const byte **datas, size_t *sizes);

char *text_to_string(const byte *data, size_t size);
char *text_to_stringv(uint8_t n, const byte **datas, size_t *sizes);

char *binary_to_string(const byte *data, size_t size);
char *binary_to_stringv(uint8_t n, const byte **datas, size_t *sizes);

void print_binary(const char *prompt, const byte *data, size_t size);

#endif // __DEBUG_H__
