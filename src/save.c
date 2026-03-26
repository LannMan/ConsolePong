#include "save.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #include <windows.h>
  #define PATH_SEP "\\"
#else
  #include <unistd.h>
  #define PATH_SEP "/"
#endif

static void get_save_path(char *buf, int buflen) {
#ifdef _WIN32
    const char *home = getenv("USERPROFILE");
    if (!home) home = "C:\\";
#else
    const char *home = getenv("HOME");
    if (!home) home = "/tmp";
#endif
    snprintf(buf, buflen, "%s%s.consolepong_save", home, PATH_SEP);
}

static void get_tmp_path(char *buf, int buflen) {
#ifdef _WIN32
    const char *home = getenv("USERPROFILE");
    if (!home) home = "C:\\";
#else
    const char *home = getenv("HOME");
    if (!home) home = "/tmp";
#endif
    snprintf(buf, buflen, "%s%s.consolepong_save.tmp", home, PATH_SEP);
}

void save_defaults(SaveData *s) {
    memset(s, 0, sizeof(*s));
    memset(s->achievements, -1, sizeof(s->achievements));
    s->ai_difficulty = 0.1f;
}

int save_has_achievement(const SaveData *s, int id) {
    for (int i = 0; i < MAX_ACH_SLOTS; i++) {
        if (s->achievements[i] == -1) break;
        if (s->achievements[i] == id)  return 1;
    }
    return 0;
}

void save_add_achievement(SaveData *s, int id, const char *date) {
    if (save_has_achievement(s, id)) return;
    for (int i = 0; i < MAX_ACH_SLOTS; i++) {
        if (s->achievements[i] == -1) {
            s->achievements[i] = id;
            strncpy(s->unlock_dates[i], date, 10);
            s->unlock_dates[i][10] = '\0';
            if (i + 1 < MAX_ACH_SLOTS) s->achievements[i + 1] = -1;
            return;
        }
    }
}

int save_load(SaveData *s) {
    save_defaults(s);
    char path[512];
    get_save_path(path, sizeof(path));

    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        /* Strip newline */
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        /* Strip comment */
        char *hash = strchr(line, '#');
        if (hash) *hash = '\0';

        char key[64], val[192];
        if (sscanf(line, "%63[^=]=%191s", key, val) != 2) continue;

        if      (strcmp(key, "total_matches")       == 0) s->total_matches       = atoi(val);
        else if (strcmp(key, "total_wins")          == 0) s->total_wins          = atoi(val);
        else if (strcmp(key, "total_balls_returned")== 0) s->total_balls_returned= atoi(val);
        else if (strcmp(key, "win_streak")          == 0) s->win_streak          = atoi(val);
        else if (strcmp(key, "best_win_streak")     == 0) s->best_win_streak     = atoi(val);
        else if (strcmp(key, "total_losses")        == 0) s->total_losses        = atoi(val);
        else if (strcmp(key, "loss_streak")         == 0) s->loss_streak         = atoi(val);
        else if (strcmp(key, "best_loss_streak")    == 0) s->best_loss_streak    = atoi(val);
        else if (strcmp(key, "paddle_speed_idx")    == 0) s->paddle_speed_idx    = atoi(val);
        else if (strcmp(key, "adaptive_paddle")     == 0) s->adaptive_paddle     = atoi(val);
        else if (strcmp(key, "ai_difficulty")       == 0) s->ai_difficulty       = (float)atof(val);
        else if (strcmp(key, "achievements")        == 0) {
            /* Parse comma-separated IDs, optional :YYYY-MM-DD suffix */
            int ach_idx = 0;
            char *tok = strtok(val, ",");
            while (tok && ach_idx < MAX_ACH_SLOTS - 1) {
                char date_buf[11] = "";
                int id = atoi(tok);
                char *colon = strchr(tok, ':');
                if (colon) { strncpy(date_buf, colon + 1, 10); date_buf[10] = '\0'; }
                s->achievements[ach_idx] = id;
                strncpy(s->unlock_dates[ach_idx], date_buf, 10);
                s->unlock_dates[ach_idx][10] = '\0';
                ach_idx++;
                tok = strtok(NULL, ",");
            }
            s->achievements[ach_idx] = -1;
        }
    }
    fclose(f);
    return 0;
}

int save_write(SaveData *s) {
    char tmp_path[512], save_path[512];
    get_tmp_path(tmp_path, sizeof(tmp_path));
    get_save_path(save_path, sizeof(save_path));

    FILE *f = fopen(tmp_path, "w");
    if (!f) return -1;

    fprintf(f, "total_matches=%d\n",        s->total_matches);
    fprintf(f, "total_wins=%d\n",           s->total_wins);
    fprintf(f, "total_balls_returned=%d\n", s->total_balls_returned);
    fprintf(f, "win_streak=%d\n",           s->win_streak);
    fprintf(f, "best_win_streak=%d\n",      s->best_win_streak);
    fprintf(f, "total_losses=%d\n",         s->total_losses);
    fprintf(f, "loss_streak=%d\n",          s->loss_streak);
    fprintf(f, "best_loss_streak=%d\n",     s->best_loss_streak);
    fprintf(f, "paddle_speed_idx=%d\n",     s->paddle_speed_idx);
    fprintf(f, "adaptive_paddle=%d\n",      s->adaptive_paddle);
    fprintf(f, "ai_difficulty=%.4f\n",      s->ai_difficulty);

    /* achievements as id:date,id:date,... */
    fprintf(f, "achievements=");
    int first = 1;
    for (int i = 0; i < MAX_ACH_SLOTS; i++) {
        if (s->achievements[i] == -1) break;
        if (!first) fprintf(f, ",");
        fprintf(f, "%d:%s", s->achievements[i], s->unlock_dates[i]);
        first = 0;
    }
    fprintf(f, "\n");

    fclose(f);

#ifdef _WIN32
    /* Windows: remove then rename */
    remove(save_path);
    return rename(tmp_path, save_path);
#else
    return rename(tmp_path, save_path);
#endif
}
