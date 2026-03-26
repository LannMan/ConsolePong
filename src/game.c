#include "game.h"
#include "powerup.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Player paddle is at col 2, AI paddle at col (cols-3) */
#define PLAYER_X  2
#define AI_X(cols) ((cols) - 3)

static float clamp_f(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

void game_init(GameState *g, int rows, int cols) {
    memset(g, 0, sizeof(*g));
    g->rows = rows;
    g->cols = cols;
    g->running = 1;
    g->ai_difficulty = 0.1f;
    g->player.y  = (rows - PADDLE_HEIGHT) / 2;
    g->ai.y      = (rows - PADDLE_HEIGHT) / 2;
    g->ai_y_f    = (float)g->ai.y;
    g->player_paddle_height = PADDLE_HEIGHT;
    ball_serve(g, 1);  /* serve toward player first */
    powerup_init(g);
}

void ball_serve(GameState *g, int toward_player) {
    g->ball.x = g->cols / 2.0f;
    g->ball.y = g->rows / 2.0f;
    g->rally_count = 0;
    g->wall_bounce_count = 0;
    g->ai_rethink = 1;
    g->flag_rally_10 = 0;
    g->flag_rally_20 = 0;
    g->flag_bounce_5 = 0;
    g->flag_max_speed_hit = 0;

    /* Random vertical angle: ±30° from horizontal */
    float angle = ((float)(rand() % 60) - 30.0f) * 3.14159f / 180.0f;
    g->ball.speed = BALL_SPEED_INIT;
    g->ball.vx = toward_player ? -g->ball.speed : g->ball.speed;
    g->ball.vx = cosf(angle) * g->ball.speed * (toward_player ? -1.0f : 1.0f);
    g->ball.vy = sinf(angle) * g->ball.speed;
}

static void handle_paddle_hit(GameState *g, Paddle *paddle, int paddle_x, int dir) {
    /* dir: -1 = ball going left (hit player), +1 = ball going right (hit AI) */
    (void)paddle_x;

    /* Use the correct height for whichever paddle was hit */
    int ph = (dir > 0) ? g->player_paddle_height : PADDLE_HEIGHT;  /* player hit=dir>0 */
    float paddle_mid = paddle->y + ph / 2.0f;
    float rel = (g->ball.y - paddle_mid) / (ph / 2.0f); /* -1..+1 */
    rel = clamp_f(rel, -1.0f, 1.0f);

    float max_angle = g->powerup.curve_pending
                      ? (3.14159f * 5.0f / 12.0f)  /* 75° when curved */
                      : (3.14159f / 4.0f);           /* 45° normal */
    float angle = rel * max_angle;
    if (g->powerup.curve_pending) g->powerup.curve_pending = 0;

    /* Speed up */
    g->ball.speed *= BALL_SPEED_INC;
    if (g->ball.speed > BALL_SPEED_MAX) {
        g->ball.speed = BALL_SPEED_MAX;
        g->flag_max_speed_hit = 1;
    }

    g->ball.vx = cosf(angle) * g->ball.speed * (dir > 0 ? 1.0f : -1.0f);
    g->ball.vy = sinf(angle) * g->ball.speed;

    g->ai_rethink = 1;
    g->rally_count++;
    if (g->rally_count >= 20) g->flag_rally_20 = 1;
    else if (g->rally_count >= 10) g->flag_rally_10 = 1;
    if (dir > 0) powerup_on_player_hit(g);  /* dir>0 = player just hit */
}

void game_update(GameState *g, float dt) {
    if (!g->running || g->paused) return;

    /* Move ball */
    float wdt = dt * g->time_warp_factor;
    g->ball.x += g->ball.vx * wdt;
    g->ball.y += g->ball.vy * wdt;

    /* Wall bounces (top row = 1, bottom row = rows-2, accounting for border) */
    float top_wall    = 1.0f;
    float bottom_wall = (float)(g->rows - 2);

    if (g->ball.y < top_wall) {
        g->ball.y  = top_wall + (top_wall - g->ball.y);
        g->ball.vy = fabsf(g->ball.vy);
        g->wall_bounce_count++;
        g->cumulative_wall_bounces++;
        if (g->wall_bounce_count >= 5) g->flag_bounce_5 = 1;
    }
    if (g->ball.y >= bottom_wall) {
        g->ball.y  = bottom_wall - (g->ball.y - bottom_wall);
        g->ball.vy = -fabsf(g->ball.vy);
        g->wall_bounce_count++;
        g->cumulative_wall_bounces++;
        if (g->wall_bounce_count >= 5) g->flag_bounce_5 = 1;
    }

    int player_x = PLAYER_X;
    int ai_x     = AI_X(g->cols);

    /* Player paddle collision */
    if (g->ball.vx < 0 &&
        (int)g->ball.x <= player_x + 1 && (int)g->ball.x >= player_x &&
        (int)g->ball.y >= g->player.y && (int)g->ball.y < g->player.y + g->player_paddle_height)
    {
        g->ball.x = (float)(player_x + 1);
        handle_paddle_hit(g, &g->player, player_x, 1);
    }

    int effective_ai_height = PADDLE_HEIGHT - (g->ai_paddle_shrunk ? 2 : 0);

    /* AI paddle collision — skipped when Ghost Ball is active */
    if (!g->powerup.ghost_pending && g->ball.vx > 0 &&
        (int)g->ball.x >= ai_x - 1 && (int)g->ball.x <= ai_x &&
        (int)g->ball.y >= g->ai.y &&
        (int)g->ball.y < g->ai.y + effective_ai_height)
    {
        g->ball.x = (float)(ai_x - 1);
        handle_paddle_hit(g, &g->ai, ai_x, -1);
    }
    /* Ghost Ball: clear flag once ball has passed the AI column */
    if (g->powerup.ghost_pending && g->ball.vx > 0 &&
        (int)g->ball.x > ai_x) {
        g->powerup.ghost_pending = 0;
    }

    /* Scoring */
    if (g->ball.x < 0.5f) {
        /* AI scores */
        g->ai.score++;
        g->player_point_streak = 0;
        g->ai_point_streak++;
        ball_serve(g, 0);  /* serve toward AI (player lost point) */
        if (g->player.score == 0 && g->ai.score == 5) {
            g->was_down_0_5 = 1;
        }
        if (g->ai.score >= SCORE_TO_WIN) {
            g->running = 0;
        }
    } else if (g->ball.x >= (float)(g->cols - 1)) {
        /* Player scores */
        g->player.score++;
        g->ai_point_streak = 0;
        g->player_point_streak++;
        ball_serve(g, 1);
        if (g->player.score >= SCORE_TO_WIN) {
            g->running = 0;
        }
    }

    /* Split ball physics */
    if (g->split_active) {
        g->split_ball.x += g->split_ball.vx * wdt;
        g->split_ball.y += g->split_ball.vy * wdt;

        if (g->split_ball.y < top_wall) {
            g->split_ball.y  = top_wall + (top_wall - g->split_ball.y);
            g->split_ball.vy = fabsf(g->split_ball.vy);
        }
        if (g->split_ball.y >= bottom_wall) {
            g->split_ball.y  = bottom_wall - (g->split_ball.y - bottom_wall);
            g->split_ball.vy = -fabsf(g->split_ball.vy);
        }

        /* Split ball paddle collisions (simple reflect, no angle change) */
        if (g->split_ball.vx < 0 &&
            (int)g->split_ball.x <= player_x + 1 &&
            (int)g->split_ball.x >= player_x &&
            (int)g->split_ball.y >= g->player.y &&
            (int)g->split_ball.y < g->player.y + g->player_paddle_height)
        {
            g->split_ball.x  = (float)(player_x + 1);
            g->split_ball.vx = -g->split_ball.vx;
        }
        if (g->split_ball.vx > 0 &&
            (int)g->split_ball.x >= ai_x - 1 &&
            (int)g->split_ball.x <= ai_x &&
            (int)g->split_ball.y >= g->ai.y &&
            (int)g->split_ball.y < g->ai.y + effective_ai_height)
        {
            g->split_ball.x  = (float)(ai_x - 1);
            g->split_ball.vx = -g->split_ball.vx;
        }

        /* Split ball scoring */
        if (g->split_ball.x < 0.5f) {
            g->ai.score++;
            g->split_active = 0;
            if (g->ai.score >= SCORE_TO_WIN) g->running = 0;
        } else if (g->split_ball.x >= (float)(g->cols - 1)) {
            g->player.score++;
            g->split_active = 0;
            if (g->player.score >= SCORE_TO_WIN) g->running = 0;
        }
    }

    /* Banner countdown */
    if (g->banner_frames > 0) g->banner_frames--;
}
