#ifndef POWERUP_H
#define POWERUP_H

/* GameState is defined in game.h — we don't forward declare here to avoid
   conflicts with game.h's typedef. Clients must include game.h to use these. */

typedef enum {
    PU_NONE = 0,
    PU_PADDLE_STRETCH,  /* 1 */
    PU_BALL_SLOW,       /* 2 */
    PU_CURVE_SHOT,      /* 3 */
    PU_SPEED_BURST,     /* 4 */
    PU_BALL_SPLIT,      /* 5 */
    PU_AI_FREEZE,       /* 6 */
    PU_SHRINK_AI,       /* 7 */
    PU_GHOST_BALL,      /* 8 */
    PU_TIME_WARP,       /* 9 */
    PU_MAGNET,          /* 10 */
    PU_COUNT            /* sentinel — not a real power-up */
} PowerUpType;

#define PU_CHARGE_MAX 8

typedef struct {
    PowerUpType queued;        /* shown on HUD while charging */
    int         charge;        /* 0 .. PU_CHARGE_MAX */
    PowerUpType active;        /* PU_NONE when idle */
    float       active_timer;  /* seconds remaining for active effect */
    int         curve_pending; /* Curve Shot: 1 until next player paddle hit */
    int         ghost_pending; /* Ghost Ball: 1 until ball passes AI column */
} PowerUpState;

struct GameState;  /* Forward declaration */

void        powerup_init(struct GameState *g);
void        powerup_on_player_hit(struct GameState *g);
void        powerup_activate(struct GameState *g);
void        powerup_update(struct GameState *g, float dt);
const char *powerup_name(PowerUpType t);

#endif /* POWERUP_H */
