#include "cabinet/config.h"
#include "cabinet/exchange.h"
#include "cabinet/power.h"
#include <assert.h>
#include <string.h>
#ifdef NDEBUG
#error Assertions must remain enabled in tests.
#endif
static unsigned unlocks;
static bool unlock_ok = true;

static bool unlock(void *context, unsigned bay)
{
    (void)context;
    assert(bay < CAB_BAYS);
    unlocks++;
    return unlock_ok;
}

static uint32_t random_state = 12345u;

static uint32_t next_random(void)
{
    random_state = random_state * 1664525u + 1013904223u;
    return random_state;
}

int main(void)
{
    cab_config_t config = cab_config_default();
    cab_bay_t bays[CAB_BAYS] = {0};
    cab_exchange_t exchange = {0};
    uint16_t demand[CAB_BAYS], allocated[CAB_BAYS];
    unsigned i, run, cursor;
    for (run = 0; run < 10000u; ++run)
    {
        uint32_t sum = 0u, wanted = 0u;
        uint16_t budget = (uint16_t)(next_random() % 8001u);
        for (i = 0; i < CAB_BAYS; ++i)
        {
            demand[i] = (uint16_t)(next_random() % 2001u);
            wanted += demand[i];
        }
        cab_power_allocate(demand, budget, next_random() % CAB_BAYS, allocated);
        for (i = 0; i < CAB_BAYS; ++i)
        {
            assert(allocated[i] <= demand[i]);
            sum += allocated[i];
        }
        assert(sum == (wanted < budget ? wanted : budget));
    }
    for (i = 0; i < CAB_BAYS; ++i)
        demand[i] = 300u;
    for (cursor = 0; cursor < CAB_BAYS; ++cursor)
    {
        cab_power_allocate(demand, 1u, cursor, allocated);
        assert(allocated[cursor] == 1u);
    }
    bays[1].present = true;
    bays[1].door_closed = true;
    bays[1].sample_valid = true;
    bays[1].sample = (cab_sample_t){4800u, 0, 950u, 250, 0u, 300u};
    bays[0].door_closed = true;
    assert(cab_exchange_start(&exchange, 0u, 1u, bays, &config, 10u, unlock, NULL) == CAB_OK);
    assert(cab_exchange_reserved(&exchange, 0u) && !cab_exchange_reserved(&exchange, 2u));
    assert(cab_exchange_start(&exchange, 0u, 1u, bays, &config, 10u, unlock, NULL) == CAB_BUSY);
    bays[0].door_closed = false;
    cab_exchange_tick(&exchange, bays, &config, 20u, unlock, NULL);
    bays[0].present = true;
    cab_exchange_tick(&exchange, bays, &config, 30u, unlock, NULL);
    bays[0].door_closed = true;
    cab_exchange_tick(&exchange, bays, &config, 40u, unlock, NULL);
    assert(exchange.state == CAB_EX_PICKUP_OPEN && unlocks == 2u);
    bays[1].door_closed = false;
    cab_exchange_tick(&exchange, bays, &config, 50u, unlock, NULL);
    bays[1].present = false;
    cab_exchange_tick(&exchange, bays, &config, 60u, unlock, NULL);
    bays[1].door_closed = true;
    cab_exchange_tick(&exchange, bays, &config, 70u, unlock, NULL);
    assert(exchange.state == CAB_EX_DONE && exchange.completed == 1u);
    assert(cab_exchange_reset(&exchange, bays) == CAB_STATE);
    for (i = 0; i < CAB_BAYS; ++i)
        bays[i].door_closed = true;
    assert(cab_exchange_reset(&exchange, bays) == CAB_OK);
    for (i = CAB_EX_RETURN_OPEN; i <= CAB_EX_PICKUP_CLOSE; ++i)
    {
        exchange.state = (cab_exchange_state_t)i;
        exchange.return_bay = 0u;
        exchange.pickup_bay = 1u;
        exchange.entered_ms = UINT32_MAX - 500u;
        cab_exchange_tick(&exchange, bays, &config, 1499u, unlock, NULL);
        assert(exchange.state == CAB_EX_FAILED && exchange.error == CAB_TIMEOUT);
    }
    assert(cab_exchange_reset(&exchange, bays) == CAB_OK);
    bays[0].present = false;
    bays[1].present = true;
    bays[1].sampled_ms = 2000u;
    unlock_ok = false;
    assert(cab_exchange_start(&exchange, 0u, 1u, bays, &config, 2000u, unlock, NULL) == CAB_IO);
    assert(exchange.state == CAB_EX_FAILED);
    bays[1].sample.soc_permille = 400u;
    assert(cab_charge_demand(&bays[1], &config, 2000u, false) == 300u);
    assert(cab_charge_demand(&bays[1], &config, 3500u, false) == 0u);
    assert(cab_charge_demand(&bays[1], &config, 2000u, true) == 0u);
    bays[1].door_closed = false;
    assert(cab_charge_demand(&bays[1], &config, 2000u, false) == 0u);
    bays[1].door_closed = true;
    bays[1].sample.temperature_dc = 451;
    assert(cab_charge_demand(&bays[1], &config, 2000u, false) == 0u);
    return 0;
}
