#ifndef CABINET_TASK_H
#define CABINET_TASK_H
#include "cabinet/controller.h"

typedef struct
{
    uint16_t watts[CAB_BAYS];
    cab_exchange_state_t exchange;
    cab_update_state_t update;
    uint32_t steps;
    uint32_t faults;
    bool safe_latched;
} cab_task_snapshot_t;

bool cabinet_task_start(const cab_ports_t *ports);
bool cabinet_task_post(const cab_event_t *event);
bool cabinet_task_receive_from_isr(const uint8_t *data, size_t size);
bool cabinet_task_snapshot(cab_task_snapshot_t *snapshot);
#endif
