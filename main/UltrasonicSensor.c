#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include <ultrasonic.h>

#include "UltrasonicSensor.h"
#include "Communication.h"

#define TRIG_PIN 26 //BLUE
#define ECHO_PIN 27 //GREEN


static void UltrasonicTask(void *pvParameters)
{
    ultrasonic_sensor_t sensor = {
        .trigger_pin = TRIG_PIN,
        .echo_pin = ECHO_PIN
    };
    ultrasonic_init(&sensor);

    int32_t currentSetDistance_cm = 50;
    float distance_m = 0.0f;           
    float distance_cm = 0.0f;           

    while(1)
    {
        xQueuePeek(thresholdQueue, &currentSetDistance_cm, 0);

        esp_err_t res = ultrasonic_measure(&sensor, 4.0, &distance_m);

        if (res == ESP_OK) 
        {
            distance_cm = distance_m * 100.0f;

            if(distance_cm < currentSetDistance_cm)
            {
                ESP_LOGI("UltraSonic", "ALARM! Przeszkoda: %.1f cm (Prog: %d cm)", distance_cm, (int)currentSetDistance_cm);
                xEventGroupSetBits(systemEventGroup, BIT_ALARM_ON);
            }
            else
            {
                ESP_LOGI("UltraSonic", "Czysto!");
                xEventGroupSetBits(systemEventGroup, BIT_ALARM_ON);
            }
        } 
        else
        {
            ESP_LOGW("UltraSonic", "Blad pomiaru: %s", esp_err_to_name(res));
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void start_ultrasonic_task(void)
{
    xTaskCreate(UltrasonicTask, "Ultrasonic_Task", 2048, NULL, 5, NULL);
}