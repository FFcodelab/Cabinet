#include "cabinet/exchange.h"

bool cab_exchange_active(const cab_exchange_t *e)
{
    return e->state >= CAB_EX_RETURN_OPEN && e->state <= CAB_EX_PICKUP_CLOSE;
}

bool cab_exchange_reserved(const cab_exchange_t *e, unsigned bay)
{
    return (cab_exchange_active(e) || e->state == CAB_EX_FAILED) &&
           (bay == e->return_bay || bay == e->pickup_bay);
}

static bool ready(const cab_bay_t *bay, const cab_config_t *c, uint32_t now)
{
    return bay->present && bay->door_closed && bay->sample_valid &&
           !cab_elapsed(now, bay->sampled_ms, c->stale_ms) &&
           bay->sample.soc_permille >= c->ready_soc && bay->sample.voltage_10mv > 0u &&
           bay->sample.flags == 0u && bay->sample.temperature_dc >= c->min_temp_dc &&
           bay->sample.temperature_dc <= c->max_temp_dc;
}

static void enter(cab_exchange_t *e, cab_exchange_state_t state, uint32_t now)
{
    e->state = state;
    e->entered_ms = now;
}

static void fail(cab_exchange_t *e, cab_result_t result, uint32_t now)
{
    e->error = result;
    enter(e, CAB_EX_FAILED, now);
}

cab_result_t cab_exchange_start(cab_exchange_t *e, unsigned returned, unsigned pickup,
                                const cab_bay_t bays[CAB_BAYS], const cab_config_t *c, uint32_t now,
                                cab_unlock_fn unlock, void *context)
{
    if (returned >= CAB_BAYS || pickup >= CAB_BAYS || returned == pickup || !unlock)
        return CAB_INVALID;
    if (cab_exchange_active(e) || e->state == CAB_EX_FAILED)
        return CAB_BUSY;
    if (bays[returned].present || !bays[returned].door_closed || !ready(&bays[pickup], c, now))
        return CAB_STATE;
    e->return_bay = returned;
    e->pickup_bay = pickup;
    e->error = CAB_OK;
    if (!unlock(context, returned))
    {
        fail(e, CAB_IO, now);
        return CAB_IO;
    }
    enter(e, CAB_EX_RETURN_OPEN, now);
    return CAB_OK;
}

void cab_exchange_tick(cab_exchange_t *e, const cab_bay_t bays[CAB_BAYS], const cab_config_t *c,
                       uint32_t now, cab_unlock_fn unlock, void *context)
{
    const cab_bay_t *returned, *pickup;
    if (!cab_exchange_active(e))
        return;
    if (cab_elapsed(now, e->entered_ms, c->phase_ms))
    {
        fail(e, CAB_TIMEOUT, now);
        return;
    }
    returned = &bays[e->return_bay];
    pickup = &bays[e->pickup_bay];
    switch (e->state)
    {
    case CAB_EX_RETURN_OPEN:
        if (!returned->door_closed)
            enter(e, CAB_EX_RETURN_PRESENT, now);
        break;
    case CAB_EX_RETURN_PRESENT:
        if (returned->door_closed)
            fail(e, CAB_STATE, now);
        else if (returned->present)
            enter(e, CAB_EX_RETURN_CLOSE, now);
        break;
    case CAB_EX_RETURN_CLOSE:
        if (!returned->present)
            fail(e, CAB_STATE, now);
        else if (returned->door_closed)
        {
            if (!ready(pickup, c, now))
                fail(e, CAB_STATE, now);
            else if (!unlock || !unlock(context, e->pickup_bay))
                fail(e, CAB_IO, now);
            else
                enter(e, CAB_EX_PICKUP_OPEN, now);
        }
        break;
    case CAB_EX_PICKUP_OPEN:
        if (!pickup->door_closed)
            enter(e, CAB_EX_PICKUP_EMPTY, now);
        break;
    case CAB_EX_PICKUP_EMPTY:
        if (pickup->door_closed)
            fail(e, CAB_STATE, now);
        else if (!pickup->present)
            enter(e, CAB_EX_PICKUP_CLOSE, now);
        break;
    case CAB_EX_PICKUP_CLOSE:
        if (pickup->present)
            fail(e, CAB_STATE, now);
        else if (pickup->door_closed)
        {
            e->completed++;
            enter(e, CAB_EX_DONE, now);
        }
        break;
    default:
        break;
    }
}

cab_result_t cab_exchange_reset(cab_exchange_t *e, const cab_bay_t bays[CAB_BAYS])
{
    unsigned i;
    if (cab_exchange_active(e))
        return CAB_BUSY;
    for (i = 0; i < CAB_BAYS; ++i)
        if (!bays[i].door_closed)
            return CAB_STATE;
    e->state = CAB_EX_IDLE;
    e->error = CAB_OK;
    return CAB_OK;
}
