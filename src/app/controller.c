#include "cabinet/controller.h"
#include "cabinet/power.h"
#include "cabinet/sample.h"
#include <string.h>

static void force_off(cab_controller_t *c)
{
    unsigned i;
    for (i = 0; i < CAB_BAYS; ++i)
    {
        c->bays[i].output_w = 0u;
        if (!c->ports.set_power(c->ports.context, i, 0u))
            c->safe_latched = true;
    }
}

static bool send_frame(void *context, const uint8_t *data, size_t size)
{
    cab_controller_t *c = context;
    return c->ports.serial_send(c->ports.context, data, size);
}

static bool unlock_bay(void *context, unsigned bay)
{
    cab_controller_t *c = context;
    c->bays[bay].output_w = 0u;
    if (!c->ports.set_power(c->ports.context, bay, 0u))
    {
        c->safe_latched = true;
        force_off(c);
        return false;
    }
    return c->ports.unlock(c->ports.context, bay);
}

static void reply(void *context, unsigned bay, cab_result_t result, const uint16_t *registers,
                  uint32_t now)
{
    cab_controller_t *c = context;
    cab_bay_t *b = &c->bays[bay];
    bool was_valid = b->sample_valid;
    if (result == CAB_OK)
        result = cab_sample_decode(registers, &b->sample);
    b->sample_valid = result == CAB_OK && b->present;
    if (b->sample_valid)
        b->sampled_ms = now;
    else if (was_valid || result != CAB_OK)
        cab_journal_record(&c->journal, now,
                           result == CAB_INVALID ? CAB_FAULT_SAMPLE : CAB_FAULT_COMM, bay, result);
}

cab_result_t cab_controller_init(cab_controller_t *c, const cab_ports_t *ports)
{
    if (!c || !ports || !ports->set_power || !ports->unlock || !ports->serial_send ||
        !ports->nv_read || !ports->nv_write || !ports->image_read || !ports->image_write)
        return CAB_INVALID;
    memset(c, 0, sizeof(*c));
    c->ports = *ports;
    force_off(c);
    cab_client_init(&c->client, send_frame, reply, c);
    cab_store_init(&c->store, ports->nv_read, ports->nv_write, ports->context);
    if (cab_store_load(&c->store, &c->config) != CAB_OK)
        cab_journal_record(&c->journal, 0u, CAB_FAULT_CONFIG, CAB_BAYS, CAB_EMPTY);
    return c->safe_latched ? CAB_IO : CAB_OK;
}

bool cab_controller_post(cab_controller_t *c, const cab_event_t *event)
{
    if (!event ||
        (event->kind != CAB_EVENT_OBSERVATION && event->kind != CAB_EVENT_EXCHANGE &&
         event->kind != CAB_EVENT_RESET) ||
        (event->kind != CAB_EVENT_RESET && event->bay >= CAB_BAYS) ||
        (event->kind == CAB_EVENT_EXCHANGE && event->peer >= CAB_BAYS))
        return false;
    if (!cab_events_push(&c->events, event))
    {
        c->safe_latched = true;
        cab_journal_record(&c->journal, c->now_ms, CAB_FAULT_QUEUE, CAB_BAYS, CAB_FULL);
        force_off(c);
        return false;
    }
    return true;
}

void cab_controller_receive(cab_controller_t *c, const uint8_t *data, size_t size, uint32_t now)
{
    cab_client_receive(&c->client, data, size, now);
}

static void process_event(cab_controller_t *c, const cab_event_t *event, uint32_t now)
{
    cab_bay_t *bay;
    switch (event->kind)
    {
    case CAB_EVENT_OBSERVATION:
        bay = &c->bays[event->bay];
        if (bay->present != event->present)
            bay->sample_valid = false;
        bay->present = event->present;
        bay->door_closed = event->door_closed;
        c->last_command = CAB_OK;
        break;
    case CAB_EVENT_EXCHANGE:
        if (c->safe_latched || c->update.state == CAB_UPDATE_RECEIVING)
            c->last_command = CAB_BUSY;
        else
            c->last_command = cab_exchange_start(&c->exchange, event->bay, event->peer, c->bays,
                                                 &c->config, now, unlock_bay, c);
        break;
    case CAB_EVENT_RESET:
        c->last_command = cab_exchange_reset(&c->exchange, c->bays);
        break;
    }
}

void cab_controller_step(cab_controller_t *c, uint32_t now)
{
    cab_event_t event;
    uint16_t demand[CAB_BAYS], allocated[CAB_BAYS];
    cab_exchange_state_t previous = c->exchange.state;
    cab_update_state_t update_previous = c->update.state;
    unsigned i;
    c->now_ms = now;
    for (i = 0; i < CAB_EVENT_CAPACITY && cab_events_pop(&c->events, &event); ++i)
    {
        process_event(c, &event, now);
        cab_exchange_tick(&c->exchange, c->bays, &c->config, now, unlock_bay, c);
    }
    cab_client_tick(&c->client, now);
    cab_exchange_tick(&c->exchange, c->bays, &c->config, now, unlock_bay, c);
    cab_update_tick(&c->update, now);
    if (c->exchange.state == CAB_EX_FAILED && previous != CAB_EX_FAILED)
        cab_journal_record(&c->journal, now, CAB_FAULT_EXCHANGE, CAB_BAYS, c->exchange.error);
    if (c->update.state == CAB_UPDATE_FAILED && update_previous != CAB_UPDATE_FAILED)
        cab_journal_record(&c->journal, now, CAB_FAULT_UPDATE, CAB_BAYS, c->update.error);
    if (cab_elapsed(now, c->cursor_ms, 1000u))
    {
        c->power_cursor = (c->power_cursor + 1u) % CAB_BAYS;
        c->cursor_ms = now;
    }
    for (i = 0; i < CAB_BAYS; ++i)
        demand[i] = cab_charge_demand(&c->bays[i], &c->config, now,
                                      c->safe_latched || c->update.state == CAB_UPDATE_RECEIVING ||
                                          cab_exchange_reserved(&c->exchange, i));
    cab_power_allocate(demand, c->config.budget_w, c->power_cursor, allocated);
    for (i = 0; i < CAB_BAYS; ++i)
    {
        if (!c->ports.set_power(c->ports.context, i, allocated[i]))
        {
            c->safe_latched = true;
            cab_journal_record(&c->journal, now, CAB_FAULT_OUTPUT, i, CAB_IO);
            break;
        }
        c->bays[i].output_w = allocated[i];
    }
    if (c->safe_latched)
        force_off(c);
    c->steps++;
}

cab_result_t cab_controller_configure(cab_controller_t *c, const cab_config_t *config)
{
    cab_result_t result;
    if (cab_exchange_active(&c->exchange) || c->update.state == CAB_UPDATE_RECEIVING)
        return CAB_BUSY;
    result = cab_store_save(&c->store, config);
    if (result == CAB_OK)
    {
        c->config = *config;
        force_off(c);
    }
    else
        cab_journal_record(&c->journal, c->now_ms, CAB_FAULT_CONFIG, CAB_BAYS, result);
    return result;
}

cab_result_t cab_controller_update_begin(cab_controller_t *c, uint32_t size, uint32_t crc,
                                         uint32_t now)
{
    cab_result_t result;
    if (c->safe_latched)
        return CAB_BUSY;
    result = cab_update_begin(&c->update, size, crc, cab_exchange_active(&c->exchange), now);
    if (result == CAB_OK)
    {
        force_off(c);
        if (c->safe_latched)
        {
            cab_update_cancel(&c->update);
            return CAB_IO;
        }
    }
    return result;
}

cab_result_t cab_controller_update_chunk(cab_controller_t *c, uint32_t offset, const uint8_t *data,
                                         size_t size, uint32_t now)
{
    cab_result_t result = cab_update_chunk(&c->update, offset, data, size, now,
                                           c->ports.image_write, c->ports.context);
    if (result != CAB_OK)
        cab_journal_record(&c->journal, now, CAB_FAULT_UPDATE, CAB_BAYS, result);
    return result;
}

cab_result_t cab_controller_update_finish(cab_controller_t *c)
{
    cab_result_t result = cab_update_finish(&c->update, c->ports.image_read, c->ports.context);
    if (result != CAB_OK)
        cab_journal_record(&c->journal, c->now_ms, CAB_FAULT_UPDATE, CAB_BAYS, result);
    return result;
}
