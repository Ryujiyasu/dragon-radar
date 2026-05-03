#pragma once

#include "ui/radar_view.h"

void uwb_filter_reset(uint8_t tag_id);
bool uwb_filter_apply(uwb_measurement_t *m);
