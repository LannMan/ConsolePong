#ifndef GAME_H
#define GAME_H

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

typedef struct {
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

    /* Achievement banner */
    int     banner_frames;  /* >0 = show banner this many frames */
    char    banner_text[64];

    /* Board dimensions (set from terminal size) */
    int     rows, cols;

    /* Match timer (seconds elapsed, set externally) */
    double  match_elapsed;
} GameState;

void game_init(GameState *g, int rows, int cols);
void game_update(GameState *g, float dt);
void ball_serve(GameState *g, int toward_player);

#endif /* GAME_H */
