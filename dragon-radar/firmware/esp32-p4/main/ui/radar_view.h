#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"

typedef struct {
    uint8_t  tag_id;
    uint16_t distance_mm;
    int16_t  azimuth_deg;
    int16_t  elevation_deg;
    uint32_t timestamp_ms;
    uint8_t  rssi;
} uwb_measurement_t;

void radar_view_create(lv_obj_t *parent);
void radar_view_set_tag(const uwb_measurement_t *m);
void radar_view_remove_tag(uint8_t tag_id);
void radar_view_set_collected(uint8_t count);
uint16_t radar_view_cycle_range(void);   /* button: cycle display range (zoom); returns new range mm */

void radar_view_start_dummy_demo(void);
void radar_view_stop_dummy_demo(void);
