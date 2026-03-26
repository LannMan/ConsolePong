#include "game.h"
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
    g->ai_difficulty = 0.3f;
    g->player.y  = (rows - PADDLE_HEIGHT) / 2;
    g->ai.y      = (rows - PADDLE_HEIGHT) / 2;
    ball_serve(g, 1);  /* serve toward player first */
}

void ball_serve(GameState *g, int toward_player) {
    g->ball.x = g->cols / 2.0f;
    g->ball.y = g->rows / 2.0f;
    g->rally_count = 0;
    g->wall_bounce_count = 0;
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

    /* Zone: where on paddle did the ball hit? -2..+2 */
    float paddle_mid = paddle->y + PADDLE_HEIGHT / 2.0f;
    float rel = (g->ball.y - paddle_mid) / (PADDLE_HEIGHT / 2.0f); /* -1..+1 */
    rel = clamp_f(rel, -1.0f, 1.0f);

    float angle = rel * (3.14159f / 4.0f); /* max ±45° */

    /* Speed up */
    g->ball.speed *= BALL_SPEED_INC;
    if (g->ball.speed > BALL_SPEED_MAX) {
        g->ball.speed = BALL_SPEED_MAX;
        g->flag_max_speed_hit = 1;
    }

    g->ball.vx = cosf(angle) * g->ball.speed * (dir > 0 ? 1.0f : -1.0f);
    g->ball.vy = sinf(angle) * g->ball.speed;

    g->rally_count++;
    if (g->rally_count >= 20) g->flag_rally_20 = 1;
    else if (g->rally_count >= 10) g->flag_rally_10 = 1;
}

void game_update(GameState *g, float dt) {
    if (!g->running || g->paused) return;

    /* Move ball */
    g->ball.x += g->ball.vx * dt;
    g->ball.y += g->ball.vy * dt;

    /* Wall bounces (top row = 1, bottom row = rows-2, accounting for border) */
    float top_wall    = 1.0f;
    float bottom_wall = (float)(g->rows - 2);

    if (g->ball.y < top_wall) {
        g->ball.y  = top_wall + (top_wall - g->ball.y);
        g->ball.vy = fabsf(g->ball.vy);
        g->wall_bounce_count++;
        if (g->wall_bounce_count >= 5) g->flag_bounce_5 = 1;
    }
    if (g->ball.y >= bottom_wall) {
        g->ball.y  = bottom_wall - (g->ball.y - bottom_wall);
        g->ball.vy = -fabsf(g->ball.vy);
        g->wall_bounce_count++;
        if (g->wall_bounce_count >= 5) g->flag_bounce_5 = 1;
    }

    int player_x = PLAYER_X;
    int ai_x     = AI_X(g->cols);

    /* Player paddle collision */
    if (g->ball.vx < 0 &&
        (int)g->ball.x <= player_x + 1 && (int)g->ball.x >= player_x &&
        (int)g->ball.y >= g->player.y && (int)g->ball.y < g->player.y + PADDLE_HEIGHT)
    {
        g->ball.x = (float)(player_x + 1);
        handle_paddle_hit(g, &g->player, player_x, 1);
    }

    /* AI paddle collision */
    if (g->ball.vx > 0 &&
        (int)g->ball.x >= ai_x - 1 && (int)g->ball.x <= ai_x &&
        (int)g->ball.y >= g->ai.y && (int)g->ball.y < g->ai.y + PADDLE_HEIGHT)
    {
        g->ball.x = (float)(ai_x - 1);
        handle_paddle_hit(g, &g->ai, ai_x, -1);
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

    /* Banner countdown */
    if (g->banner_frames > 0) g->banner_frames--;
}
