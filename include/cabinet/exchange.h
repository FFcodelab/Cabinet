#ifndef CABINET_EXCHANGE_H
#define CABINET_EXCHANGE_H
#include "cabinet/types.h"

typedef enum
{
    CAB_EX_IDLE,
    CAB_EX_RETURN_OPEN,
    CAB_EX_RETURN_PRESENT,
    CAB_EX_RETURN_CLOSE,
    CAB_EX_PICKUP_OPEN,
    CAB_EX_PICKUP_EMPTY,
    CAB_EX_PICKUP_CLOSE,
    CAB_EX_DONE,
    CAB_EX_FAILED
} cab_exchange_state_t;

typedef bool (*cab_unlock_fn)(void *context, unsigned bay);

typedef struct
{
    cab_exchange_state_t state;
    unsigned return_bay;
    unsigned pickup_bay;
    uint32_t entered_ms;
    uint32_t completed;
    cab_result_t error;
} cab_exchange_t;

bool cab_exchange_active(const cab_exchange_t *exchange);
bool cab_exchange_reserved(const cab_exchange_t *exchange, unsigned bay);
cab_result_t cab_exchange_start(cab_exchange_t *exchange, unsigned returned, unsigned pickup,
                                const cab_bay_t bays[CAB_BAYS], const cab_config_t *config,
                                uint32_t now, cab_unlock_fn unlock, void *context);
void cab_exchange_tick(cab_exchange_t *exchange, const cab_bay_t bays[CAB_BAYS],
                       const cab_config_t *config, uint32_t now, cab_unlock_fn unlock,
                       void *context);
cab_result_t cab_exchange_reset(cab_exchange_t *exchange, const cab_bay_t bays[CAB_BAYS]);
#endif
