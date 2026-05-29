#include "game_state.h"
#include "ui/theme.h"

#include <string.h>

typedef struct {
    bool     collected;
    bool     in_reach;       /* currently within collection range */
    uint32_t reach_since_ms; /* when it entered range */
} ball_state_t;

static game_state_t s_state;
static uint8_t      s_collected;
static ball_state_t s_balls[DR_MAX_TAGS];

/* single-slot event queue: the more important pending event wins
 * (ALL_COLLECTED > COLLECTED > IN_REACH > NONE, matching enum order). */
static game_event_t s_pending;

static void push_event(game_event_t ev)
{
    if (ev > s_pending) s_pending = ev;
}

void game_init(void)
{
    s_state = GAME_SEARCHING;
    s_collected = 0;
    s_pending = GAME_EV_NONE;
    memset(s_balls, 0, sizeof(s_balls));
}

void game_on_measurement(const uwb_measurement_t *m)
{
    if (m->tag_id == 0 || m->tag_id > DR_MAX_TAGS) return;
    ball_state_t *b = &s_balls[m->tag_id - 1];

    if (b->collected) return;

    if (m->distance_mm <= DR_COLLECT_THRESH_MM) {
        if (!b->in_reach) {
            b->in_reach = true;
            b->reach_since_ms = m->timestamp_ms;
            push_event(GAME_EV_IN_REACH);
        }
        else if ((m->timestamp_ms - b->reach_since_ms) >= DR_COLLECT_HOLD_MS) {
            b->collected = true;
            b->in_reach = false;
            s_collected++;
            radar_view_set_collected(s_collected);

            if (s_state == GAME_SEARCHING) s_state = GAME_COLLECTING;

            if (s_collected >= DR_GAME_TARGET_BALLS) {
                s_state = GAME_SUMMONING;
                push_event(GAME_EV_ALL_COLLECTED);
            } else {
                push_event(GAME_EV_COLLECTED);
            }
        }
    } else {
        /* left range before hold completed: reset the timer */
        b->in_reach = false;
        b->reach_since_ms = 0;
    }
}

game_state_t game_get_state(void)     { return s_state; }
uint8_t      game_get_collected(void) { return s_collected; }

game_event_t game_poll_event(void)
{
    game_event_t ev = s_pending;
    s_pending = GAME_EV_NONE;
    return ev;
}
