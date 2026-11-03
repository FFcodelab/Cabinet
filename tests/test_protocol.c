#include "cabinet/checksum.h"
#include "cabinet/client.h"
#include "cabinet/sample.h"
#include <assert.h>
#include <string.h>
#ifdef NDEBUG
#error Assertions must remain enabled in tests.
#endif
static unsigned frames, sends, replies;
static cab_result_t last_reply;

static void frame_seen(void *context, const uint8_t *data, size_t size)
{
    (void)context;
    assert(cab_rtu_valid(data, size));
    frames++;
}

static bool sent(void *context, const uint8_t *data, size_t size)
{
    (void)context;
    assert(size == 8u && cab_rtu_valid(data, size));
    sends++;
    return true;
}

static void replied(void *context, unsigned bay, cab_result_t result, const uint16_t *values,
                    uint32_t now)
{
    (void)context;
    (void)values;
    (void)now;
    assert(bay < CAB_BAYS);
    replies++;
    last_reply = result;
}

int main(void)
{
    const uint8_t reference[] = "123456789";
    const uint8_t request_reference[] = {1u, 3u, 0u, 0u, 0u, 10u, 0xc5u, 0xcdu};
    uint8_t request[8], response[255], packet[512], exception[5] = {1u, 0x83u, 2u, 0u, 0u};
    uint16_t values[125] = {4800u, (uint16_t)-100, 450u, 250u, 0u, 300u};
    uint16_t decoded[125], crc;
    uint8_t code = 0;
    size_t size, i;
    cab_stream_t stream;
    cab_sample_t sample;
    cab_client_t client;
    assert(cab_crc16(reference, 9u) == 0x4b37u);
    assert(cab_crc32(reference, 9u) == UINT32_C(0xcbf43926));
    assert(cab_rtu_read(1u, 0u, 10u, request) == CAB_OK);
    assert(memcmp(request, request_reference, sizeof(request)) == 0);
    assert(cab_rtu_read(0u, 0u, 1u, request) == CAB_INVALID);
    assert(cab_rtu_read(1u, 65535u, 2u, request) == CAB_INVALID);
    assert(cab_rtu_read(1u, 0u, 126u, request) == CAB_INVALID);
    assert(cab_rtu_response(1u, values, CAB_SAMPLE_REGS, response, sizeof(response), &size) ==
           CAB_OK);
    assert(cab_rtu_decode(response, size, 1u, CAB_SAMPLE_REGS, decoded, &code) == CAB_OK);
    assert(memcmp(values, decoded, sizeof(uint16_t) * CAB_SAMPLE_REGS) == 0);
    assert(cab_rtu_decode(response, size, 2u, CAB_SAMPLE_REGS, decoded, &code) == CAB_INVALID);
    assert(cab_rtu_decode(response, size, 1u, 1u, decoded, &code) == CAB_INVALID);
    response[size - 1u] ^= 1u;
    assert(cab_rtu_decode(response, size, 1u, CAB_SAMPLE_REGS, decoded, &code) == CAB_CRC);
    response[size - 1u] ^= 1u;
    crc = cab_crc16(exception, 3u);
    exception[3] = (uint8_t)crc;
    exception[4] = (uint8_t)(crc >> 8);
    assert(cab_rtu_decode(exception, 5u, 1u, CAB_SAMPLE_REGS, decoded, &code) == CAB_REMOTE);
    assert(code == 2u);
    cab_stream_init(&stream);
    for (i = 0; i < size; ++i)
        cab_stream_feed(&stream, response + i, 1u, frame_seen, NULL);
    assert(frames == 1u);
    memset(packet, 0xff, 5u);
    memcpy(packet + 5u, response, size);
    memcpy(packet + 5u + size, response, size);
    cab_stream_feed(&stream, packet, 5u + size * 2u, frame_seen, NULL);
    assert(frames == 3u && stream.discarded >= 5u);
    packet[0] = 1u;
    packet[1] = 3u;
    packet[2] = 250u;
    cab_stream_feed(&stream, packet, 3u, frame_seen, NULL);
    cab_stream_boundary(&stream);
    cab_stream_feed(&stream, exception, sizeof(exception), frame_seen, NULL);
    assert(frames == 4u && stream.used == 0u);
    assert(cab_sample_decode(values, &sample) == CAB_OK && sample.current_10ma == -100);
    values[2] = 1001u;
    assert(cab_sample_decode(values, &sample) == CAB_INVALID);
    values[2] = 450u;
    cab_client_init(&client, sent, replied, NULL);
    cab_client_tick(&client, UINT32_MAX - 20u);
    cab_client_tick(&client, UINT32_MAX - 10u);
    assert(sends == 1u && client.pending);
    response[0] = 2u;
    crc = cab_crc16(response, size - 2u);
    response[size - 2u] = (uint8_t)crc;
    response[size - 1u] = (uint8_t)(crc >> 8);
    cab_client_receive(&client, response, size, 0u);
    assert(replies == 0u && client.pending);
    cab_client_tick(&client, 109u);
    cab_client_tick(&client, 229u);
    cab_client_tick(&client, 349u);
    assert(sends == 3u && replies == 1u && last_reply == CAB_TIMEOUT && client.bay == 1u);
    cab_client_tick(&client, 359u);
    cab_client_receive(&client, response, size, 360u);
    assert(replies == 2u && last_reply == CAB_OK && !client.pending);
    assert(cab_rtu_response(247u, values, 125u, response, sizeof(response), &size) == CAB_OK);
    cab_stream_feed(&stream, response, size, frame_seen, NULL);
    assert(stream.used == 0u && frames == 5u);
    return 0;
}
