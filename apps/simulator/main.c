#include "host_port.h"
#include "report.h"
#include "cabinet/checksum.h"
#include <string.h>
static cab_controller_t controller;
static host_port_t host;
static cab_report_t report;

int main(int argc, char **argv)
{
    cab_ports_t ports;
    uint32_t now;
    uint8_t image[128];
    unsigned i;
    bool fault_scenario;
    if (argc != 3 || (strcmp(argv[1], "normal") != 0 && strcmp(argv[1], "fault") != 0))
        return 2;
    fault_scenario = strcmp(argv[1], "fault") == 0;
    host_port_init(&host);
    ports = host_port_bind(&host);
    if (cab_controller_init(&controller, &ports) != CAB_OK)
        return 3;
    host.samples[1].soc_permille = 950u;
    for (i = 0; i < CAB_BAYS; ++i)
        host_observe(&host, &controller, i, i == 1u || i == 2u, true);
    for (i = 0; i < sizeof(image); ++i)
        image[i] = (uint8_t)(i * 7u + 3u);
    cab_report_begin(&report);
    for (now = 0u; now <= 8000u; now += 10u)
    {
        if (now == 500u)
        {
            cab_event_t event = {CAB_EVENT_EXCHANGE, 0u, 1u, false, false};
            if (!cab_controller_post(&controller, &event))
                return 4;
        }
        if (now == 550u)
            host_observe(&host, &controller, 0u, false, false);
        if (now == 600u)
            host_observe(&host, &controller, 0u, true, false);
        if (now == 650u)
            host_observe(&host, &controller, 0u, true, true);
        if (now == 700u)
            host_observe(&host, &controller, 1u, true, false);
        if (now == 750u)
            host_observe(&host, &controller, 1u, false, false);
        if (now == 800u)
            host_observe(&host, &controller, 1u, false, true);
        if (now == 2000u && fault_scenario)
            host.offline[2] = true;
        if (now == 4000u && fault_scenario)
            host.offline[2] = false;
        if (now == 4500u &&
            cab_controller_update_begin(&controller, sizeof(image), cab_crc32(image, sizeof(image)),
                                        now) != CAB_OK)
            return 5;
        if (now == 4600u && cab_controller_update_chunk(&controller, 0u, image, 64u, now) != CAB_OK)
            return 6;
        if (now == 4700u &&
            cab_controller_update_chunk(&controller, 64u, image + 64u, 64u, now) != CAB_OK)
            return 7;
        if (now == 4800u && cab_controller_update_finish(&controller) != CAB_OK)
            return 8;
        if (now == 5000u)
        {
            cab_config_t config = controller.config;
            config.budget_w = 400u;
            if (cab_controller_configure(&controller, &config) != CAB_OK)
                return 9;
        }
        host_pump(&host, &controller, now);
        if (now % 50u == 0u)
            cab_report_sample(&report, &controller);
    }
    if (controller.exchange.completed != 1u || controller.update.state != CAB_UPDATE_READY ||
        controller.safe_latched || host.unlocks != 2u)
        return 10;
    return cab_report_finish(&report, &controller, argv[2]) ? 0 : 11;
}
