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

// Definicje pinów
#define PIN_NUM_CLK    18
#define PIN_NUM_MOSI   23
#define PIN_NUM_CS     5
#define PIN_NUM_DC     2
#define PIN_NUM_RST    4

// Rozdzielczość
#define LCD_H_RES      128
#define LCD_V_RES      160

void LCD_Task(void *pvParameters)
{
    printf("Inicjalizacja magistrali SPI...\n");
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_CLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1, 
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * LCD_V_RES * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    printf("Inicjalizacja interfejsu IO panelu...\n");
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

    printf("Instalacja sterownika ST7735...\n");
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

    printf("Uruchamianie silnika LVGL...\n");
    
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

    printf("Tworzenie interfejsu użytkownika...\n");

    // --- BUDOWA INTERFEJSU LVGL ---
    lvgl_port_lock(0); 

    // Tło na czarne
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);

    // 1. Etykieta Kąta Serwa
    lv_obj_t *labelAngle = lv_label_create(lv_scr_act());
    lv_label_set_text(labelAngle, "Kat: --");
    lv_obj_set_style_text_color(labelAngle, lv_color_hex(0xFFFFFF), 0); // Biały
    lv_obj_align(labelAngle, LV_ALIGN_CENTER, 0, -45); // Najwyżej

    // 2. Etykieta Zasięgu Radaru (Potencjometr)
    lv_obj_t *labelThreshold = lv_label_create(lv_scr_act());
    lv_label_set_text(labelThreshold, "Zasieg: -- cm");
    lv_obj_set_style_text_color(labelThreshold, lv_color_hex(0xAAAAAA), 0); // Jasnoszary
    lv_obj_align(labelThreshold, LV_ALIGN_CENTER, 0, 0); // Na środku

    // 3. Etykieta Statusu / Wykrycia Obiektu
    lv_obj_t *labelStatus = lv_label_create(lv_scr_act());
    lv_label_set_text(labelStatus, "Czekam...");
    lv_obj_set_style_text_color(labelStatus, lv_color_hex(0xFFFFFF), 0); // Biały
    lv_obj_align(labelStatus, LV_ALIGN_CENTER, 0, 45); // Najniżej

    lvgl_port_unlock();
    // -------------------------------

    // --- GŁÓWNA PĘTLA ZADANIA ---
    while(1)
    {
        int32_t current_angle = 0;
        int32_t current_threshold = 0;
        int32_t current_distance = 0;

        if (lvgl_port_lock(0)) {
            
            // 1. Aktualizacja Kąta
            if(xQueuePeek(angleQueue, &current_angle, 0) == pdTRUE) {
                lv_label_set_text_fmt(labelAngle, "Kat: %d stopni", (int)current_angle); 
            }

            // 2. Aktualizacja Ustawionego Zasięgu
            if(xQueuePeek(thresholdQueue, &current_threshold, 0) == pdTRUE) {
                lv_label_set_text_fmt(labelThreshold, "Zasieg: %d cm", (int)current_threshold); 
            }

            // 3. Aktualizacja Statusu na podstawie flagi alarmu
            if(xQueuePeek(distanceQueue, &current_distance, 0) == pdTRUE) {
                
                // Zrzutowanie flagi zdarzeń do zmiennej logicznej
                if(xEventGroupGetBits(systemEventGroup) & BIT_ALARM_ON) 
                {
                    // ALARM! Tekst na żółto i pokazujemy dystans
                    lv_obj_set_style_text_color(labelStatus, lv_color_hex(0xFFFF00), 0);
                    lv_label_set_text_fmt(labelStatus, "Wykryto: %d cm", (int)current_distance);
                } 
                else 
                {
                    // CZYSTO! Tekst na biało i odpowiedni komunikat
                    lv_obj_set_style_text_color(labelStatus, lv_color_hex(0xFFFFFF), 0);
                    lv_label_set_text(labelStatus, "Status: Czysto");
                }
            }
        
            lvgl_port_unlock();
        }

        // Odświeżanie danych co 100 milisekund
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void start_st7735_task(void)
{
    xTaskCreate(LCD_Task, "LCD_Task", 4096, NULL, 5, NULL);
}