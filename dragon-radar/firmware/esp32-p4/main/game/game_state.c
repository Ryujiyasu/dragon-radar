#include "game_state.h"

static game_state_t s_state;
static uint8_t      s_collected;

void game_init(void) { s_state = GAME_SEARCHING; s_collected = 0; }

void game_on_measurement(const uwb_measurement_t *m) { (void)m; }

game_state_t game_get_state(void) { return s_state; }
uint8_t      game_get_collected(void) { return s_collected; }
