#ifndef CABINET_STREAM_H
#define CABINET_STREAM_H
#include "cabinet/modbus.h"
typedef void (*cab_frame_fn)(void *context, const uint8_t *data, size_t size);

typedef struct
{
    uint8_t data[CAB_RTU_MAX];
    size_t used;
    uint32_t discarded;
    uint32_t accepted;
} cab_stream_t;

void cab_stream_init(cab_stream_t *stream);
void cab_stream_feed(cab_stream_t *stream, const uint8_t *data, size_t size, cab_frame_fn callback,
                     void *context);
void cab_stream_boundary(cab_stream_t *stream);
#endif
