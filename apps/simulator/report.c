#include "report.h"
#include <stdio.h>
#include <string.h>

static void append(cab_report_t *report, const char *text)
{
    size_t size = strlen(text);
    if (size > sizeof(report->bytes) - report->size)
    {
        report->failed = true;
        return;
    }
    memcpy(report->bytes + report->size, text, size);
    report->size += size;
}

static void number(cab_report_t *report, uint32_t n)
{
    char digits[11], ordered[11];
    unsigned count = 0u, i;
    do
    {
        digits[count++] = (char)('0' + n % 10u);
        n /= 10u;
    } while (n);
    for (i = 0; i < count; ++i)
        ordered[i] = digits[count - 1u - i];
    ordered[count] = '\0';
    append(report, ordered);
}

void cab_report_begin(cab_report_t *report)
{
    memset(report, 0, sizeof(*report));
    append(report, "{\"schema\":1,\"trace\":[");
}

void cab_report_sample(cab_report_t *report, const cab_controller_t *c)
{
    unsigned i;
    if (report->samples++)
        append(report, ",");
    append(report, "{\"ms\":");
    number(report, c->now_ms);
    append(report, ",\"exchange\":");
    number(report, (uint32_t)c->exchange.state);
    append(report, ",\"update\":");
    number(report, (uint32_t)c->update.state);
    append(report, ",\"watts\":[");
    for (i = 0; i < CAB_BAYS; ++i)
    {
        if (i)
            append(report, ",");
        number(report, c->bays[i].output_w);
    }
    append(report, "]}");
}

bool cab_report_finish(cab_report_t *report, const cab_controller_t *c, const char *path)
{
    FILE *file;
    bool success;
    append(report, "],\"completed\":");
    number(report, c->exchange.completed);
    append(report, ",\"faults\":");
    number(report, c->journal.total);
    append(report, ",\"requests\":");
    number(report, c->client.tx_count);
    append(report, ",\"timeouts\":");
    number(report, c->client.timeout_count);
    append(report, ",\"safe_latched\":");
    append(report, c->safe_latched ? "true" : "false");
    append(report, "}\n");
    if (report->failed)
        return false;
    file = fopen(path, "wb");
    if (!file)
        return false;
    success = fwrite(report->bytes, 1u, report->size, file) == report->size;
    return fclose(file) == 0 && success;
}
