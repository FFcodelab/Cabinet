#include "cabinet/stream.h"
#include <string.h>

void cab_stream_init(cab_stream_t *stream) { memset(stream, 0, sizeof(*stream)); }

static void discard(cab_stream_t *stream, size_t count)
{
    memmove(stream->data, stream->data + count, stream->used - count);
    stream->used -= count;
    stream->discarded += (uint32_t)count;
}

static void parse(cab_stream_t *stream, cab_frame_fn callback, void *context)
{
    size_t expected;
    while (stream->used >= 3u)
    {
        if (stream->data[0] == 0u || stream->data[0] > 247u ||
            (stream->data[1] != 3u && stream->data[1] != 0x83u))
        {
            discard(stream, 1u);
            continue;
        }
        expected = stream->data[1] == 0x83u ? 5u : (size_t)stream->data[2] + 5u;
        if (expected > CAB_RTU_MAX ||
            (stream->data[1] == 3u && (stream->data[2] == 0u || (stream->data[2] & 1u))))
        {
            discard(stream, 1u);
            continue;
        }
        if (stream->used < expected)
            break;
        if (!cab_rtu_valid(stream->data, expected))
        {
            discard(stream, 1u);
            continue;
        }
        if (callback)
            callback(context, stream->data, expected);
        stream->accepted++;
        memmove(stream->data, stream->data + expected, stream->used - expected);
        stream->used -= expected;
    }
}

void cab_stream_feed(cab_stream_t *stream, const uint8_t *data, size_t size, cab_frame_fn callback,
                     void *context)
{
    size_t i;
    for (i = 0; i < size; ++i)
    {
        if (stream->used == sizeof(stream->data))
            discard(stream, 1u);
        stream->data[stream->used++] = data[i];
        parse(stream, callback, context);
    }
}

void cab_stream_boundary(cab_stream_t *stream)
{
    if (stream->used)
        discard(stream, stream->used);
}
