#include "host_port.h"
#include "cabinet/sample.h"
#include <string.h>

static bool serial_send(void *context, const uint8_t *data, size_t size)
{
    host_port_t *port = context;
    uint16_t registers[CAB_SAMPLE_REGS];
    unsigned bay;
    if (size != 8u || !cab_rtu_valid(data, size) || data[1] != 3u || cab_get_be16(data + 2) != 0u ||
        cab_get_be16(data + 4) != CAB_SAMPLE_REGS || data[0] == 0u || data[0] > CAB_BAYS)
        return false;
    bay = data[0] - 1u;
    if (port->offline[bay])
        return true;
    if (port->response_size)
        return false;
    cab_sample_encode(&port->samples[bay], registers);
    if (cab_rtu_response(data[0], registers, CAB_SAMPLE_REGS, port->response,
                         sizeof(port->response), &port->response_size) != CAB_OK)
        return false;
    if (port->corrupt_reply)
        port->response[port->response_size - 1u] ^= 1u;
    port->due_ms = port->now_ms + 10u;
    return true;
}

static bool unlock(void *context, unsigned bay)
{
    host_port_t *port = context;
    if (bay >= CAB_BAYS || port->unlock_failed || port->power[bay] != 0u)
        return false;
    port->unlocks++;
    return true;
}

static bool set_power(void *context, unsigned bay, uint16_t watts)
{
    host_port_t *port = context;
    if (bay >= CAB_BAYS || port->output_failed)
        return false;
    port->power[bay] = watts;
    return true;
}

static bool nv_read(void *context, unsigned slot, uint8_t *data, size_t size)
{
    host_port_t *port = context;
    if (slot > 1u || size != CAB_CONFIG_BYTES)
        return false;
    memcpy(data, port->nv[slot], size);
    return true;
}

static bool nv_write(void *context, unsigned slot, const uint8_t *data, size_t size)
{
    host_port_t *port = context;
    if (slot > 1u || size != CAB_CONFIG_BYTES)
        return false;
    port->writes++;
    memset(port->nv[slot], 0xff, size);
    if (port->torn_write)
    {
        memcpy(port->nv[slot], data, size / 2u);
        return false;
    }
    memcpy(port->nv[slot], data, size);
    return true;
}

static bool image_write(void *context, uint32_t offset, const uint8_t *data, size_t size)
{
    host_port_t *port = context;
    if (port->image_failed || offset > CAB_IMAGE_MAX || size > CAB_IMAGE_MAX - offset)
        return false;
    memcpy(port->image + offset, data, size);
    return true;
}

static bool image_read(void *context, uint32_t offset, uint8_t *data, size_t size)
{
    host_port_t *port = context;
    if (port->image_failed || offset > CAB_IMAGE_MAX || size > CAB_IMAGE_MAX - offset)
        return false;
    memcpy(data, port->image + offset, size);
    return true;
}

void host_port_init(host_port_t *port)
{
    unsigned i;
    memset(port, 0, sizeof(*port));
    memset(port->nv, 0xff, sizeof(port->nv));
    for (i = 0; i < CAB_BAYS; ++i)
    {
        cab_sample_t sample = {4800u, 0, 450u, 250, 0u, 300u};
        port->samples[i] = sample;
        port->closed[i] = true;
    }
}

cab_ports_t host_port_bind(host_port_t *port)
{
    cab_ports_t ports = {port,    serial_send, unlock,      set_power,
                         nv_read, nv_write,    image_write, image_read};
    return ports;
}

void host_observe(host_port_t *port, cab_controller_t *controller, unsigned bay, bool present,
                  bool closed)
{
    cab_event_t event = {CAB_EVENT_OBSERVATION, bay, 0u, present, closed};
    if (bay >= CAB_BAYS)
        return;
    port->present[bay] = present;
    port->closed[bay] = closed;
    cab_controller_post(controller, &event);
}

void host_pump(host_port_t *port, cab_controller_t *controller, uint32_t now)
{
    port->now_ms = now;
    if (port->response_size && (uint32_t)(now - port->due_ms) < UINT32_C(0x80000000))
    {
        size_t size = port->response_size;
        port->response_size = 0u;
        cab_controller_receive(controller, port->response, 3u, now);
        cab_controller_receive(controller, port->response + 3u, size - 3u, now);
    }
    cab_controller_step(controller, now);
}
