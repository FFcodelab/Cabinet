#include "cabinet/event.h"

bool cab_events_push(cab_events_t *events, const cab_event_t *event)
{
    unsigned tail;
    if (events->count == CAB_EVENT_CAPACITY)
    {
        events->dropped++;
        return false;
    }
    tail = (events->head + events->count) % CAB_EVENT_CAPACITY;
    events->items[tail] = *event;
    events->count++;
    return true;
}

bool cab_events_pop(cab_events_t *events, cab_event_t *event)
{
    if (!events->count)
        return false;
    *event = events->items[events->head];
    events->head = (events->head + 1u) % CAB_EVENT_CAPACITY;
    events->count--;
    return true;
}
