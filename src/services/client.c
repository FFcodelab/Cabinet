#include "cabinet/client.h"
#include <string.h>

enum
{
    REPLY_MS = 120,
    QUIET_MS = 10,
    RETRIES = 2
};

void cab_client_init(cab_client_t *client, cab_send_fn send, cab_reply_fn reply, void *context)
{
    memset(client, 0, sizeof(*client));
    client->send = send;
    client->reply = reply;
    client->context = context;
}

static void finish(cab_client_t *client, cab_result_t result, const uint16_t *registers)
{
    if (client->reply)
        client->reply(client->context, client->bay, result, registers, client->now_ms);
    client->pending = false;
    client->idle_ms = client->now_ms;
    client->bay = (client->bay + 1u) % CAB_BAYS;
    client->retries = 0u;
}

static bool send_request(cab_client_t *client)
{
    uint8_t frame[8];
    cab_rtu_read((uint8_t)(client->bay + 1u), 0u, CAB_SAMPLE_REGS, frame);
    cab_stream_boundary(&client->stream);
    client->sent_ms = client->now_ms;
    client->pending = true;
    client->tx_count++;
    if (!client->send || !client->send(client->context, frame, sizeof(frame)))
    {
        client->error_count++;
        finish(client, CAB_IO, NULL);
        return false;
    }
    return true;
}

void cab_client_tick(cab_client_t *client, uint32_t now)
{
    client->now_ms = now;
    if (!client->started)
    {
        client->started = true;
        client->idle_ms = now;
    }
    if (client->pending && cab_elapsed(now, client->sent_ms, REPLY_MS))
    {
        client->timeout_count++;
        if (client->retries < RETRIES)
        {
            client->retries++;
            send_request(client);
        }
        else
        {
            finish(client, CAB_TIMEOUT, NULL);
        }
    }
    else if (!client->pending && cab_elapsed(now, client->idle_ms, QUIET_MS))
    {
        send_request(client);
    }
}

static void on_frame(void *context, const uint8_t *data, size_t size)
{
    cab_client_t *client = context;
    uint16_t registers[CAB_SAMPLE_REGS];
    uint8_t exception = 0u;
    cab_result_t result;
    if (!client->pending || data[0] != client->bay + 1u)
        return;
    result = cab_rtu_decode(data, size, data[0], CAB_SAMPLE_REGS, registers, &exception);
    if (result == CAB_OK || result == CAB_REMOTE)
    {
        if (result != CAB_OK)
            client->error_count++;
        finish(client, result, result == CAB_OK ? registers : NULL);
    }
    else
    {
        client->error_count++;
    }
}

void cab_client_receive(cab_client_t *client, const uint8_t *data, size_t size, uint32_t now)
{
    client->now_ms = now;
    cab_stream_feed(&client->stream, data, size, on_frame, client);
}
