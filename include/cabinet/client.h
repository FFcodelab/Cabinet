#ifndef CABINET_CLIENT_H
#define CABINET_CLIENT_H
#include "cabinet/stream.h"
typedef bool (*cab_send_fn)(void *context, const uint8_t *data, size_t size);
typedef void (*cab_reply_fn)(void *context, unsigned bay, cab_result_t result,
                             const uint16_t *registers, uint32_t now);

typedef struct
{
    cab_stream_t stream;
    cab_send_fn send;
    cab_reply_fn reply;
    void *context;
    unsigned bay;
    unsigned retries;
    bool pending;
    bool started;
    uint32_t sent_ms;
    uint32_t idle_ms;
    uint32_t now_ms;
    uint32_t tx_count;
    uint32_t timeout_count;
    uint32_t error_count;
} cab_client_t;

void cab_client_init(cab_client_t *client, cab_send_fn send, cab_reply_fn reply, void *context);
void cab_client_tick(cab_client_t *client, uint32_t now);
void cab_client_receive(cab_client_t *client, const uint8_t *data, size_t size, uint32_t now);
#endif
