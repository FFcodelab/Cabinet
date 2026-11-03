#include "host_port.h"
#include "cabinet/checksum.h"
#include <assert.h>
#include <string.h>
#ifdef NDEBUG
#error Assertions must remain enabled in tests.
#endif
static void advance(host_port_t *host, cab_controller_t *c, uint32_t first, uint32_t last)
{
    uint32_t now;
    for (now = first; now <= last; now += 10u)
    {
        unsigned i;
        uint32_t sum = 0u;
        host_pump(host, c, now);
        for (i = 0; i < CAB_BAYS; ++i)
        {
            sum += host->power[i];
            assert(host->power[i] <= c->config.bay_limit_w);
            if (!c->bays[i].present || !c->bays[i].door_closed)
                assert(host->power[i] == 0u);
        }
        assert(sum <= c->config.budget_w);
    }
}

int main(void)
{
    host_port_t host;
    cab_controller_t c;
    cab_ports_t ports;
    cab_event_t event = {CAB_EVENT_OBSERVATION, 0u, 0u, true, true};
    uint8_t image[64] = {1u, 2u, 3u};
    unsigned i;
    host_port_init(&host);
    ports = host_port_bind(&host);
    assert(cab_controller_init(&c, &ports) == CAB_OK);
    for (i = 0; i < CAB_BAYS; ++i)
        host_observe(&host, &c, i, true, true);
    advance(&host, &c, 0u, 500u);
    for (i = 0; i < CAB_BAYS; ++i)
        assert(host.power[i] == 200u);
    host_observe(&host, &c, 0u, true, false);
    advance(&host, &c, 510u, 510u);
    assert(host.power[0] == 0u);
    host_observe(&host, &c, 0u, true, true);
    host.offline[2] = true;
    advance(&host, &c, 520u, 2500u);
    assert(host.power[2] == 0u && c.client.timeout_count > 0u);
    host.offline[2] = false;
    advance(&host, &c, 2510u, 3200u);
    assert(host.power[2] > 0u);
    assert(cab_controller_update_begin(&c, sizeof(image), cab_crc32(image, sizeof(image)), 3210u) ==
           CAB_OK);
    for (i = 0; i < CAB_BAYS; ++i)
        assert(host.power[i] == 0u);
    advance(&host, &c, 3210u, 3300u);
    for (i = 0; i < CAB_BAYS; ++i)
        assert(host.power[i] == 0u);
    assert(cab_controller_update_chunk(&c, 0u, image, sizeof(image), 3300u) == CAB_OK);
    assert(cab_controller_update_finish(&c) == CAB_OK);
    advance(&host, &c, 3310u, 3500u);
    assert(host.power[0] > 0u);
    for (i = 0; i < CAB_EVENT_CAPACITY; ++i)
        assert(cab_controller_post(&c, &event));
    assert(!cab_controller_post(&c, &event));
    assert(c.safe_latched);
    for (i = 0; i < CAB_BAYS; ++i)
        assert(host.power[i] == 0u);
    advance(&host, &c, 3510u, 3600u);
    for (i = 0; i < CAB_BAYS; ++i)
        assert(host.power[i] == 0u);
    assert(cab_controller_update_begin(&c, 64u, 0u, 3610u) == CAB_BUSY);
    host_port_init(&host);
    ports = host_port_bind(&host);
    assert(cab_controller_init(&c, &ports) == CAB_OK);
    for (i = 0; i < CAB_BAYS; ++i)
        host_observe(&host, &c, i, i != 0u, true);
    host.samples[1].soc_permille = 950u;
    advance(&host, &c, 0u, 500u);
    event = (cab_event_t){CAB_EVENT_EXCHANGE, 0u, 1u, false, false};
    assert(cab_controller_post(&c, &event));
    host_pump(&host, &c, 510u);
    assert(c.exchange.state == CAB_EX_RETURN_OPEN);
    assert(cab_controller_update_begin(&c, sizeof(image), 0u, 520u) == CAB_BUSY);
    assert(cab_controller_configure(&c, &c.config) == CAB_BUSY);
    host_observe(&host, &c, 0u, false, false);
    host_observe(&host, &c, 0u, true, false);
    host_observe(&host, &c, 0u, true, true);
    host_observe(&host, &c, 1u, true, false);
    host_observe(&host, &c, 1u, false, false);
    host_observe(&host, &c, 1u, false, true);
    host_pump(&host, &c, 520u);
    assert(c.exchange.state == CAB_EX_DONE && c.exchange.completed == 1u);
    assert(host.unlocks == 2u);
    host_port_init(&host);
    ports = host_port_bind(&host);
    assert(cab_controller_init(&c, &ports) == CAB_OK);
    for (i = 0; i < CAB_BAYS; ++i)
        host_observe(&host, &c, i, i != 0u, true);
    host.samples[1].soc_permille = 950u;
    advance(&host, &c, 0u, 500u);
    host.output_failed = true;
    assert(cab_controller_post(&c, &event));
    host_pump(&host, &c, 510u);
    assert(c.safe_latched && host.unlocks == 0u);
    assert(c.exchange.state == CAB_EX_FAILED);
    return 0;
}
