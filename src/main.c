#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
  #include <windows.h>
  static void sleep_ms(int ms) { Sleep(ms); }
  static double get_time_sec(void) {
      LARGE_INTEGER freq, cnt;
      QueryPerformanceFrequency(&freq);
      QueryPerformanceCounter(&cnt);
      return (double)cnt.QuadPart / (double)freq.QuadPart;
  }
#else
  #include <time.h>
  static void sleep_ms(int ms) {
      struct timespec ts;
      ts.tv_sec  = ms / 1000;
      ts.tv_nsec = (ms % 1000) * 1000000L;
      nanosleep(&ts, NULL);
  }
  static double get_time_sec(void) {
      struct timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
  }
#endif

#include "game.h"
#include "ai.h"
#include "render.h"
#include "achievements.h"
#include "save.h"

#define FRAME_MS    16     /* ~60fps */
#define MAX_NEW_ACH  8

static SaveData     save;
static Achievement  ach_list[ACH_COUNT];

/* ─────────────── play loop ─────────────── */

static void play_game(void) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    if (rows < 20) rows = 20;
    if (cols < 40) cols = 40;

    GameState g;
    game_init(&g, rows, cols);

    double match_start = get_time_sec();
    double prev_time   = match_start;

    /* Track if AI difficulty ever exceeded 0.5 (for Unbeatable achievement) */
    int ai_exceeded_50 = 0;

    /* Snapshot of player score before each point for comeback detection */
    int prev_player_score = 0;
    int prev_ai_score     = 0;

    while (1) {
        double now = get_time_sec();
        double dt  = now - prev_time;
        if (dt > 0.05) dt = 0.05;  /* cap dt to avoid spiral on lag */
        prev_time = now;

        g.match_elapsed = now - match_start;
        if (g.ai_difficulty > 0.5f) ai_exceeded_50 = 1;

        /* Input */
        int ch = getch();
        if (ch == 'q' || ch == 'Q') break;

        if (ch == 'p' || ch == 'P') {
            g.paused = !g.paused;
        }

        if (!g.paused) {
            int paddle_speed_cells = 18; /* cells/sec */
            float move = (float)paddle_speed_cells * (float)dt;
            if (ch == KEY_UP   || ch == 'w' || ch == 'W' || ch == 'k' || ch == 'K') {
                g.player.y -= (int)(move + 0.5f);
                if (g.player.y < 1) g.player.y = 1;
            }
            if (ch == KEY_DOWN || ch == 's' || ch == 'S' || ch == 'j' || ch == 'J') {
                g.player.y += (int)(move + 0.5f);
                if (g.player.y + PADDLE_HEIGHT > rows - 2)
                    g.player.y = rows - 2 - PADDLE_HEIGHT;
            }
        }

        int scored_before_update = (g.player.score != prev_player_score ||
                                    g.ai.score    != prev_ai_score);

        game_update(&g, (float)dt);
        ai_update(&g, (float)dt);

        /* Detect a point just scored */
        if (g.player.score != prev_player_score || g.ai.score != prev_ai_score) {
            int player_just_scored = (g.player.score > prev_player_score);
            save.total_balls_returned++;
            ai_record_point(&g, player_just_scored);

            /* Achievement check mid-game */
            Achievement *new_ach[MAX_NEW_ACH];
            int n = achievement_check(ach_list, &g, &save,
                                      new_ach, MAX_NEW_ACH, 0, g.match_elapsed);
            if (n > 0) {
                snprintf(g.banner_text, sizeof(g.banner_text),
                         "Achievement: %s", new_ach[0]->name);
                g.banner_frames = 180;
                save_write(&save);
            }
            prev_player_score = g.player.score;
            prev_ai_score     = g.ai.score;
        }
        (void)scored_before_update;

        render_game(&g);

        /* Match over */
        if (!g.running) break;

        /* Frame timing */
        double elapsed_ms = (get_time_sec() - now) * 1000.0;
        int sleep_time = FRAME_MS - (int)elapsed_ms;
        if (sleep_time > 0) sleep_ms(sleep_time);
    }

    /* ── Post-match ── */
    int player_won = (g.player.score >= SCORE_TO_WIN);

    save.total_matches++;
    if (player_won) {
        save.total_wins++;
        save.win_streak++;
        save.loss_streak = 0;
        if (save.win_streak > save.best_win_streak)
            save.best_win_streak = save.win_streak;
    } else {
        save.total_losses++;
        save.loss_streak++;
        save.win_streak = 0;
        if (save.loss_streak > save.best_loss_streak)
            save.best_loss_streak = save.loss_streak;
    }

    if (ai_exceeded_50 == 0) {
        /* Keep difficulty low for Unbeatable check */
    } else {
        /* force off: tag game state so Unbeatable can't fire */
        g.ai_difficulty = 0.99f;
    }

    double match_elapsed = get_time_sec() - match_start;

    Achievement *new_ach[MAX_NEW_ACH];
    int n = achievement_check(ach_list, &g, &save,
                              new_ach, MAX_NEW_ACH, 1, match_elapsed);
    save_write(&save);

    int choice = 0;
    render_post_match(&g, new_ach, n, &choice);
    if (choice == 1) return;  /* back to menu */
    /* else play again: tail-recurse */
    play_game();
}

/* ─────────────── main menu loop ─────────────── */

static void main_menu(void) {
    int sel = 0;
    nodelay(stdscr, FALSE);

    while (1) {
        render_menu(sel);
        int ch = getch();

        if (ch == KEY_UP   || ch == 'w' || ch == 'W') { if (sel > 0) sel--; }
        if (ch == KEY_DOWN || ch == 's' || ch == 'S') { if (sel < 2) sel++; }

        if (ch == '\n' || ch == '\r' || ch == ' ') {
            if (sel == 0) {
                nodelay(stdscr, TRUE);
                play_game();
                nodelay(stdscr, FALSE);
            } else if (sel == 1) {
                render_achievements(ach_list);
            } else {
                break;
            }
        }
        if (ch == 'q' || ch == 'Q') break;
    }
}

/* ─────────────── entry point ─────────────── */

int main(void) {
    srand((unsigned int)time(NULL));

    save_load(&save);
    /* Ensure achievements array is terminated */
    int found_term = 0;
    for (int i = 0; i < MAX_ACH_SLOTS; i++) {
        if (save.achievements[i] == -1) { found_term = 1; break; }
    }
    if (!found_term) save.achievements[MAX_ACH_SLOTS - 1] = -1;

    render_init();
    achievements_init(ach_list, &save);
    main_menu();
    render_cleanup();

    return 0;
}
