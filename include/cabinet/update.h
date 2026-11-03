#ifndef CABINET_UPDATE_H
#define CABINET_UPDATE_H
#include "cabinet/types.h"

typedef enum
{
    CAB_UPDATE_IDLE,
    CAB_UPDATE_RECEIVING,
    CAB_UPDATE_READY,
    CAB_UPDATE_FAILED
} cab_update_state_t;

typedef bool (*cab_image_write_fn)(void *, uint32_t, const uint8_t *, size_t);
typedef bool (*cab_image_read_fn)(void *, uint32_t, uint8_t *, size_t);

typedef struct
{
    cab_update_state_t state;
    uint32_t size;
    uint32_t received;
    uint32_t expected_crc;
    uint32_t running_crc;
    uint32_t touched_ms;
    cab_result_t error;
} cab_update_t;

cab_result_t cab_update_begin(cab_update_t *update, uint32_t size, uint32_t crc,
                              bool exchange_active, uint32_t now);
cab_result_t cab_update_chunk(cab_update_t *update, uint32_t offset, const uint8_t *data,
                              size_t size, uint32_t now, cab_image_write_fn write, void *context);
cab_result_t cab_update_finish(cab_update_t *update, cab_image_read_fn read, void *context);
void cab_update_tick(cab_update_t *update, uint32_t now);
void cab_update_cancel(cab_update_t *update);
#endif
