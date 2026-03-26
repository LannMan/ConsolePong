#ifndef GAME_H
#define GAME_H

#include "powerup.h"

#define PADDLE_HEIGHT    4
#define BALL_SPEED_INIT  18.0f
#define BALL_SPEED_MAX   36.0f
#define BALL_SPEED_INC   1.05f   /* multiply on each paddle hit */
#define SCORE_TO_WIN     11

typedef struct {
    float x, y;   /* position (float for smooth sub-cell movement) */
    float vx, vy; /* velocity in cells/sec */
    float speed;  /* current magnitude */
} Ball;

typedef struct {
    int y;        /* top row of paddle */
    int score;
} Paddle;

typedef struct GameState {
    Ball    ball;
    Paddle  player;
    Paddle  ai;

    int     running;      /* 0 = match over */
    int     paused;

    /* Rally tracking */
    int     rally_count;       /* consecutive paddle hits this rally */
    int     wall_bounce_count; /* wall bounces in current rally */

    /* Adaptive AI */
    int     last_10[10];   /* circular buffer: 0=player won point, 1=ai won */
    int     last_10_idx;
    int     last_10_count; /* how many entries are valid (0-10) */
    float   ai_difficulty; /* 0.0 – 1.0 */

    /* Per-point streak for Hat Trick achievement */
    int     player_point_streak;
    int     ai_point_streak;

    /* Flag: was player down 0-5 at any point this match? */
    int     was_down_0_5;

    /* Achievement trigger flags (set during update, consumed by checker) */
    int     flag_max_speed_hit;
    int     flag_rally_10;
    int     flag_rally_20;
    int     flag_bounce_5;

    /* Float accumulator for AI paddle (same sub-cell fix as player_y_f in main.c) */
    float   ai_y_f;

    /* Adaptive paddle: current height for the player paddle (normally PADDLE_HEIGHT) */
    int     player_paddle_height;

    int     rainbow_mode;
    int     shown_7_banner;
    int     cumulative_wall_bounces;

    float   ai_noise;     /* committed noise offset for this shot */
    int     ai_rethink;   /* set on serve/paddle-hit → re-roll noise next update */

    /* Achievement banner */
    int     banner_frames;  /* >0 = show banner this many frames */
    char    banner_text[64];

    /* Visual flash feedback when adaptive stats change */
    int     diff_flash_frames;   /* >0 while AI-difficulty display should flash */
    int     paddle_flash_frames; /* >0 while player paddle should flash */

    /* Board dimensions (set from terminal size) */
    int     rows, cols;

    /* Match timer (seconds elapsed, set externally) */
    double  match_elapsed;

    /* Power-up system */
    PowerUpState powerup;

    /* Split ball (Ball Split power-up) */
    Ball  split_ball;
    int   split_active;
    float split_timer;

    /* Effect flags */
    int   ai_frozen;           /* AI Freeze: ai.c skips movement when 1 */
    int   ai_paddle_shrunk;    /* Shrink AI: AI paddle is PADDLE_HEIGHT-2 when 1 */
    float time_warp_factor;    /* 1.0 normal, 0.5 during Time Warp */
    float player_speed_mult;   /* 1.0 normal, 2.0 during Speed Burst */
} GameState;

void game_init(GameState *g, int rows, int cols);
void game_update(GameState *g, float dt);
void ball_serve(GameState *g, int toward_player);

#endif /* GAME_H */
