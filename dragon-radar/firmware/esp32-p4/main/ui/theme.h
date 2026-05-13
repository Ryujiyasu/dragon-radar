#pragma once

#include "lvgl.h"

/* Dragon Radar visual constants for Waveshare ESP32-P4-WIFI6-Touch-LCD-3.4C
 * Panel: 3.4 inch round, 800x800 IPS, MIPI-DSI 2-lane, JD9365 driver, GT911 touch.
 */
#define DR_SCREEN_SIZE      800
#define DR_CENTER           (DR_SCREEN_SIZE / 2)

/* Concentric rings inside the round visible area.
 * Scaled ~2.22x from the 360x360 (1.85") prototype: 60/120/180 -> 133/267/400.
 * Outer ring kept slightly inside the panel edge to leave room for bezel artwork.
 */
#define DR_RADAR_RING_R0    133
#define DR_RADAR_RING_R1    267
#define DR_RADAR_RING_R2    390

#define DR_MAX_TAGS         7
#define DR_RANGE_MAX_MM     10000

/* CRT-style green like Bandai Dragon Radar toy + anime Bulma radar */
#define DR_COLOR_BG         lv_color_hex(0x1A5C2E)  /* 深緑背景 (CRT 風) */
#define DR_COLOR_RADAR      lv_color_hex(0x0E3A1B)  /* グリッド線、暗い緑 */
#define DR_COLOR_RADAR_DIM  lv_color_hex(0x0A2912)  /* さらに暗い緑 */
#define DR_COLOR_SWEEP      lv_color_hex(0x80FF80)  /* スイープは明るい緑 */
#define DR_COLOR_TEXT       lv_color_hex(0xFFD700)  /* 黄色 (ドラゴンボール風) */
#define DR_COLOR_DOT        lv_color_hex(0xFFC800)  /* 黄色オレンジ (ドラゴンボール) */

extern const lv_color_t dr_tag_palette[DR_MAX_TAGS];
