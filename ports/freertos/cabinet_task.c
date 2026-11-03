#include "cabinet_task.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include <stdatomic.h>
#include <string.h>
#if ATOMIC_INT_LOCK_FREE != 2
#error This adapter requires lock-free unsigned atomics.
#endif
typedef struct
{
    bool serial;
    size_t size;
    cab_event_t event;
    uint8_t data[64];
} ingress_t;

static cab_controller_t controller;
static QueueHandle_t inbox;
static SemaphoreHandle_t snapshot_lock;
static cab_task_snapshot_t published;
static atomic_uint ingress_fault;
static uint32_t monotonic_ms, remainder;
static TickType_t previous_tick;

static uint32_t clock_ms(void)
{
    TickType_t current = xTaskGetTickCount();
    TickType_t elapsed = (TickType_t)(current - previous_tick);
    uint64_t scaled = (uint64_t)elapsed * 1000u + remainder;
    monotonic_ms += (uint32_t)(scaled / configTICK_RATE_HZ);
    remainder = (uint32_t)(scaled % configTICK_RATE_HZ);
    previous_tick = current;
    return monotonic_ms;
}

static void publish(void)
{
    unsigned i;
    if (xSemaphoreTake(snapshot_lock, 0) != pdTRUE)
        return;
    for (i = 0; i < CAB_BAYS; ++i)
        published.watts[i] = controller.bays[i].output_w;
    published.exchange = controller.exchange.state;
    published.update = controller.update.state;
    published.steps = controller.steps;
    published.faults = controller.journal.total;
    published.safe_latched = controller.safe_latched;
    xSemaphoreGive(snapshot_lock);
}

static void control_task(void *unused)
{
    ingress_t message;
    TickType_t period = pdMS_TO_TICKS(10);
    (void)unused;
    if (period == 0)
        period = 1;
    previous_tick = xTaskGetTickCount();
    for (;;)
    {
        unsigned drained = 0u;
        uint32_t now;
        BaseType_t received = xQueueReceive(inbox, &message, period);
        now = clock_ms();
        while (received == pdTRUE && drained++ < CAB_EVENT_CAPACITY)
        {
            if (message.serial)
                cab_controller_receive(&controller, message.data, message.size, now);
            else
                cab_controller_post(&controller, &message.event);
            if (drained >= CAB_EVENT_CAPACITY)
                break;
            received = xQueueReceive(inbox, &message, 0);
        }
        if (atomic_exchange_explicit(&ingress_fault, 0u, memory_order_relaxed))
        {
            controller.safe_latched = true;
            cab_journal_record(&controller.journal, now, CAB_FAULT_QUEUE, CAB_BAYS, CAB_FULL);
        }
        cab_controller_step(&controller, now);
        publish();
    }
}

bool cabinet_task_start(const cab_ports_t *ports)
{
    if (inbox)
        return false;
    if (cab_controller_init(&controller, ports) != CAB_OK)
        return false;
    inbox = xQueueCreate(CAB_EVENT_CAPACITY, sizeof(ingress_t));
    snapshot_lock = xSemaphoreCreateMutex();
    if (!inbox || !snapshot_lock)
        goto failed;
    atomic_init(&ingress_fault, 0u);
    if (xTaskCreate(control_task, "cabinet", 1024u, NULL, tskIDLE_PRIORITY + 2u, NULL) != pdPASS)
        goto failed;
    return true;
failed:
    if (inbox)
        vQueueDelete(inbox);
    if (snapshot_lock)
        vSemaphoreDelete(snapshot_lock);
    inbox = NULL;
    snapshot_lock = NULL;
    return false;
}

bool cabinet_task_post(const cab_event_t *event)
{
    ingress_t message = {0};
    if (!inbox || !event)
        return false;
    message.event = *event;
    if (xQueueSend(inbox, &message, 0) != pdPASS)
    {
        atomic_store_explicit(&ingress_fault, 1u, memory_order_relaxed);
        return false;
    }
    return true;
}

bool cabinet_task_receive_from_isr(const uint8_t *data, size_t size)
{
    ingress_t message = {0};
    BaseType_t higher_priority_woken = pdFALSE;
    if (!inbox || !data || size == 0u || size > sizeof(message.data))
    {
        atomic_store_explicit(&ingress_fault, 1u, memory_order_relaxed);
        return false;
    }
    message.serial = true;
    message.size = size;
    memcpy(message.data, data, size);
    if (xQueueSendFromISR(inbox, &message, &higher_priority_woken) != pdPASS)
    {
        atomic_store_explicit(&ingress_fault, 1u, memory_order_relaxed);
        return false;
    }
    portYIELD_FROM_ISR(higher_priority_woken);
    return true;
}

bool cabinet_task_snapshot(cab_task_snapshot_t *snapshot)
{
    if (!snapshot || !snapshot_lock || xSemaphoreTake(snapshot_lock, pdMS_TO_TICKS(10)) != pdTRUE)
        return false;
    *snapshot = published;
    xSemaphoreGive(snapshot_lock);
    return true;
}
