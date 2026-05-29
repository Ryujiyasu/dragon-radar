#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"

#include "bsp/esp-bsp.h"
#include "bsp/display.h"

#include "ui/radar_view.h"
#include "ui/theme.h"
#include "uwb/uwb_uart.h"
#include "uwb/uwb_filter.h"
#include "game/game_state.h"

static const char *TAG = "dragon-radar";

static void handle_game_event(game_event_t ev)
{
    switch (ev) {
    case GAME_EV_IN_REACH:
        ESP_LOGI(TAG, "ball in reach"); break;
    case GAME_EV_COLLECTED:
        ESP_LOGI(TAG, "ball COLLECTED (%u/%u)",
                 (unsigned)game_get_collected(), (unsigned)DR_GAME_TARGET_BALLS); break;
    case GAME_EV_ALL_COLLECTED:
        ESP_LOGI(TAG, "ALL BALLS COLLECTED -> summoning!"); break;
    default: break;
    }
    /* Phase 4: trigger audio cues here based on ev */
}

static void uwb_task(void *arg)
{
    (void)arg;
    uwb_measurement_t m;
    int seen = 0;
    while (1) {
        if (uwb_uart_recv(&m, 1000)) {
            if (!uwb_filter_apply(&m)) continue;
            bsp_display_lock(-1);
            radar_view_set_tag(&m);
            bsp_display_unlock();
            game_on_measurement(&m);
            handle_game_event(game_poll_event());
            if ((++seen % 10) == 0) {
                ESP_LOGI(TAG, "tag=%u d=%u mm az=%d el=%d collected=%u (n=%d)",
                         (unsigned)m.tag_id, (unsigned)m.distance_mm,
                         m.azimuth_deg, m.elevation_deg,
                         (unsigned)game_get_collected(), seen);
            }
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Dragon Radar UWB (ESP32-P4 + Waveshare 3.4C) - boot");

    bsp_display_cfg_t cfg = {
        .lv_adapter_cfg   = ESP_LV_ADAPTER_DEFAULT_CONFIG(),
        .rotation         = ESP_LV_ADAPTER_ROTATE_0,
        .tear_avoid_mode  = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL,
        .touch_flags      = { .swap_xy = 0, .mirror_x = 0, .mirror_y = 0 },
    };
    bsp_display_start_with_config(&cfg);
    bsp_display_backlight_on();

    bsp_display_lock(-1);
    radar_view_create(lv_screen_active());
    bsp_display_unlock();

    game_init();
    uwb_uart_init();
    xTaskCreate(uwb_task, "uwb_rx", 4096, NULL, 5, NULL);

    /* app_main must NOT return here: doing so tears down the display on this
     * BSP/LVGL-adapter setup (confirmed 5/19 — static UI vanished without this). */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
