#include "cabinet/journal.h"

void cab_journal_record(cab_journal_t *journal, uint32_t now, cab_fault_kind_t kind, unsigned bay,
                        cab_result_t reason)
{
    cab_fault_t entry = {now, kind, bay, reason};
    journal->items[journal->next] = entry;
    journal->next = (journal->next + 1u) % CAB_JOURNAL_CAPACITY;
    if (journal->count < CAB_JOURNAL_CAPACITY)
        journal->count++;
    journal->total++;
}

bool cab_journal_get(const cab_journal_t *journal, unsigned newest_index, cab_fault_t *fault)
{
    unsigned index;
    if (newest_index >= journal->count || !fault)
        return false;
    index = (journal->next + CAB_JOURNAL_CAPACITY - 1u - newest_index) % CAB_JOURNAL_CAPACITY;
    *fault = journal->items[index];
    return true;
}
