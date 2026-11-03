#ifndef CABINET_CONTROLLER_H
#define CABINET_CONTROLLER_H
#include "cabinet/client.h"
#include "cabinet/config.h"
#include "cabinet/event.h"
#include "cabinet/exchange.h"
#include "cabinet/journal.h"
#include "cabinet/update.h"

typedef struct
{
    void *context;
    cab_send_fn serial_send;
    cab_unlock_fn unlock;
    bool (*set_power)(void *, unsigned, uint16_t);
    cab_nv_read_fn nv_read;
    cab_nv_write_fn nv_write;
    cab_image_write_fn image_write;
    cab_image_read_fn image_read;
} cab_ports_t;

typedef struct
{
    cab_bay_t bays[CAB_BAYS];
    cab_config_t config;
    cab_client_t client;
    cab_store_t store;
    cab_exchange_t exchange;
    cab_update_t update;
    cab_events_t events;
    cab_journal_t journal;
    cab_ports_t ports;
    bool safe_latched;
    uint32_t steps;
    uint32_t now_ms;
    uint32_t cursor_ms;
    unsigned power_cursor;
    cab_result_t last_command;
} cab_controller_t;

cab_result_t cab_controller_init(cab_controller_t *controller, const cab_ports_t *ports);
bool cab_controller_post(cab_controller_t *controller, const cab_event_t *event);
void cab_controller_receive(cab_controller_t *controller, const uint8_t *data, size_t size,
                            uint32_t now);
void cab_controller_step(cab_controller_t *controller, uint32_t now);
cab_result_t cab_controller_configure(cab_controller_t *controller, const cab_config_t *config);
cab_result_t cab_controller_update_begin(cab_controller_t *controller, uint32_t size, uint32_t crc,
                                         uint32_t now);
cab_result_t cab_controller_update_chunk(cab_controller_t *controller, uint32_t offset,
                                         const uint8_t *data, size_t size, uint32_t now);
cab_result_t cab_controller_update_finish(cab_controller_t *controller);
#endif
