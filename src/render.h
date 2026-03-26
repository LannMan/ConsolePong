#ifndef RENDER_H
#define RENDER_H

#ifdef _WIN32
  #include <pdcurses.h>
#else
  #include <ncurses.h>
#endif

#include "game.h"
#include "achievements.h"

/* Color pair IDs */
#define CP_BALL      1
#define CP_PLAYER    2
#define CP_AI        3
#define CP_SCORE     4
#define CP_BORDER    5
#define CP_BANNER    6
#define CP_WIN       7
#define CP_LOSS      8
#define CP_DIM       9
#define CP_GOLD     10
#define CP_TITLE    11

void render_init(void);
void render_cleanup(void);

void render_game(const GameState *g);
/* selected: 0=Play, 1=Achievements, 2=Speed, 3=Adaptive Paddle, 4=Quit */
void render_menu(int selected, int speed_idx, int adaptive_paddle);
void render_achievements(Achievement list[ACH_COUNT]);
void render_post_match(const GameState *g,
                       Achievement *newly_unlocked[],
                       int          newly_count,
                       int         *choice);  /* 0=play again, 1=menu */

#endif /* RENDER_H */
