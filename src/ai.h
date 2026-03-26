#ifndef AI_H
#define AI_H

#include "game.h"

#define AI_MIN_SPEED   8.0f
#define AI_MAX_SPEED  20.0f

void  ai_update(GameState *g, float dt);
void  ai_record_point(GameState *g, int player_scored);
float ai_calc_difficulty(GameState *g);

#endif /* AI_H */
