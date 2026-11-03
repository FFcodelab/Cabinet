#include "cabinet/config.h"
#include "cabinet/checksum.h"
#include <string.h>

static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void put32(uint8_t *p, uint32_t value)
{
    unsigned i;
    for (i = 0u; i < 4u; ++i)
        p[i] = (uint8_t)(value >> (8u * i));
}

cab_config_t cab_config_default(void)
{
    cab_config_t config = {800u, 300u, 900u, 0, 450, 1500u, 2000u};
    return config;
}

bool cab_config_valid(const cab_config_t *c)
{
    return c && c->budget_w >= 1u && c->budget_w <= 8000u && c->bay_limit_w >= 1u &&
           c->bay_limit_w <= 2000u && c->ready_soc >= 500u && c->ready_soc <= 1000u &&
           c->min_temp_dc >= -400 && c->max_temp_dc <= 800 && c->min_temp_dc < c->max_temp_dc &&
           c->stale_ms >= 500u && c->stale_ms <= 60000u && c->phase_ms >= 500u &&
           c->phase_ms <= 60000u;
}

void cab_config_encode(const cab_config_t *c, uint32_t sequence, uint8_t record[CAB_CONFIG_BYTES])
{
    memset(record, 0, CAB_CONFIG_BYTES);
    memcpy(record, "GCC1", 4u);
    put32(record + 4, 1u);
    put32(record + 8, sequence);
    cab_put_be16(record + 12, c->budget_w);
    cab_put_be16(record + 14, c->bay_limit_w);
    cab_put_be16(record + 16, c->ready_soc);
    cab_put_be16(record + 18, (uint16_t)(c->min_temp_dc + 400));
    cab_put_be16(record + 20, (uint16_t)(c->max_temp_dc + 400));
    put32(record + 24, c->stale_ms);
    put32(record + 28, c->phase_ms);
    put32(record + 44, cab_crc32(record, 44u));
}

cab_result_t cab_config_decode(const uint8_t record[CAB_CONFIG_BYTES], cab_config_t *config,
                               uint32_t *sequence)
{
    cab_config_t c;
    if (!record || !config || !sequence || memcmp(record, "GCC1", 4u) != 0 ||
        get32(record + 4) != 1u)
        return CAB_INVALID;
    if (get32(record + 44) != cab_crc32(record, 44u))
        return CAB_CRC;
    c.budget_w = cab_get_be16(record + 12);
    c.bay_limit_w = cab_get_be16(record + 14);
    c.ready_soc = cab_get_be16(record + 16);
    c.min_temp_dc = (int16_t)((int32_t)cab_get_be16(record + 18) - 400);
    c.max_temp_dc = (int16_t)((int32_t)cab_get_be16(record + 20) - 400);
    c.stale_ms = get32(record + 24);
    c.phase_ms = get32(record + 28);
    if (!cab_config_valid(&c))
        return CAB_INVALID;
    *config = c;
    *sequence = get32(record + 8);
    return CAB_OK;
}
