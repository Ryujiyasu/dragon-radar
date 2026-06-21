#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "lvgl.h"

#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "bsp/esp32_p4_wifi6_touch_lcd_xc.h"

#include "ui/radar_view.h"
#include "ui/theme.h"
#include "uwb/uwb_uart.h"
#include "uwb/uwb_filter.h"
#include "game/game_state.h"
#include "audio/audio_player.h"
#include "imu/imu_bno086.h"

static const char *TAG = "dragon-radar";

/* IMU heading -> world-frame azimuth. With the BNO086 the UWB azimuth (device-frame)
 * is rotated by the device heading so the blip holds its real-world bearing as you
 * turn the radar. Sign/offset are empirical (IMU mount orientation) - flip if the
 * blip rotates the wrong way. */
#define IMU_HEADING_SIGN        (+1.0f)
#define IMU_HEADING_OFFSET_DEG  (0)

/* Front-panel momentary button (active-low, internal pull-up): cycles the radar
 * display range 1m -> 3m -> 10m, like the original Dragon Radar's zoom. */
#define BUTTON_GPIO  22

static void handle_game_event(game_event_t ev)
{
    switch (ev) {
    case GAME_EV_IN_REACH:
        ESP_LOGI(TAG, "ball in reach");
        audio_player_cue(AUDIO_CUE_PROXIMITY);
        break;
    case GAME_EV_COLLECTED:
        ESP_LOGI(TAG, "ball COLLECTED (%u/%u)",
                 (unsigned)game_get_collected(), (unsigned)DR_GAME_TARGET_BALLS);
        audio_player_cue(AUDIO_CUE_COLLECTED);
        break;
    case GAME_EV_ALL_COLLECTED:
        ESP_LOGI(TAG, "ALL BALLS COLLECTED -> summoning!");
        audio_player_cue(AUDIO_CUE_SUMMON);
        break;
    default: break;
    }
}

static void uwb_task(void *arg)
{
    (void)arg;
    uwb_measurement_t m;
    int seen = 0;
    while (1) {
        if (uwb_uart_recv(&m, 1000)) {
            float heading = 0.0f;
            bool have_heading = imu_bno086_get_heading(&heading);
            if (have_heading) {
                /* device-frame azimuth -> world frame so the blip is rotation-stable */
                int wa = (int)lroundf((float)m.azimuth_deg + IMU_HEADING_SIGN * heading)
                         + IMU_HEADING_OFFSET_DEG;
                while (wa >= 180) wa -= 360;
                while (wa < -180) wa += 360;
                m.azimuth_deg = (int16_t)wa;
            }
            if (!uwb_filter_apply(&m)) continue;
            bsp_display_lock(-1);
            radar_view_set_tag(&m);
            bsp_display_unlock();
            game_on_measurement(&m);
            handle_game_event(game_poll_event());
            if ((++seen % 10) == 0) {
                ESP_LOGI(TAG, "tag=%u d=%u mm az=%d el=%d hd=%.0f%s collected=%u (n=%d)",
                         (unsigned)m.tag_id, (unsigned)m.distance_mm,
                         m.azimuth_deg, m.elevation_deg,
                         heading, have_heading ? "" : "(no-imu)",
                         (unsigned)game_get_collected(), seen);
            }
        }
    }
}

static void button_task(void *arg)
{
    (void)arg;
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    int last = 1;
    while (1) {
        int v = gpio_get_level(BUTTON_GPIO);
        if (last == 1 && v == 0) {                    /* falling edge = press */
            vTaskDelay(pdMS_TO_TICKS(30));            /* debounce */
            if (gpio_get_level(BUTTON_GPIO) == 0) {
                bsp_display_lock(-1);
                uint16_t r = radar_view_cycle_range();  /* zoom: 1m -> 3m -> 10m */
                bsp_display_unlock();
                ESP_LOGI(TAG, "button: range -> %u mm", (unsigned)r);
                audio_player_cue(AUDIO_CUE_PROXIMITY);
                while (gpio_get_level(BUTTON_GPIO) == 0) vTaskDelay(pdMS_TO_TICKS(20)); /* wait release */
            }
        }
        last = v;
        vTaskDelay(pdMS_TO_TICKS(20));
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

    /* IMU shares the BSP I2C bus (touch already brought it up). Optional: if the
     * BNO086 is absent, init fails gracefully and UWB/touch keep working. */
    if (imu_bno086_init(bsp_i2c_get_handle()) == ESP_OK) {
        imu_bno086_start_task();
    }

    game_init();
    audio_player_init();
    uwb_uart_init();
    xTaskCreate(uwb_task, "uwb_rx", 4096, NULL, 5, NULL);
    xTaskCreate(button_task, "button", 3072, NULL, 4, NULL);

    /* app_main must NOT return here: doing so tears down the display on this
     * BSP/LVGL-adapter setup (confirmed 5/19 — static UI vanished without this). */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
