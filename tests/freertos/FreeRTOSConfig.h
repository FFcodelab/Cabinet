#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H
#include <stdlib.h>
#define configUSE_PREEMPTION 1
#define configCPU_CLOCK_HZ 1000000u
#define configTICK_RATE_HZ 1000u
#define configMAX_PRIORITIES 5u
#define configMINIMAL_STACK_SIZE 128u
#define configTOTAL_HEAP_SIZE 16384u
#define configMAX_TASK_NAME_LEN 16u
#define configTICK_TYPE_WIDTH_IN_BITS TICK_TYPE_WIDTH_32_BITS
#define configUSE_MUTEXES 1
#define configUSE_RECURSIVE_MUTEXES 0
#define configUSE_COUNTING_SEMAPHORES 0
#define configSUPPORT_STATIC_ALLOCATION 0
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configUSE_TIMERS 0
#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configUSE_CO_ROUTINES 0
#define configQUEUE_REGISTRY_SIZE 0u
#define configUSE_TRACE_FACILITY 0
#define configUSE_STATS_FORMATTING_FUNCTIONS 0
#define configASSERT(condition)                                                                    \
    do                                                                                             \
    {                                                                                              \
        if (!(condition))                                                                          \
            abort();                                                                               \
    } while (0)
#endif
