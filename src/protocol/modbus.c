#include "cabinet/modbus.h"
#include "cabinet/checksum.h"

static void append_crc(uint8_t *frame, size_t payload)
{
    uint16_t crc = cab_crc16(frame, payload);
    frame[payload] = (uint8_t)crc;
    frame[payload + 1u] = (uint8_t)(crc >> 8);
}

bool cab_rtu_valid(const uint8_t *frame, size_t size)
{
    uint16_t wire;
    if (!frame || size < 5u || size > CAB_RTU_MAX || frame[0] == 0u || frame[0] > 247u)
        return false;
    wire = (uint16_t)(frame[size - 2u] | ((uint16_t)frame[size - 1u] << 8));
    return wire == cab_crc16(frame, size - 2u);
}

cab_result_t cab_rtu_read(uint8_t address, uint16_t first, uint16_t count, uint8_t out[8])
{
    if (!out || address == 0u || address > 247u || count == 0u || count > 125u ||
        (uint32_t)first + count > 65536u)
        return CAB_INVALID;
    out[0] = address;
    out[1] = 3u;
    cab_put_be16(out + 2, first);
    cab_put_be16(out + 4, count);
    append_crc(out, 6u);
    return CAB_OK;
}

cab_result_t cab_rtu_decode(const uint8_t *frame, size_t size, uint8_t address, uint16_t count,
                            uint16_t *values, uint8_t *exception)
{
    uint16_t i;
    if (!frame || !values || count == 0u || count > 125u || size < 5u)
        return CAB_INVALID;
    if (!cab_rtu_valid(frame, size))
        return CAB_CRC;
    if (frame[0] != address)
        return CAB_INVALID;
    if (frame[1] == 0x83u && size == 5u)
    {
        if (exception)
            *exception = frame[2];
        return CAB_REMOTE;
    }
    if (frame[1] != 3u || frame[2] != count * 2u || size != 5u + count * 2u)
        return CAB_INVALID;
    for (i = 0; i < count; ++i)
        values[i] = cab_get_be16(frame + 3u + 2u * i);
    return CAB_OK;
}

cab_result_t cab_rtu_response(uint8_t address, const uint16_t *values, uint16_t count, uint8_t *out,
                              size_t capacity, size_t *written)
{
    uint16_t i;
    size_t size = 5u + 2u * count;
    if (!out || !values || !written || address == 0u || address > 247u || count == 0u ||
        count > 125u || size > capacity)
        return CAB_INVALID;
    out[0] = address;
    out[1] = 3u;
    out[2] = (uint8_t)(2u * count);
    for (i = 0; i < count; ++i)
        cab_put_be16(out + 3u + 2u * i, values[i]);
    append_crc(out, size - 2u);
    *written = size;
    return CAB_OK;
}
