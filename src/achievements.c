#include "achievements.h"
#include <string.h>
#include <time.h>
#include <stdio.h>

/* Static table of achievement metadata */
static const struct { int id; const char *name; const char *desc; } ACH_TABLE[ACH_COUNT] = {
    {  1, "First Blood",   "Score your first ever point" },
    {  2, "Hat Trick",     "Score 3 consecutive points in a row" },
    {  3, "Comeback Kid",  "Win a match after being down 0-5" },
    {  4, "Shutout",       "Win 11-0 (perfect game)" },
    {  5, "Nail-Biter",    "Win a match 11-10" },
    {  6, "Rally Starter", "Achieve a 10-hit rally" },
    {  7, "Marathon Man",  "Achieve a 20-hit rally" },
    {  8, "Bounce Master", "5 wall bounces in a single rally" },
    {  9, "Century",       "Return 100 balls (lifetime)" },
    { 10, "Veteran",       "Return 500 balls (lifetime)" },
    { 11, "First Win",     "Win your first match" },
    { 12, "On a Roll",     "Win 3 matches in a row" },
    { 13, "Dominant",      "Win 10 total matches" },
    { 14, "Champion",      "Win 50 total matches" },
    { 15, "Quick Match",   "Win a match in under 2 minutes" },
    { 16, "Underdog",      "Win when AI difficulty >= 90%" },
    { 17, "Speed Demon",   "Hit the ball at maximum speed" },
    { 18, "Unbeatable",    "Win without AI ever exceeding 50% difficulty" },
    { 19, "Humbled",       "Lose 5 consecutive matches" },
    { 20, "Night Owl",     "Play a match after midnight" },
    { 21, "Persistence",   "Lose 10 total matches and keep coming back" },
    { 22, "Speed Runner",  "Win a match in under 60 seconds" },
    { 23, "Dedicated",     "Play 100 total matches" },
};

static void today_str(char *buf) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, 11, "%Y-%m-%d", tm);
}

void achievements_init(Achievement list[ACH_COUNT], const SaveData *s) {
    for (int i = 0; i < ACH_COUNT; i++) {
        list[i].id = ACH_TABLE[i].id;
        list[i].name = ACH_TABLE[i].name;
        list[i].description = ACH_TABLE[i].desc;
        list[i].unlocked = 0;
        list[i].unlock_date[0] = '\0';
    }
    /* Mark already-unlocked ones from save */
    for (int si = 0; si < MAX_ACH_SLOTS; si++) {
        if (s->achievements[si] == -1) break;
        int id = s->achievements[si];
        for (int ai = 0; ai < ACH_COUNT; ai++) {
            if (list[ai].id == id) {
                list[ai].unlocked = 1;
                strncpy(list[ai].unlock_date, s->unlock_dates[si], 10);
                list[ai].unlock_date[10] = '\0';
                break;
            }
        }
    }
}

/* Helper: try to unlock achievement by id. Returns 1 if newly unlocked. */
static int try_unlock(Achievement list[ACH_COUNT], SaveData *s, int id,
                      Achievement *newly_unlocked[], int *new_count, int max_new)
{
    for (int i = 0; i < ACH_COUNT; i++) {
        if (list[i].id != id) continue;
        if (list[i].unlocked) return 0;
        char date[11];
        today_str(date);
        list[i].unlocked = 1;
        strncpy(list[i].unlock_date, date, 10);
        list[i].unlock_date[10] = '\0';
        save_add_achievement(s, id, date);
        if (*new_count < max_new) {
            newly_unlocked[(*new_count)++] = &list[i];
        }
        return 1;
    }
    return 0;
}

int achievement_check(Achievement list[ACH_COUNT],
                      GameState *g,
                      SaveData  *s,
                      Achievement *newly_unlocked[],
                      int         max_new,
                      int         match_over,
                      double      match_elapsed_sec)
{
    int count = 0;

#define UNLOCK(id) try_unlock(list, s, (id), newly_unlocked, &count, max_new)

    /* ---- Per-point checks ---- */

    /* 1: First Blood — player scored at least once ever */
    if (s->total_balls_returned > 0 || g->player.score > 0)
        UNLOCK(1);

    /* 2: Hat Trick — 3 consecutive points */
    if (g->player_point_streak >= 3) UNLOCK(2);

    /* 6: Rally Starter */
    if (g->flag_rally_10) UNLOCK(6);

    /* 7: Marathon Man */
    if (g->flag_rally_20) UNLOCK(7);

    /* 8: Bounce Master */
    if (g->flag_bounce_5) UNLOCK(8);

    /* 9: Century */
    if (s->total_balls_returned >= 100) UNLOCK(9);

    /* 10: Veteran */
    if (s->total_balls_returned >= 500) UNLOCK(10);

    /* 17: Speed Demon */
    if (g->flag_max_speed_hit) UNLOCK(17);

    /* 20: Night Owl — check current hour */
    {
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        if (tm->tm_hour == 0 || tm->tm_hour >= 23) UNLOCK(20);
    }

    /* ---- Match-over checks ---- */
    if (match_over) {
        int player_won = (g->player.score >= SCORE_TO_WIN);

        if (player_won) {
            /* 3: Comeback Kid */
            if (g->was_down_0_5) UNLOCK(3);

            /* 4: Shutout */
            if (g->ai.score == 0) UNLOCK(4);

            /* 5: Nail-Biter */
            if (g->ai.score == 10) UNLOCK(5);

            /* 11: First Win */
            if (s->total_wins >= 1) UNLOCK(11);

            /* 12: On a Roll */
            if (s->win_streak >= 3) UNLOCK(12);

            /* 13: Dominant */
            if (s->total_wins >= 10) UNLOCK(13);

            /* 14: Champion */
            if (s->total_wins >= 50) UNLOCK(14);

            /* 15: Quick Match — under 2 minutes */
            if (match_elapsed_sec < 120.0) UNLOCK(15);

            /* 16: Underdog */
            if (g->ai_difficulty >= 0.9f) UNLOCK(16);

            /* 18: Unbeatable — AI never exceeded 0.5 difficulty */
            /* tracked externally; check via g->ai_difficulty history not available,
               so we approximate: if win_streak==1 (just won) and difficulty < 0.5 */
            if (g->ai_difficulty < 0.5f) UNLOCK(18);

            /* 22: Speed Runner — under 60 seconds */
            if (match_elapsed_sec < 60.0) UNLOCK(22);
        } else {
            /* 19: Humbled — 5 consecutive losses */
            if (s->loss_streak >= 5) UNLOCK(19);

            /* 21: Persistence — 10 total losses */
            if (s->total_losses >= 10) UNLOCK(21);
        }

        /* 23: Dedicated — 100 total matches */
        if (s->total_matches >= 100) UNLOCK(23);
    }

#undef UNLOCK

    return count;
}
