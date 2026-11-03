#ifndef CABINET_JOURNAL_H
#define CABINET_JOURNAL_H
#include "cabinet/types.h"
#define CAB_JOURNAL_CAPACITY 16u

typedef enum
{
    CAB_FAULT_COMM,
    CAB_FAULT_SAMPLE,
    CAB_FAULT_QUEUE,
    CAB_FAULT_OUTPUT,
    CAB_FAULT_EXCHANGE,
    CAB_FAULT_CONFIG,
    CAB_FAULT_UPDATE
} cab_fault_kind_t;

typedef struct
{
    uint32_t at_ms;
    cab_fault_kind_t kind;
    unsigned bay;
    cab_result_t reason;
} cab_fault_t;

typedef struct
{
    cab_fault_t items[CAB_JOURNAL_CAPACITY];
    unsigned next;
    unsigned count;
    uint32_t total;
} cab_journal_t;

void cab_journal_record(cab_journal_t *journal, uint32_t now, cab_fault_kind_t kind, unsigned bay,
                        cab_result_t reason);
bool cab_journal_get(const cab_journal_t *journal, unsigned newest_index, cab_fault_t *fault);
#endif
