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

/* Bandai Dragon Radar toy reference colors */
#define DR_COLOR_BG         lv_color_hex(0x2A9040)  /* 鮮やかな緑背景 */
#define DR_COLOR_GRID       lv_color_hex(0x001008)  /* グリッド線、ほぼ黒 (くっきり) */
#define DR_COLOR_RADAR_DIM  lv_color_hex(0x0A2912)  /* 未使用 (削除候補) */
#define DR_COLOR_SWEEP      lv_color_hex(0xA0FFB0)  /* スイープは明るい緑 (半透明) */
#define DR_COLOR_TEXT       lv_color_hex(0xFFD700)  /* 黄色 */
#define DR_COLOR_DOT        lv_color_hex(0xFFC800)  /* 黄色オレンジ (ドラゴンボール) */
#define DR_COLOR_POINTER    lv_color_hex(0xFF6E50)  /* オレンジ/コーラル (Bandai 中央三角) */

/* Backward-compat alias used by older code paths */
#define DR_COLOR_RADAR      DR_COLOR_GRID

extern const lv_color_t dr_tag_palette[DR_MAX_TAGS];
