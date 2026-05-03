/*
 * Waveshare ESP32-S3-Touch-LCD-1.85 display init.
 *
 * Panel : ST77916 360x360 round LCD (QSPI)
 * Touch : CST816T capacitive (I2C)
 *
 * TODO(phase-2): Waveshare 公式 GitHub から ST77916 driver を取得して
 *   esp_lcd_panel_handle_t を初期化、LVGL の display と紐付ける。
 *   参考: https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.85
 *
 * 現状はビルドだけ通すための薄いダミー (LVGL を 360x360 で動かすが、
 * フラッシュ前に panel handle を実物に差し替えること)。
 */

#include <stdlib.h>
#include "esp_log.h"
#include "lvgl.h"

#include "ui/theme.h"

static const char *TAG = "display";

static lv_color_t  *s_buf1;
static lv_color_t  *s_buf2;
static lv_display_t *s_disp;

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    (void)area;
    (void)px_map;
    lv_display_flush_ready(disp);
}

void display_init(void)
{
    ESP_LOGW(TAG, "display_init is a STUB. Wire ST77916 driver before flashing.");

    s_disp = lv_display_create(DR_SCREEN_SIZE, DR_SCREEN_SIZE);

    size_t buf_pixels = DR_SCREEN_SIZE * 40;
    s_buf1 = malloc(buf_pixels * sizeof(lv_color_t));
    s_buf2 = malloc(buf_pixels * sizeof(lv_color_t));
    lv_display_set_buffers(s_disp, s_buf1, s_buf2,
                           buf_pixels * sizeof(lv_color_t),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(s_disp, flush_cb);
}
