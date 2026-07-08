#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "Communication.h"

#define BUZZER_PIN 33 //PURPLE

static void AlarmTask(void *pvParameters)
{
    gpio_reset_pin(BUZZER_PIN);
    gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT);

    gpio_set_level(BUZZER_PIN, 0);

    while(1)
    {
        xEventGroupWaitBits(systemEventGroup, BIT_ALARM_ON, pdFALSE, pdFALSE, portMAX_DELAY);

        gpio_set_level(BUZZER_PIN, 1);

        while( (xEventGroupGetBits(systemEventGroup) & BIT_ALARM_ON) != 0 )
        {
             vTaskDelay(pdMS_TO_TICKS(50)); 
        }

        gpio_set_level(BUZZER_PIN, 0);
    }
}

void start_alarm_task(void)
{
    xTaskCreate(AlarmTask, "Alarm Task", 2048, NULL, 5, NULL);
}
