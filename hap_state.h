
#pragma once
typedef enum{
    HAP_STATE_IDLE,
    HAP_STATE_CONNECTED,
    HAP_STATE_PAIRED
}hap_state_t;

typedef enum{
    HAP_EVENT_CONNECT,
    HAP_EVENT_PAIR,
    HAP_EVENT_DISCONNECT
}hap_event_t;

typedef struct{
    hap_state_t state;
}hap_state_machine_t;

void hap_state_step(hap_state_machine_t*,int);
