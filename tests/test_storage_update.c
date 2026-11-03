#include "host_port.h"
#include "cabinet/checksum.h"
#include <assert.h>
#include <string.h>
#ifdef NDEBUG
#error Assertions must remain enabled in tests.
#endif
int main(void)
{
    host_port_t host;
    cab_ports_t ports;
    cab_store_t store, reboot;
    cab_config_t config = cab_config_default(), loaded;
    cab_update_t update = {0};
    uint32_t sequence;
    uint8_t record[CAB_CONFIG_BYTES], image[320];
    unsigned i;
    host_port_init(&host);
    ports = host_port_bind(&host);
    cab_store_init(&store, ports.nv_read, ports.nv_write, &host);
    assert(cab_store_load(&store, &loaded) == CAB_EMPTY && loaded.budget_w == 800u);
    assert(cab_store_save(&store, &config) == CAB_OK);
    config.budget_w = 600u;
    assert(cab_store_save(&store, &config) == CAB_OK);
    host.nv[store.active][17] ^= 1u;
    cab_store_init(&reboot, ports.nv_read, ports.nv_write, &host);
    assert(cab_store_load(&reboot, &loaded) == CAB_OK && loaded.budget_w == 800u);
    host.torn_write = true;
    assert(cab_store_save(&reboot, &config) == CAB_IO);
    assert(cab_store_load(&reboot, &loaded) == CAB_OK && loaded.budget_w == 800u);
    host.torn_write = false;
    cab_config_encode(&config, UINT32_MAX, host.nv[0]);
    config.budget_w = 500u;
    cab_config_encode(&config, 0u, host.nv[1]);
    assert(cab_store_load(&reboot, &loaded) == CAB_OK && loaded.budget_w == 500u);
    assert(reboot.active == 1u);
    assert(cab_store_save(&reboot, &config) == CAB_OK && reboot.sequence == 1u);
    config.phase_ms = 0u;
    assert(cab_store_save(&reboot, &config) == CAB_INVALID);
    config = cab_config_default();
    cab_config_encode(&config, 9u, record);
    record[13] ^= 1u;
    assert(cab_config_decode(record, &loaded, &sequence) == CAB_CRC);
    for (i = 0; i < sizeof(image); ++i)
        image[i] = (uint8_t)(i ^ 0x5au);
    assert(cab_update_begin(&update, sizeof(image), cab_crc32(image, sizeof(image)), true, 0u) ==
           CAB_BUSY);
    assert(cab_update_begin(&update, CAB_IMAGE_MAX + 1u, 0u, false, 0u) == CAB_INVALID);
    assert(cab_update_begin(&update, sizeof(image), cab_crc32(image, sizeof(image)), false, 0u) ==
           CAB_OK);
    assert(cab_update_chunk(&update, 1u, image, 64u, 10u, ports.image_write, &host) == CAB_INVALID);
    assert(update.received == 0u);
    assert(cab_update_chunk(&update, 0u, image, 256u, 10u, ports.image_write, &host) == CAB_OK);
    assert(cab_update_finish(&update, ports.image_read, &host) == CAB_STATE);
    assert(cab_update_chunk(&update, 256u, image + 256u, 64u, 20u, ports.image_write, &host) ==
           CAB_OK);
    assert(cab_update_finish(&update, ports.image_read, &host) == CAB_OK &&
           update.state == CAB_UPDATE_READY);
    assert(memcmp(host.image, image, sizeof(image)) == 0);
    assert(cab_update_begin(&update, 64u, cab_crc32(image, 64u), false, 100u) == CAB_OK);
    assert(cab_update_chunk(&update, 0u, image, 64u, 110u, ports.image_write, &host) == CAB_OK);
    host.image[1] ^= 1u;
    assert(cab_update_finish(&update, ports.image_read, &host) == CAB_CRC);
    assert(update.state == CAB_UPDATE_FAILED);
    assert(cab_update_begin(&update, 64u, 0u, false, UINT32_MAX - 1000u) == CAB_OK);
    cab_update_tick(&update, 1999u);
    assert(update.state == CAB_UPDATE_FAILED && update.error == CAB_TIMEOUT);
    assert(cab_update_begin(&update, 64u, 0u, false, 0u) == CAB_OK);
    host.image_failed = true;
    assert(cab_update_chunk(&update, 0u, image, 64u, 1u, ports.image_write, &host) == CAB_IO);
    cab_update_cancel(&update);
    assert(update.state == CAB_UPDATE_IDLE);
    return 0;
}
