#include "uwb_filter.h"

void uwb_filter_reset(uint8_t tag_id) { (void)tag_id; }

bool uwb_filter_apply(uwb_measurement_t *m)
{
    if (m->distance_mm == 0 || m->distance_mm > 50000) return false;
    return true;
}
