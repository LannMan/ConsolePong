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

    float diff = g->ai_difficulty;
    if (player_wins > 6)       diff += 0.05f;  /* player winning too much */
    else if (player_wins < 4)  diff -= 0.05f;  /* player losing too much */

    if (diff < 0.1f) diff = 0.1f;
    if (diff > 1.0f) diff = 1.0f;
    return diff;
}

void ai_update(GameState *g, float dt) {
    if (!g->running || g->paused) return;

    float ai_speed = AI_MIN_SPEED + g->ai_difficulty * (AI_MAX_SPEED - AI_MIN_SPEED);

    /* Add reaction noise inversely proportional to difficulty */
    float noise_range = (1.0f - g->ai_difficulty) * 4.0f;
    float noise = ((float)(rand() % 1000) / 1000.0f - 0.5f) * noise_range;
    float target_y = g->ball.y + noise - PADDLE_HEIGHT / 2.0f;

    float paddle_y = (float)g->ai.y;
    float dist = target_y - paddle_y;
    float move  = ai_speed * dt;

    if (fabsf(dist) < move) {
        paddle_y = target_y;
    } else if (dist > 0) {
        paddle_y += move;
    } else {
        paddle_y -= move;
    }

    /* Clamp to board */
    if (paddle_y < 1.0f) paddle_y = 1.0f;
    if (paddle_y + PADDLE_HEIGHT > g->rows - 2)
        paddle_y = (float)(g->rows - 2 - PADDLE_HEIGHT);

    g->ai.y = (int)paddle_y;
}
