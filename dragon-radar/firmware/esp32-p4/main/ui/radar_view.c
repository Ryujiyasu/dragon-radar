#include "radar_view.h"
#include "theme.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SWEEP_PERIOD_MS     1500
#define SWEEP_ARC_WIDTH_DEG 30
#define DOT_DIAMETER_PX     30          /* 800x800 -> bigger dots than 1.85" */
#define DOT_INVISIBLE_X     -200
#define DOT_SHADOW_PX       28

typedef struct {
    bool        active;
    lv_obj_t   *dot;
    int32_t     target_x;
    int32_t     target_y;
} radar_dot_t;

static lv_obj_t   *s_radar_root;
static lv_obj_t   *s_sweep_arc;
static lv_obj_t   *s_collected_label;
static radar_dot_t s_dots[DR_MAX_TAGS];
static lv_timer_t *s_dummy_timer;

static int32_t distance_to_radius_px(uint16_t distance_mm)
{
    if (distance_mm == 0) return 0;
    if (distance_mm >= DR_RANGE_MAX_MM) return DR_RADAR_RING_R2;
    float ratio = logf(1.0f + (float)distance_mm / 1000.0f) / logf(1.0f + (float)DR_RANGE_MAX_MM / 1000.0f);
    return (int32_t)(DR_RADAR_RING_R2 * ratio);
}

static void polar_to_cartesian(uint16_t distance_mm, int16_t azimuth_deg, int32_t *out_x, int32_t *out_y)
{
    int32_t r = distance_to_radius_px(distance_mm);
    float az_rad = (float)azimuth_deg * (float)M_PI / 180.0f;
    *out_x = DR_CENTER + (int32_t)(r * sinf(az_rad));
    *out_y = DR_CENTER - (int32_t)(r * cosf(az_rad));
}

static void draw_grid(lv_obj_t *parent)
{
    /* Fine grid (Bandai CRT-style mesh).
     * Lines run across full canvas, hidden outside the circular display area by panel mask. */
    #define GRID_STEP 40
    /* vertical lines */
    for (int x = GRID_STEP; x < DR_SCREEN_SIZE; x += GRID_STEP) {
        static lv_point_precise_t pts[16][2];
        int i = x / GRID_STEP - 1;
        if (i >= 16) break;
        pts[i][0].x = x; pts[i][0].y = 0;
        pts[i][1].x = x; pts[i][1].y = DR_SCREEN_SIZE;
        lv_obj_t *line = lv_line_create(parent);
        lv_line_set_points(line, pts[i], 2);
        lv_obj_set_style_line_color(line, DR_COLOR_RADAR_DIM, 0);
        lv_obj_set_style_line_width(line, 1, 0);
        lv_obj_set_style_line_opa(line, LV_OPA_40, 0);
    }
    /* horizontal lines */
    for (int y = GRID_STEP; y < DR_SCREEN_SIZE; y += GRID_STEP) {
        static lv_point_precise_t pts[16][2];
        int i = y / GRID_STEP - 1;
        if (i >= 16) break;
        pts[i][0].x = 0;              pts[i][0].y = y;
        pts[i][1].x = DR_SCREEN_SIZE; pts[i][1].y = y;
        lv_obj_t *line = lv_line_create(parent);
        lv_line_set_points(line, pts[i], 2);
        lv_obj_set_style_line_color(line, DR_COLOR_RADAR_DIM, 0);
        lv_obj_set_style_line_width(line, 1, 0);
        lv_obj_set_style_line_opa(line, LV_OPA_40, 0);
    }
    #undef GRID_STEP
}

static void draw_concentric_rings(lv_obj_t *parent)
{
    static const int32_t radii[] = {DR_RADAR_RING_R0, DR_RADAR_RING_R1, DR_RADAR_RING_R2};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *ring = lv_obj_create(parent);
        lv_obj_remove_style_all(ring);
        lv_obj_set_size(ring, radii[i] * 2, radii[i] * 2);
        lv_obj_align(ring, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(ring, 2, 0);
        lv_obj_set_style_border_color(ring, DR_COLOR_RADAR, 0);
        lv_obj_set_style_border_opa(ring, LV_OPA_80, 0);
        lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);
        lv_obj_clear_flag(ring, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    }
}

static void draw_crosshair(lv_obj_t *parent)
{
    /* Faint crosshair (anime references show very subtle or no crosshair) */
    static lv_point_precise_t hline[] = {
        {DR_CENTER - DR_RADAR_RING_R2, DR_CENTER},
        {DR_CENTER + DR_RADAR_RING_R2, DR_CENTER},
    };
    static lv_point_precise_t vline[] = {
        {DR_CENTER, DR_CENTER - DR_RADAR_RING_R2},
        {DR_CENTER, DR_CENTER + DR_RADAR_RING_R2},
    };
    lv_obj_t *h = lv_line_create(parent);
    lv_line_set_points(h, hline, 2);
    lv_obj_set_style_line_color(h, DR_COLOR_RADAR_DIM, 0);
    lv_obj_set_style_line_width(h, 1, 0);
    lv_obj_set_style_line_opa(h, LV_OPA_50, 0);

    lv_obj_t *v = lv_line_create(parent);
    lv_line_set_points(v, vline, 2);
    lv_obj_set_style_line_color(v, DR_COLOR_RADAR_DIM, 0);
    lv_obj_set_style_line_width(v, 1, 0);
    lv_obj_set_style_line_opa(v, LV_OPA_50, 0);
}

static void draw_center_pointer(lv_obj_t *parent)
{
    /* Triangle pointing up at the center = "you are here" indicator (Bandai radar style) */
    #define PT_SIZE 24
    static lv_point_precise_t triangle[] = {
        {DR_CENTER,           DR_CENTER - PT_SIZE},      /* top */
        {DR_CENTER - PT_SIZE, DR_CENTER + PT_SIZE / 2},  /* bottom-left */
        {DR_CENTER + PT_SIZE, DR_CENTER + PT_SIZE / 2},  /* bottom-right */
        {DR_CENTER,           DR_CENTER - PT_SIZE},      /* close */
    };
    lv_obj_t *tri = lv_line_create(parent);
    lv_line_set_points(tri, triangle, 4);
    lv_obj_set_style_line_color(tri, DR_COLOR_TEXT, 0);   /* gold */
    lv_obj_set_style_line_width(tri, 3, 0);
    lv_obj_set_style_line_opa(tri, LV_OPA_COVER, 0);
    lv_obj_set_style_line_rounded(tri, true, 0);
    #undef PT_SIZE
}

static void create_sweep(lv_obj_t *parent)
{
    s_sweep_arc = lv_arc_create(parent);
    lv_obj_remove_style_all(s_sweep_arc);
    lv_obj_set_size(s_sweep_arc, DR_RADAR_RING_R2 * 2, DR_RADAR_RING_R2 * 2);
    lv_obj_align(s_sweep_arc, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_bg_angles(s_sweep_arc, 0, 360);
    lv_arc_set_rotation(s_sweep_arc, 270);
    lv_arc_set_angles(s_sweep_arc, 0, SWEEP_ARC_WIDTH_DEG);
    lv_obj_set_style_arc_color(s_sweep_arc, DR_COLOR_SWEEP, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_sweep_arc, DR_RADAR_RING_R2, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(s_sweep_arc, LV_OPA_30, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(s_sweep_arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_remove_flag(s_sweep_arc, LV_OBJ_FLAG_CLICKABLE);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_sweep_arc);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_arc_set_rotation);
    lv_anim_set_values(&a, 270, 270 + 360);
    lv_anim_set_duration(&a, SWEEP_PERIOD_MS);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);
}

static void create_dots(lv_obj_t *parent)
{
    for (int i = 0; i < DR_MAX_TAGS; i++) {
        lv_obj_t *dot = lv_obj_create(parent);
        lv_obj_remove_style_all(dot);
        lv_obj_set_size(dot, DOT_DIAMETER_PX, DOT_DIAMETER_PX);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, dr_tag_palette[i], 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_shadow_color(dot, dr_tag_palette[i], 0);
        lv_obj_set_style_shadow_width(dot, DOT_SHADOW_PX, 0);
        lv_obj_set_style_shadow_opa(dot, LV_OPA_70, 0);
        lv_obj_set_pos(dot, DOT_INVISIBLE_X, DOT_INVISIBLE_X);
        lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        s_dots[i].dot = dot;
        s_dots[i].active = false;
    }
}

static void create_collected_label(lv_obj_t *parent)
{
    s_collected_label = lv_label_create(parent);
    lv_label_set_text(s_collected_label, "0 / 7");
    lv_obj_set_style_text_color(s_collected_label, DR_COLOR_TEXT, 0);
    lv_obj_align(s_collected_label, LV_ALIGN_TOP_RIGHT, -50, 50);
}

static void dummy_demo_cb(lv_timer_t *t)
{
    static uint32_t tick;
    tick++;
    for (int i = 0; i < DR_MAX_TAGS; i++) {
        uwb_measurement_t m = {
            .tag_id      = (uint8_t)(i + 1),
            .distance_mm = (uint16_t)(2000 + i * 800 + 500 * sinf((tick + i * 30) * 0.05f)),
            .azimuth_deg = (int16_t)(((tick * 2 + i * 51) % 360) - 180),
        };
        radar_view_set_tag(&m);
    }
}

void radar_view_create(lv_obj_t *parent)
{
    s_radar_root = lv_obj_create(parent);
    lv_obj_remove_style_all(s_radar_root);
    lv_obj_set_size(s_radar_root, DR_SCREEN_SIZE, DR_SCREEN_SIZE);
    lv_obj_align(s_radar_root, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(s_radar_root, DR_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(s_radar_root, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_radar_root, LV_OBJ_FLAG_SCROLLABLE);

    draw_grid(s_radar_root);
    draw_concentric_rings(s_radar_root);
    draw_crosshair(s_radar_root);
    create_sweep(s_radar_root);
    create_dots(s_radar_root);
    draw_center_pointer(s_radar_root);
    create_collected_label(s_radar_root);
}

void radar_view_set_tag(const uwb_measurement_t *m)
{
    if (m->tag_id == 0 || m->tag_id > DR_MAX_TAGS) return;
    radar_dot_t *d = &s_dots[m->tag_id - 1];

    int32_t x, y;
    polar_to_cartesian(m->distance_mm, m->azimuth_deg, &x, &y);

    d->active = true;
    lv_obj_clear_flag(d->dot, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(d->dot, x - DOT_DIAMETER_PX / 2, y - DOT_DIAMETER_PX / 2);
}

void radar_view_remove_tag(uint8_t tag_id)
{
    if (tag_id == 0 || tag_id > DR_MAX_TAGS) return;
    radar_dot_t *d = &s_dots[tag_id - 1];
    d->active = false;
    lv_obj_add_flag(d->dot, LV_OBJ_FLAG_HIDDEN);
}

void radar_view_set_collected(uint8_t count)
{
    if (count > DR_MAX_TAGS) count = DR_MAX_TAGS;
    lv_label_set_text_fmt(s_collected_label, "%u / %u", count, DR_MAX_TAGS);
}

void radar_view_start_dummy_demo(void)
{
    if (s_dummy_timer) return;
    s_dummy_timer = lv_timer_create(dummy_demo_cb, 33, NULL);
}

void radar_view_stop_dummy_demo(void)
{
    if (s_dummy_timer) {
        lv_timer_delete(s_dummy_timer);
        s_dummy_timer = NULL;
    }
}
