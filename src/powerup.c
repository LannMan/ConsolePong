#include "game.h"
#include <stdlib.h>
#include <string.h>

static PowerUpType random_powerup(void) {
    return (PowerUpType)(rand() % (PU_COUNT - 1) + 1);
}

const char *powerup_name(PowerUpType t) {
    switch (t) {
        case PU_PADDLE_STRETCH: return "PADDLE STRETCH";
        case PU_BALL_SLOW:      return "BALL SLOW";
        case PU_CURVE_SHOT:     return "CURVE SHOT";
        case PU_SPEED_BURST:    return "SPEED BURST";
        case PU_BALL_SPLIT:     return "BALL SPLIT";
        case PU_AI_FREEZE:      return "AI FREEZE";
        case PU_SHRINK_AI:      return "SHRINK AI";
        case PU_GHOST_BALL:     return "GHOST BALL";
        case PU_TIME_WARP:      return "TIME WARP";
        case PU_MAGNET:         return "MAGNET";
        default:                return "NONE";
    }
}

void powerup_init(struct GameState *g) {
    g->powerup.queued      = random_powerup();
    g->powerup.charge      = 0;
    g->powerup.active      = PU_NONE;
    g->powerup.active_timer = 0.0f;
    g->powerup.curve_pending = 0;
    g->powerup.ghost_pending = 0;
    g->split_active        = 0;
    g->split_timer         = 0.0f;
    g->ai_frozen           = 0;
    g->ai_paddle_shrunk    = 0;
    g->time_warp_factor    = 1.0f;
    g->player_speed_mult   = 1.0f;
}

void powerup_on_player_hit(struct GameState *g) {
    if (g->powerup.charge < PU_CHARGE_MAX)
        g->powerup.charge++;
}

/* activate and update stubs — will be filled in later tasks */
void powerup_activate(struct GameState *g) { (void)g; }
void powerup_update(struct GameState *g, float dt) { (void)g; (void)dt; }
