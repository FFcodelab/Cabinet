#include "cabinet/checksum.h"

uint16_t cab_crc16(const uint8_t *data, size_t size)
{
    uint16_t crc = 0xffffu;
    size_t i;
    unsigned bit;
    for (i = 0; i < size; ++i)
    {
        crc ^= data[i];
        for (bit = 0; bit < 8; ++bit)
            crc = (uint16_t)((crc >> 1) ^ ((crc & 1u) ? 0xa001u : 0u));
    }
    return crc;
}

uint32_t cab_crc32_update(uint32_t state, const uint8_t *data, size_t size)
{
    size_t i;
    unsigned bit;
    for (i = 0; i < size; ++i)
    {
        state ^= data[i];
        for (bit = 0; bit < 8; ++bit)
            state = (state >> 1) ^ ((state & 1u) ? 0xedb88320u : 0u);
    }
    return state;
}

uint32_t cab_crc32(const uint8_t *data, size_t size)
{
    return cab_crc32_update(UINT32_MAX, data, size) ^ UINT32_MAX;
}
