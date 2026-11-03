#include "cabinet/power.h"
#include <string.h>

void cab_power_allocate(const uint16_t demand[CAB_BAYS], uint16_t budget, unsigned cursor,
                        uint16_t output[CAB_BAYS])
{
    uint32_t remaining = budget;
    unsigned active, i, bay;
    uint32_t share;
    bool progressed;
    memset(output, 0, sizeof(uint16_t) * CAB_BAYS);
    do
    {
        active = 0u;
        for (i = 0; i < CAB_BAYS; ++i)
            if (output[i] < demand[i])
                active++;
        if (!active || !remaining)
            break;
        share = remaining / active;
        if (!share)
            share = 1u;
        progressed = false;
        for (i = 0; i < CAB_BAYS && remaining; ++i)
        {
            uint32_t grant, room;
            bay = (cursor + i) % CAB_BAYS;
            room = (uint32_t)demand[bay] - output[bay];
            grant = room < share ? room : share;
            if (grant > remaining)
                grant = remaining;
            output[bay] = (uint16_t)(output[bay] + grant);
            remaining -= grant;
            progressed = progressed || grant != 0u;
        }
    } while (progressed);
}
