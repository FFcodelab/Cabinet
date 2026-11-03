#include "cabinet/config.h"
#include <string.h>

void cab_store_init(cab_store_t *store, cab_nv_read_fn read, cab_nv_write_fn write, void *context)
{
    memset(store, 0, sizeof(*store));
    store->read = read;
    store->write = write;
    store->context = context;
}

cab_result_t cab_store_load(cab_store_t *store, cab_config_t *config)
{
    uint8_t record[2][CAB_CONFIG_BYTES];
    cab_config_t candidates[2];
    uint32_t sequences[2] = {0u, 0u};
    bool valid[2] = {false, false};
    unsigned i;
    if (!store || !config || !store->read)
        return CAB_INVALID;
    store->loaded = false;
    for (i = 0; i < 2u; ++i)
        valid[i] = store->read(store->context, i, record[i], CAB_CONFIG_BYTES) &&
                   cab_config_decode(record[i], &candidates[i], &sequences[i]) == CAB_OK;
    if (!valid[0] && !valid[1])
    {
        *config = cab_config_default();
        return CAB_EMPTY;
    }
    i = valid[1] && (!valid[0] || ((sequences[1] - sequences[0]) != 0u &&
                                   (uint32_t)(sequences[1] - sequences[0]) < UINT32_C(0x80000000)))
            ? 1u
            : 0u;
    store->active = i;
    store->sequence = sequences[i];
    store->loaded = true;
    *config = candidates[i];
    return CAB_OK;
}

cab_result_t cab_store_save(cab_store_t *store, const cab_config_t *config)
{
    uint8_t record[CAB_CONFIG_BYTES], verify[CAB_CONFIG_BYTES];
    unsigned target;
    uint32_t sequence;
    if (!store || !cab_config_valid(config) || !store->write || !store->read)
        return CAB_INVALID;
    target = store->loaded ? store->active ^ 1u : 0u;
    sequence = store->loaded ? store->sequence + 1u : 1u;
    cab_config_encode(config, sequence, record);
    if (!store->write(store->context, target, record, sizeof(record)) ||
        !store->read(store->context, target, verify, sizeof(verify)) ||
        memcmp(record, verify, sizeof(record)) != 0)
        return CAB_IO;
    store->active = target;
    store->sequence = sequence;
    store->loaded = true;
    return CAB_OK;
}
