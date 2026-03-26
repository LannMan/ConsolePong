#include "ai.h"
#include <stdlib.h>
#include <math.h>

void ai_record_point(GameState *g, int player_scored) {
    g->last_10[g->last_10_idx % 10] = player_scored ? 0 : 1;
    g->last_10_idx++;
    if (g->last_10_count < 10) g->last_10_count++;

    /* Recalculate difficulty */
    g->ai_difficulty = ai_calc_difficulty(g);
}

float ai_calc_difficulty(GameState *g) {
    if (g->last_10_count < 3) return g->ai_difficulty; /* not enough data yet */

    int count = g->last_10_count < 10 ? g->last_10_count : 10;
    int player_wins = 0;
    for (int i = 0; i < count; i++) {
        if (g->last_10[i] == 0) player_wins++;
    }

    float win_rate = (float)player_wins / (float)count;
    float diff = g->ai_difficulty;
    if (win_rate > 0.6f)      diff += 0.05f;  /* player winning too much */
    else if (win_rate < 0.4f) diff -= 0.05f;  /* player losing too much */

    if (diff < 0.1f) diff = 0.1f;
    if (diff > 1.0f) diff = 1.0f;
    return diff;
}

/* Predict where ball will be (in y) when it reaches x_target,
   accounting for wall bounces. Returns predicted y. */
static float predict_ball_y(const GameState *g, float x_target) {
    if (g->ball.vx <= 0.0f) return g->ball.y; /* ball moving away, no prediction */

    float travel_time = (x_target - g->ball.x) / g->ball.vx;
    float py = g->ball.y + g->ball.vy * travel_time;

    /* Reflect off top/bottom walls using the periodic reflection trick */
    float top = 1.0f;
    float bot = (float)(g->rows - 2);
    float ht  = bot - top;
    if (ht > 0.0f) {
        py -= top;
        float period = ht * 2.0f;
        py = fmodf(fabsf(py), period);
        if (py > ht) py = period - py;
        py += top;
    }
    return py;
}

void ai_update(GameState *g, float dt) {
    if (!g->running || g->paused) return;
    if (g->ai_frozen) return;

    float ai_speed = (AI_MIN_SPEED + g->ai_difficulty * (AI_MAX_SPEED - AI_MIN_SPEED))
                     * g->time_warp_factor;
    int ai_height = PADDLE_HEIGHT - (g->ai_paddle_shrunk ? 2 : 0);
    float ai_x     = (float)(g->cols - 3);

    float aim_y;
    if (g->ball.vx > 0.0f) {
        /* Ball heading toward AI — blend between pure tracking and full prediction */
        float tracked   = g->ball.y;
        float predicted = predict_ball_y(g, ai_x);
        aim_y = tracked + g->ai_difficulty * (predicted - tracked);
    } else {
        /* Ball heading away — drift back toward centre */
        aim_y = (float)(g->rows / 2);
    }

    /* Re-roll noise only when flagged (serve or paddle hit), not every frame */
    if (g->ai_rethink) {
        float noise_range = (1.0f - g->ai_difficulty) * 5.0f;
        g->ai_noise = ((float)(rand() % 1000) / 1000.0f - 0.5f) * noise_range;
        g->ai_rethink = 0;
    }
    float target_y = aim_y + g->ai_noise - ai_height / 2.0f;

    float dist = target_y - g->ai_y_f;
    float move = ai_speed * dt;

    if (fabsf(dist) < move) {
        g->ai_y_f = target_y;
    } else if (dist > 0.0f) {
        g->ai_y_f += move;
    } else {
        g->ai_y_f -= move;
    }

    /* Clamp to board */
    if (g->ai_y_f < 1.0f) g->ai_y_f = 1.0f;
    if (g->ai_y_f + ai_height > g->rows - 2)
        g->ai_y_f = (float)(g->rows - 2 - ai_height);

    g->ai.y = (int)g->ai_y_f;
}
