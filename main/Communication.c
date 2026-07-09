#include "Communication.h"

QueueHandle_t thresholdQueue = NULL;
QueueHandle_t angleQueue = NULL;     
QueueHandle_t distanceQueue = NULL;   
EventGroupHandle_t systemEventGroup = NULL;

void init_communication(void)
{
    thresholdQueue = xQueueCreate(1, sizeof(int32_t));
    angleQueue = xQueueCreate(1, sizeof(int32_t));    
    distanceQueue = xQueueCreate(1, sizeof(int32_t));

    systemEventGroup = xEventGroupCreate();
}