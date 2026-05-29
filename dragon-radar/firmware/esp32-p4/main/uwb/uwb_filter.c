#include "uwb_filter.h"
#include "ui/theme.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Per-tag sliding window. Azimuth/elevation are averaged as unit vectors
 * (sin/cos) so the result is correct across the +/-180 wrap. Distance uses a
 * plain arithmetic mean.
 *   WIN=6  @ ~5 Hz -> ~1.2 s smoothing (initial; jittery on LCD)
 *   WIN=12 @ ~5 Hz -> ~2.4 s smoothing (used now; smoother dot, slight lag)
 * Without Murata's antenna-delay calibration the SR150 itself has ~5-10 cm /
 * ~2-5 deg residual noise, so the window is what we lean on. */
#define WIN 12

typedef struct {
    int      count;
    int      head;
    uint16_t dist[WIN];
    float    az_s[WIN], az_c[WIN];
    float    el_s[WIN], el_c[WIN];
} tag_filter_t;

static tag_filter_t s_f[DR_MAX_TAGS];

void uwb_filter_reset(uint8_t tag_id)
{
    if (tag_id == 0 || tag_id > DR_MAX_TAGS) return;
    memset(&s_f[tag_id - 1], 0, sizeof(tag_filter_t));
}

bool uwb_filter_apply(uwb_measurement_t *m)
{
    if (m->distance_mm == 0 || m->distance_mm > 50000) return false;
    if (m->tag_id == 0 || m->tag_id > DR_MAX_TAGS) return false;

    tag_filter_t *f = &s_f[m->tag_id - 1];

    float az = (float)m->azimuth_deg * (float)M_PI / 180.0f;
    float el = (float)m->elevation_deg * (float)M_PI / 180.0f;

    f->dist[f->head] = m->distance_mm;
    f->az_s[f->head] = sinf(az);
    f->az_c[f->head] = cosf(az);
    f->el_s[f->head] = sinf(el);
    f->el_c[f->head] = cosf(el);
    f->head = (f->head + 1) % WIN;
    if (f->count < WIN) f->count++;

    uint32_t dsum = 0;
    float azs = 0, azc = 0, els = 0, elc = 0;
    for (int i = 0; i < f->count; i++) {
        dsum += f->dist[i];
        azs += f->az_s[i]; azc += f->az_c[i];
        els += f->el_s[i]; elc += f->el_c[i];
    }

    m->distance_mm   = (uint16_t)(dsum / f->count);
    m->azimuth_deg   = (int16_t)lroundf(atan2f(azs, azc) * 180.0f / (float)M_PI);
    m->elevation_deg = (int16_t)lroundf(atan2f(els, elc) * 180.0f / (float)M_PI);
    return true;
}
