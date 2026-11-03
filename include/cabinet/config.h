#ifndef CABINET_CONFIG_H
#define CABINET_CONFIG_H
#include "cabinet/types.h"
typedef bool (*cab_nv_read_fn)(void *, unsigned, uint8_t *, size_t);
typedef bool (*cab_nv_write_fn)(void *, unsigned, const uint8_t *, size_t);

typedef struct
{
    cab_nv_read_fn read;
    cab_nv_write_fn write;
    void *context;
    unsigned active;
    uint32_t sequence;
    bool loaded;
} cab_store_t;

cab_config_t cab_config_default(void);
bool cab_config_valid(const cab_config_t *config);
void cab_config_encode(const cab_config_t *config, uint32_t sequence,
                       uint8_t record[CAB_CONFIG_BYTES]);
cab_result_t cab_config_decode(const uint8_t record[CAB_CONFIG_BYTES], cab_config_t *config,
                               uint32_t *sequence);
void cab_store_init(cab_store_t *store, cab_nv_read_fn read, cab_nv_write_fn write, void *context);
cab_result_t cab_store_load(cab_store_t *store, cab_config_t *config);
cab_result_t cab_store_save(cab_store_t *store, const cab_config_t *config);
#endif
