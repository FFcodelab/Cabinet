#ifndef CABINET_EVENT_H
#define CABINET_EVENT_H
#include "cabinet/types.h"

typedef enum
{
    CAB_EVENT_OBSERVATION,
    CAB_EVENT_EXCHANGE,
    CAB_EVENT_RESET
} cab_event_kind_t;

typedef struct
{
    cab_event_kind_t kind;
    unsigned bay;
    unsigned peer;
    bool present;
    bool door_closed;
} cab_event_t;

typedef struct
{
    cab_event_t items[CAB_EVENT_CAPACITY];
    unsigned head;
    unsigned count;
    uint32_t dropped;
} cab_events_t;

bool cab_events_push(cab_events_t *events, const cab_event_t *event);
bool cab_events_pop(cab_events_t *events, cab_event_t *event);
#endif
