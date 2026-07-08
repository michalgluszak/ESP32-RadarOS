#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

// Flagi dla Event Group
#define BIT_ALARM_ON  (1 << 0) // Ustawiamy bit 0

extern QueueHandle_t thresholdQueue;
extern EventGroupHandle_t systemEventGroup;

void init_communication(void);

#endif