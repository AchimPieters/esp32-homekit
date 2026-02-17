
#pragma once
#include <stddef.h>
typedef struct{
    size_t parsed;
    int state;
}hap_stream_t;
void hap_stream_init(hap_stream_t*);
int hap_stream_feed(hap_stream_t*,const void*,size_t);
