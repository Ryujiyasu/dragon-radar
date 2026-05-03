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

void          game_init(void);
void          game_on_measurement(const uwb_measurement_t *m);
game_state_t  game_get_state(void);
uint8_t       game_get_collected(void);
