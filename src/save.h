#ifndef SAVE_H
#define SAVE_H

#define MAX_ACH_SLOTS 32

typedef struct {
    int    total_matches;
    int    total_wins;
    int    total_balls_returned;
    int    win_streak;
    int    best_win_streak;
    int    total_losses;
    int    loss_streak;
    int    best_loss_streak;
    /* achievement IDs that are unlocked, terminated by -1 */
    int    achievements[MAX_ACH_SLOTS];
    /* unlock dates parallel to achievements[] */
    char   unlock_dates[MAX_ACH_SLOTS][11]; /* "YYYY-MM-DD\0" */
} SaveData;

void save_defaults(SaveData *s);
int  save_load(SaveData *s);   /* returns 0 on success, -1 if no file (defaults used) */
int  save_write(SaveData *s);  /* returns 0 on success */

int  save_has_achievement(const SaveData *s, int id);
void save_add_achievement(SaveData *s, int id, const char *date);

#endif /* SAVE_H */
