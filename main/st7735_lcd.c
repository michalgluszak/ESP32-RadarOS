#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_st7735.h" 

#include "lvgl.h"
#include "esp_lvgl_port.h"

#include "st7735_lcd.h"
#include "Communication.h"

//PIN OUT
#define PIN_NUM_CLK    18
#define PIN_NUM_MOSI   23
#define PIN_NUM_CS     5
#define PIN_NUM_DC     2
#define PIN_NUM_RST    4

//Resolution
#define LCD_H_RES      128
#define LCD_V_RES      160

void LCD_Task(void *pvParameters)
{
    printf("Initializing SPI bus...\n");
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_CLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1, 
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * LCD_V_RES * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    printf("Initializing panel IO interface...\n");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = 20 * 1000 * 1000, 
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &io_handle));

    printf("Installing ST7735 driver...\n");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB, 
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7735(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, false)); 
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    printf("Starting LVGL engine...\n");
    
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg); 

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LCD_H_RES * LCD_V_RES,
        .double_buffer = true,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = false,
        .flags = {
            .buff_dma = true,
            .swap_bytes = true 
        }
    };
    lvgl_port_add_disp(&disp_cfg); 

    printf("Creating user interface...\n");

    // --- BUILDING LVGL INTERFACE ---
    lvgl_port_lock(0); 

    // Background to black
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);

    // 1. Servo Angle Label
    lv_obj_t *labelAngle = lv_label_create(lv_scr_act());
    lv_label_set_text(labelAngle, "Angle: --");
    lv_obj_set_style_text_color(labelAngle, lv_color_hex(0xFFFFFF), 0); // White
    lv_obj_align(labelAngle, LV_ALIGN_CENTER, 0, -45); // Top

    // 2. Radar Range Label (Potentiometer)
    lv_obj_t *labelThreshold = lv_label_create(lv_scr_act());
    lv_label_set_text(labelThreshold, "Range: -- cm");
    lv_obj_set_style_text_color(labelThreshold, lv_color_hex(0xAAAAAA), 0); // Light gray
    lv_obj_align(labelThreshold, LV_ALIGN_CENTER, 0, 0); // Center

    // 3. Status / Object Detection Label
    lv_obj_t *labelStatus = lv_label_create(lv_scr_act());
    lv_label_set_text(labelStatus, "Waiting...");
    lv_obj_set_style_text_color(labelStatus, lv_color_hex(0xFFFFFF), 0); // White
    lv_obj_align(labelStatus, LV_ALIGN_CENTER, 0, 45); // Bottom

    lvgl_port_unlock();
    // -------------------------------

    // --- MAIN TASK LOOP ---
    while(1)
    {
        int32_t current_angle = 0;
        int32_t current_threshold = 0;
        int32_t current_distance = 0;

        if (lvgl_port_lock(0)) {
            
            // 1. Update Angle
            if(xQueuePeek(angleQueue, &current_angle, 0) == pdTRUE) {
                lv_label_set_text_fmt(labelAngle, "Angle: %d degrees", (int)current_angle); 
            }

            // 2. Update Set Range
            if(xQueuePeek(thresholdQueue, &current_threshold, 0) == pdTRUE) {
                lv_label_set_text_fmt(labelThreshold, "Range: %d cm", (int)current_threshold); 
            }

            // 3. Update Status based on alarm flag
            if(xQueuePeek(distanceQueue, &current_distance, 0) == pdTRUE) {
                
                // Read event flag for the alarm state
                if(xEventGroupGetBits(systemEventGroup) & BIT_ALARM_ON) 
                {
                    // ALARM! Text to yellow and show distance
                    lv_obj_set_style_text_color(labelStatus, lv_color_hex(0xFFFF00), 0);
                    lv_label_set_text_fmt(labelStatus, "Detected: %d cm", (int)current_distance);
                } 
                else 
                {
                    // CLEAR! Text to white and appropriate message
                    lv_obj_set_style_text_color(labelStatus, lv_color_hex(0xFFFFFF), 0);
                    lv_label_set_text(labelStatus, "Status: Clear");
                }
            }
        
            lvgl_port_unlock();
        }

        // Refresh data every 100 milliseconds
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void start_st7735_task(void)
{
    xTaskCreate(LCD_Task, "LCD_Task", 4096, NULL, 1, NULL);
}