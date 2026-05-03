#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"

#include "ui/radar_view.h"

static const char *TAG = "dragon-radar";

extern void display_init(void);

void app_main(void)
{
    ESP_LOGI(TAG, "Dragon Radar UWB - boot");

    display_init();

    radar_view_create(lv_screen_active());
    radar_view_start_dummy_demo();

    while (1) {
        uint32_t delay_ms = lv_timer_handler();
        if (delay_ms > 33) delay_ms = 33;
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}
