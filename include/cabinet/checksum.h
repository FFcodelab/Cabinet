#ifndef CABINET_CHECKSUM_H
#define CABINET_CHECKSUM_H
#include "cabinet/types.h"
uint16_t cab_crc16(const uint8_t *data, size_t size);
uint32_t cab_crc32_update(uint32_t state, const uint8_t *data, size_t size);
uint32_t cab_crc32(const uint8_t *data, size_t size);
#endif
