#ifndef CABINET_POWER_H
#define CABINET_POWER_H
#include "cabinet/types.h"
uint16_t cab_charge_demand(const cab_bay_t *bay, const cab_config_t *config, uint32_t now,
                           bool reserved);
void cab_power_allocate(const uint16_t demand[CAB_BAYS], uint16_t budget, unsigned cursor,
                        uint16_t output[CAB_BAYS]);
#endif
