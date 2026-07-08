#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_adc/adc_oneshot.h>
#include "esp_log.h"
#include "esp_err.h"

#include "Potentiometer.h"
#include "Communication.h"

// GPIO35 WHITE
#define ADC_PIN ADC_CHANNEL_7    
#define ADC_UNIT ADC_UNIT_1      
#define ADC_BITWIDTH ADC_BITWIDTH_12  
#define ADC_ATTEN ADC_ATTEN_DB_12  

static int32_t mapValue(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static void SetDistanceValuePotentiometerTask(void *pvParameters)
{
    adc_oneshot_unit_handle_t handleADC;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &handleADC));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(handleADC, ADC_PIN, &config));

    int distanceADC = 0;
    int32_t mappedDistance_cm = 0;

    while(1)
    {
        ESP_ERROR_CHECK(adc_oneshot_read(handleADC, ADC_PIN, &distanceADC));
        
        mappedDistance_cm = mapValue(distanceADC, 0, 4095, 1, 100);
        xQueueOverwrite(thresholdQueue, &mappedDistance_cm);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void start_potentiometer_task(void)
{
    xTaskCreate(SetDistanceValuePotentiometerTask, "Potentiometer Task", 2048, NULL, 5, NULL);
}