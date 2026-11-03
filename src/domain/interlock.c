#include "cabinet/power.h"

uint16_t cab_charge_demand(const cab_bay_t *bay, const cab_config_t *config, uint32_t now,
                           bool reserved)
{
    const cab_sample_t *s = &bay->sample;
    if (reserved || !bay->present || !bay->door_closed || !bay->sample_valid ||
        cab_elapsed(now, bay->sampled_ms, config->stale_ms) || s->flags != 0u ||
        s->temperature_dc < config->min_temp_dc || s->temperature_dc > config->max_temp_dc ||
        s->voltage_10mv == 0u || s->soc_permille >= config->ready_soc)
        return 0u;
    return s->limit_w < config->bay_limit_w ? s->limit_w : config->bay_limit_w;
}
