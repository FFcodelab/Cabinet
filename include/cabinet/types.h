#ifndef CABINET_TYPES_H
#define CABINET_TYPES_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define CAB_BAYS 4u
#define CAB_SAMPLE_REGS 6u
#define CAB_IMAGE_MAX 4096u
#define CAB_CONFIG_BYTES 48u
#define CAB_EVENT_CAPACITY 32u

typedef enum
{
    CAB_OK,
    CAB_INVALID,
    CAB_BUSY,
    CAB_IO,
    CAB_CRC,
    CAB_TIMEOUT,
    CAB_EMPTY,
    CAB_FULL,
    CAB_STATE,
    CAB_REMOTE
} cab_result_t;

typedef struct
{
    uint16_t voltage_10mv;
    int16_t current_10ma;
    uint16_t soc_permille;
    int16_t temperature_dc;
    uint16_t flags;
    uint16_t limit_w;
} cab_sample_t;

typedef struct
{
    bool present;
    bool door_closed;
    bool sample_valid;
    uint32_t sampled_ms;
    cab_sample_t sample;
    uint16_t output_w;
} cab_bay_t;

typedef struct
{
    uint16_t budget_w;
    uint16_t bay_limit_w;
    uint16_t ready_soc;
    int16_t min_temp_dc;
    int16_t max_temp_dc;
    uint32_t stale_ms;
    uint32_t phase_ms;
} cab_config_t;

static inline bool cab_elapsed(uint32_t now, uint32_t then, uint32_t duration)
{
    return (uint32_t)(now - then) >= duration;
}

static inline uint16_t cab_get_be16(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static inline void cab_put_be16(uint8_t *p, uint16_t n)
{
    p[0] = (uint8_t)(n >> 8);
    p[1] = (uint8_t)n;
}
#endif
