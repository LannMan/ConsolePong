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

void powerup_activate(GameState *g) {
    if (g->powerup.charge < PU_CHARGE_MAX) return;
    if (g->powerup.active != PU_NONE) return;

    PowerUpType t = g->powerup.queued;
    g->powerup.active = t;
    g->powerup.charge = 0;

    switch (t) {
        case PU_PADDLE_STRETCH:
            g->player_paddle_height += 3;
            g->powerup.active_timer = 6.0f;
            break;
        case PU_BALL_SLOW:
            g->powerup.saved_ball_speed = g->ball.speed;
            g->ball.vx   *= 0.5f;
            g->ball.vy   *= 0.5f;
            g->ball.speed *= 0.5f;
            g->powerup.active_timer = 5.0f;
            break;
        case PU_CURVE_SHOT:
            g->powerup.curve_pending = 1;
            g->powerup.active_timer  = 15.0f;
            break;
        case PU_SPEED_BURST:
            g->player_speed_mult    = 2.0f;
            g->powerup.active_timer = 5.0f;
            break;
        case PU_BALL_SPLIT:
            g->split_ball        = g->ball;
            g->split_ball.vy     = -g->ball.vy;
            g->split_active      = 1;
            g->powerup.active_timer = 6.0f;
            break;
        case PU_AI_FREEZE:
            g->ai_frozen            = 1;
            g->powerup.active_timer = 3.0f;
            break;
        case PU_SHRINK_AI:
            g->ai_paddle_shrunk     = 1;
            g->powerup.active_timer = 6.0f;
            break;
        case PU_GHOST_BALL:
            g->powerup.ghost_pending = 1;
            g->powerup.active_timer  = 15.0f;
            break;
        case PU_TIME_WARP:
            g->time_warp_factor     = 0.5f;
            g->powerup.active_timer = 5.0f;
            break;
        case PU_MAGNET:
            g->powerup.active_timer = 4.0f;
            break;
        default:
            break;
    }

    g->powerup.queued = random_powerup();
}

void powerup_update(GameState *g, float dt) {
    if (g->powerup.active == PU_NONE) return;

    /* Per-frame effects */
    if (g->powerup.active == PU_MAGNET) {
        float paddle_mid = (float)g->player.y + g->player_paddle_height / 2.0f;
        float diff = paddle_mid - g->ball.y;
        float nudge = 2.0f * dt;
        if (diff > 0.0f)       g->ball.vy -= nudge;
        else if (diff < 0.0f)  g->ball.vy += nudge;
    }

    /* Always decrement the timer first (safety fallback for curve/ghost) */
    g->powerup.active_timer -= dt;

    /* Curve Shot: expire immediately once the pending hit is consumed,
       or when the safety timer runs out */
    if (g->powerup.active == PU_CURVE_SHOT &&
        (!g->powerup.curve_pending || g->powerup.active_timer <= 0.0f)) {
        g->powerup.curve_pending = 0;
        g->powerup.active = PU_NONE;
        return;
    }

    /* Ghost Ball: expire immediately once the pending pass is consumed,
       or when the safety timer runs out */
    if (g->powerup.active == PU_GHOST_BALL &&
        (!g->powerup.ghost_pending || g->powerup.active_timer <= 0.0f)) {
        g->powerup.ghost_pending = 0;
        g->powerup.active = PU_NONE;
        return;
    }

    if (g->powerup.active_timer > 0.0f) return;

    /* On expiry: reverse effects */
    switch (g->powerup.active) {
        case PU_PADDLE_STRETCH:
            g->player_paddle_height -= 3;
            if (g->player_paddle_height < PADDLE_HEIGHT)
                g->player_paddle_height = PADDLE_HEIGHT;
            break;
        case PU_BALL_SLOW: {
            /* Restore original speed magnitude, recalculate velocity direction */
            float restore = g->powerup.saved_ball_speed;
            if (restore < 0.1f) restore = BALL_SPEED_MAX; /* safety fallback */
            float cur = g->ball.speed > 0.0f ? g->ball.speed : 0.001f;
            float scale = restore / cur;
            g->ball.vx    *= scale;
            g->ball.vy    *= scale;
            g->ball.speed  = restore;
            break;
        }
        case PU_SPEED_BURST:
            g->player_speed_mult = 1.0f;
            break;
        case PU_BALL_SPLIT:
            g->split_active = 0;
            break;
        case PU_AI_FREEZE:
            g->ai_frozen = 0;
            break;
        case PU_SHRINK_AI:
            g->ai_paddle_shrunk = 0;
            break;
        case PU_TIME_WARP:
            g->time_warp_factor = 1.0f;
            break;
        case PU_MAGNET:
            break; /* no state to reverse */
        default:
            break;
    }

    g->powerup.active = PU_NONE;
}
