#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "ServoMotor.h"

#include "Communication.h"

#define SERVO_PIN 25 //YELLOW

static int32_t angle_to_duty_cycle(int32_t angle)
{
    if(angle > 180) 
    {
        angle = 180;
    }

    float pulse_width = 0.5 + (angle / 180.0) * 2.0;
    int32_t duty = (pulse_width / 20.0) * 4096;
    return duty;
}

static void ServoTask(void *pvParameters)
{
    //Configure PWM timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_12_BIT,
        .freq_hz = 50,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    //Configure PWM channel
    ledc_channel_config_t ledc_channel = {
        .channel = LEDC_CHANNEL_0,
        .duty = 0,
        .gpio_num = SERVO_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = LEDC_TIMER_0
    };
    ledc_channel_config(&ledc_channel);

    while(1)
    {
        // 0 -> 180
        for(int angle = 0; angle <= 180; angle += 10)
        {            
            while(xEventGroupGetBits(systemEventGroup) & BIT_ALARM_ON) {
                vTaskDelay(pdMS_TO_TICKS(100)); 
            }

            //ESP_LOGI("RADAR", "Kat serwa: %d stopni", angle);
            
            xQueueOverwrite(angleQueue, &angle);

            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, angle_to_duty_cycle(angle));
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);  
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        // 180 -> 0
        for(int angle = 170 ; angle >= 10 ; angle -= 10)
        {
            while(xEventGroupGetBits(systemEventGroup) & BIT_ALARM_ON) {
                vTaskDelay(pdMS_TO_TICKS(100)); 
            }
            
            //ESP_LOGI("RADAR", "Kat serwa: %d stopni", angle);
            
            xQueueOverwrite(angleQueue, &angle);

            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, angle_to_duty_cycle(angle));
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

void start_servomotor_task(void)
{
    xTaskCreate(ServoTask, "Servo_Task", 2048, NULL, 3, NULL);
}