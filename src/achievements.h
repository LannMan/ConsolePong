#ifndef ACHIEVEMENTS_H
#define ACHIEVEMENTS_H

#include "game.h"
#include "save.h"

#define ACH_COUNT 23

typedef struct {
    int         id;
    const char *name;
    const char *description;
    int         unlocked;
    char        unlock_date[11];  /* "YYYY-MM-DD\0" */
} Achievement;

/* Fill the array with all 23 achievement definitions (unlocked=0, date="") */
void achievements_init(Achievement list[ACH_COUNT], const SaveData *s);

/*
 * Check all achievements after a point or match end.
 * Populates newly_unlocked[] with pointers to newly unlocked achievements.
 * Returns count of newly unlocked.
 * Also calls save_add_achievement for each new one.
 */
int achievement_check(Achievement list[ACH_COUNT],
                      GameState *g,
                      SaveData  *s,
                      Achievement *newly_unlocked[],
                      int         max_new,
                      int         match_over,
                      double      match_elapsed_sec);

#endif /* ACHIEVEMENTS_H */
