
#pragma once
#include <stddef.h>
typedef struct{
    int (*send)(const void*,size_t);
    int (*recv)(void*,size_t);
    int (*close)(void);
}hap_transport_t;
