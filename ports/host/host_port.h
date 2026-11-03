#ifndef CABINET_HOST_PORT_H
#define CABINET_HOST_PORT_H
#include "cabinet/controller.h"

typedef struct
{
    cab_sample_t samples[CAB_BAYS];
    bool present[CAB_BAYS];
    bool closed[CAB_BAYS];
    bool offline[CAB_BAYS];
    bool unlock_failed;
    bool output_failed;
    bool image_failed;
    bool corrupt_reply;
    bool torn_write;
    uint8_t nv[2][CAB_CONFIG_BYTES];
    uint8_t image[CAB_IMAGE_MAX];
    uint8_t response[CAB_RTU_MAX];
    size_t response_size;
    uint32_t due_ms;
    uint32_t now_ms;
    uint16_t power[CAB_BAYS];
    uint32_t unlocks;
    uint32_t writes;
} host_port_t;

void host_port_init(host_port_t *port);
cab_ports_t host_port_bind(host_port_t *port);
void host_observe(host_port_t *port, cab_controller_t *controller, unsigned bay, bool present,
                  bool closed);
void host_pump(host_port_t *port, cab_controller_t *controller, uint32_t now);
#endif
