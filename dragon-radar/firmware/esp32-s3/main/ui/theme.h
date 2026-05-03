#pragma once

#include "lvgl.h"

#define DR_SCREEN_SIZE      360
#define DR_CENTER           (DR_SCREEN_SIZE / 2)

#define DR_RADAR_RING_R0    60
#define DR_RADAR_RING_R1    120
#define DR_RADAR_RING_R2    180

#define DR_MAX_TAGS         7
#define DR_RANGE_MAX_MM     10000

#define DR_COLOR_BG         lv_color_hex(0x000000)
#define DR_COLOR_RADAR      lv_color_hex(0x00FF66)
#define DR_COLOR_RADAR_DIM  lv_color_hex(0x004D1F)
#define DR_COLOR_TEXT       lv_color_hex(0xCCFFCC)

extern const lv_color_t dr_tag_palette[DR_MAX_TAGS];
