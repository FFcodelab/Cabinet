#include "cabinet/update.h"
#include "cabinet/checksum.h"
#include <string.h>

static cab_result_t update_fail(cab_update_t *update, cab_result_t error)
{
    update->state = CAB_UPDATE_FAILED;
    update->error = error;
    return error;
}

cab_result_t cab_update_begin(cab_update_t *update, uint32_t size, uint32_t crc,
                              bool exchange_active, uint32_t now)
{
    if (size == 0u || size > CAB_IMAGE_MAX)
        return CAB_INVALID;
    if (exchange_active || update->state == CAB_UPDATE_RECEIVING)
        return CAB_BUSY;
    memset(update, 0, sizeof(*update));
    update->state = CAB_UPDATE_RECEIVING;
    update->size = size;
    update->expected_crc = crc;
    update->running_crc = UINT32_MAX;
    update->touched_ms = now;
    return CAB_OK;
}

cab_result_t cab_update_chunk(cab_update_t *update, uint32_t offset, const uint8_t *data,
                              size_t size, uint32_t now, cab_image_write_fn write, void *context)
{
    if (update->state != CAB_UPDATE_RECEIVING)
        return CAB_STATE;
    if (!data || !write || size == 0u || size > 256u || offset != update->received ||
        size > update->size - update->received)
        return CAB_INVALID;
    if (!write(context, offset, data, size))
        return update_fail(update, CAB_IO);
    update->running_crc = cab_crc32_update(update->running_crc, data, size);
    update->received += (uint32_t)size;
    update->touched_ms = now;
    return CAB_OK;
}

cab_result_t cab_update_finish(cab_update_t *update, cab_image_read_fn read, void *context)
{
    uint8_t buffer[64];
    uint32_t offset = 0u, crc = UINT32_MAX;
    if (update->state != CAB_UPDATE_RECEIVING || update->received != update->size)
        return CAB_STATE;
    if ((update->running_crc ^ UINT32_MAX) != update->expected_crc)
        return update_fail(update, CAB_CRC);
    while (offset < update->size)
    {
        size_t size = update->size - offset;
        if (size > sizeof(buffer))
            size = sizeof(buffer);
        if (!read || !read(context, offset, buffer, size))
            return update_fail(update, CAB_IO);
        crc = cab_crc32_update(crc, buffer, size);
        offset += (uint32_t)size;
    }
    if ((crc ^ UINT32_MAX) != update->expected_crc)
        return update_fail(update, CAB_CRC);
    update->state = CAB_UPDATE_READY;
    return CAB_OK;
}

void cab_update_tick(cab_update_t *update, uint32_t now)
{
    if (update->state == CAB_UPDATE_RECEIVING && cab_elapsed(now, update->touched_ms, 3000u))
        update_fail(update, CAB_TIMEOUT);
}

void cab_update_cancel(cab_update_t *update) { memset(update, 0, sizeof(*update)); }
