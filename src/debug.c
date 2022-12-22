#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "debug.h"


char *data_to_string(const byte *data, size_t size) {
    return data_to_stringv(1, (const byte*[]){ data }, (size_t[]){ size });
}


char *text_to_string(const byte *data, size_t size) {
    return text_to_stringv(1, (const byte*[]){ data }, (size_t[]){ size });
}


char *binary_to_string(const byte *data, size_t size) {
    return binary_to_stringv(1, (const byte*[]){ data }, (size_t[]){ size });
}


bool is_human_readable(uint8_t n, const byte **datas, size_t *sizes) {
    int i, j;

    size_t lookahead = 20;

    int readable_chars = 0;
    for (j=0; j<n && lookahead > 0; j++) {
        const byte *data = datas[j];
        for (i=0; i<sizes[j] && lookahead > 0; i++, lookahead--) {
            byte c = data[i];
            if ((c >= 32 && c < 128) || (c == '\r') || (c == '\n'))
                readable_chars++;
        }
    }

    return readable_chars >= 15;
}


char *text_to_stringv(uint8_t n, const byte **datas, size_t *sizes) {
    int i, j;

    size_t buffer_size = 1; // 1 char for eos
    for (j=0; j<n; j++) {
        const byte *data = datas[j];
        for (i=0; i<sizes[j]; i++)
            buffer_size += (data[i] == '\\') ? 2 : ((data[i] >= 32 && data[i] < 128) ? 1 : 4);
    }

    char *buffer = malloc(buffer_size);
    if (!buffer)
        return NULL;

    int pos = 0;
    for (j=0; j<n; j++) {
        const byte *data = datas[j];
        size_t size = sizes[j];
        for (i=0; i<size; i++) {
            if (data[i] == '\\') {
                buffer[pos++] = '\\';
                buffer[pos++] = '\\';
            } else if (data[i] < 32 || data[i] >= 128) {
                size_t printed = snprintf(&buffer[pos], buffer_size - pos, "\\x%02X", data[i]);
                if (printed > 0) {
                    pos += printed;
                }
            } else {
                buffer[pos++] = data[i];
            }
        }
    }
    buffer[pos] = 0;

    return buffer;
}


char *binary_to_stringv(uint8_t n, const byte **datas, size_t *sizes) {
    int i, j;

    size_t buffer_size = 0;
    for (j=0; j<n; j++) {
        buffer_size += sizes[j];
    }
    buffer_size = buffer_size * 4 + 1; // 4 chars for hex code of each char + 1 for eos

    char *buffer = malloc(buffer_size);
    if (!buffer)
        return NULL;

    int pos = 0;
    for (j=0; j<n; j++) {
        const byte *data = datas[j];
        size_t size = sizes[j];
        for (i=0; i<size; i++) {
            size_t printed = snprintf(&buffer[pos], buffer_size - pos, "\\x%02X", data[i]);
            if (printed > 0) {
                pos += printed;
            }
        }
    }
    buffer[pos] = 0;

    return buffer;
}


char *data_to_stringv(uint8_t n, const byte **datas, size_t *sizes) {
    return (is_human_readable(n, datas, sizes)) ?
        text_to_stringv(n, datas, sizes) : binary_to_stringv(n, datas, sizes);
}


void print_binary(const char *prompt, const byte *data, size_t size) {
#ifdef HOMEKIT_DEBUG
    char *buffer = binary_to_string(data, size);
    printf("%s (%d bytes): \"%s\"\n", prompt, (int)size, buffer);
    free(buffer);
#endif
}
