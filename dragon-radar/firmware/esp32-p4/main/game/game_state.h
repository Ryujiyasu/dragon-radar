#pragma once

#include "ui/radar_view.h"

typedef enum {
    GAME_IDLE = 0,
    GAME_SEARCHING,
    GAME_COLLECTING,
    GAME_SUMMONING,
    GAME_WISH,
    GAME_END,
} game_state_t;

/* Events emitted by the game logic for audio/visual cues.
 * Poll with game_poll_event(); it returns the oldest pending event and clears it. */
typedef enum {
    GAME_EV_NONE = 0,
    GAME_EV_IN_REACH,       /* a ball entered collection range */
    GAME_EV_COLLECTED,      /* a ball was just collected */
    GAME_EV_ALL_COLLECTED,  /* target reached -> summoning */
} game_event_t;

void          game_init(void);
void          game_on_measurement(const uwb_measurement_t *m);
game_state_t  game_get_state(void);
uint8_t       game_get_collected(void);
game_event_t  game_poll_event(void);
