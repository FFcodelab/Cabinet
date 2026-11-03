#ifndef CABINET_SAMPLE_H
#define CABINET_SAMPLE_H
#include "cabinet/types.h"
cab_result_t cab_sample_decode(const uint16_t registers[CAB_SAMPLE_REGS], cab_sample_t *sample);
void cab_sample_encode(const cab_sample_t *sample, uint16_t registers[CAB_SAMPLE_REGS]);
#endif
