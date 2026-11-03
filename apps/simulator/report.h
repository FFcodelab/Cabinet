#ifndef CABINET_REPORT_H
#define CABINET_REPORT_H
#include "cabinet/controller.h"

typedef struct
{
    char bytes[65536];
    size_t size;
    bool failed;
    unsigned samples;
} cab_report_t;

void cab_report_begin(cab_report_t *report);
void cab_report_sample(cab_report_t *report, const cab_controller_t *controller);
bool cab_report_finish(cab_report_t *report, const cab_controller_t *controller, const char *path);
#endif
