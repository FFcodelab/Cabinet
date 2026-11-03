#include "cabinet/sample.h"

cab_result_t cab_sample_decode(const uint16_t registers[CAB_SAMPLE_REGS], cab_sample_t *sample)
{
    cab_sample_t value;
    if (!registers || !sample)
        return CAB_INVALID;
    value.voltage_10mv = registers[0];
    value.current_10ma = registers[1] <= INT16_MAX ? (int16_t)registers[1]
                                                   : (int16_t)((int32_t)registers[1] - 65536);
    value.soc_permille = registers[2];
    value.temperature_dc = registers[3] <= INT16_MAX ? (int16_t)registers[3]
                                                     : (int16_t)((int32_t)registers[3] - 65536);
    value.flags = registers[4];
    value.limit_w = registers[5];
    if (value.voltage_10mv > 10000u || value.soc_permille > 1000u || value.temperature_dc < -500 ||
        value.temperature_dc > 1500 || value.flags > 3u || value.limit_w > 2000u)
        return CAB_INVALID;
    *sample = value;
    return CAB_OK;
}

void cab_sample_encode(const cab_sample_t *sample, uint16_t registers[CAB_SAMPLE_REGS])
{
    registers[0] = sample->voltage_10mv;
    registers[1] = (uint16_t)sample->current_10ma;
    registers[2] = sample->soc_permille;
    registers[3] = (uint16_t)sample->temperature_dc;
    registers[4] = sample->flags;
    registers[5] = sample->limit_w;
}
