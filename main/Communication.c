#include "Communication.h"

QueueHandle_t thresholdQueue = NULL;
EventGroupHandle_t systemEventGroup = NULL;

void init_communication(void)
{
    thresholdQueue = xQueueCreate(1, sizeof(int32_t));

    systemEventGroup = xEventGroupCreate();
}