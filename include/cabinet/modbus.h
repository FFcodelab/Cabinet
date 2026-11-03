#ifndef CABINET_MODBUS_H
#define CABINET_MODBUS_H
#include "cabinet/types.h"
#define CAB_RTU_MAX 255u
bool cab_rtu_valid(const uint8_t *frame, size_t size);
cab_result_t cab_rtu_read(uint8_t address, uint16_t first, uint16_t count, uint8_t out[8]);
cab_result_t cab_rtu_decode(const uint8_t *frame, size_t size, uint8_t address, uint16_t count,
                            uint16_t *values, uint8_t *exception);
cab_result_t cab_rtu_response(uint8_t address, const uint16_t *values, uint16_t count, uint8_t *out,
                              size_t capacity, size_t *written);
#endif
