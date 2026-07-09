#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#define BIT_ALARM_ON  (1 << 0)

extern QueueHandle_t thresholdQueue;
extern QueueHandle_t angleQueue;    
extern QueueHandle_t distanceQueue;  

extern EventGroupHandle_t systemEventGroup;

void init_communication(void);

#endif