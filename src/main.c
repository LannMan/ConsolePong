#define _POSIX_C_SOURCE 200809L  /* clock_gettime, nanosleep, struct timespec */
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
#include "powerup.h"

#define FRAME_MS    16     /* ~60fps */
#define MAX_NEW_ACH  8

static SaveData     save;
static Achievement  ach_list[ACH_COUNT];

/* ─────────────── play loop ─────────────── */

static const float PADDLE_SPEEDS[] = { 30.0f, 50.0f, 75.0f, 105.0f, 145.0f };

static int g_rainbow_mode = 0;

static void show_help_screen(void) {
    nodelay(stdscr, FALSE);
    clear();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int r = rows / 2 - 7;
    int c = cols / 2 - 20;
    attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
    mvprintw(r++, c, "  CONSOLEPONG HELP  ");
    attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);
    r++;
    attron(COLOR_PAIR(CP_SCORE));
    mvprintw(r++, c, "Q: Move paddle expertly");
    mvprintw(r++, c, "A: Also move paddle");
    mvprintw(r++, c, "W/S or UP/DOWN: move paddle (actually works)");
    r++;
    mvprintw(r++, c, "Strategy tips:");
    mvprintw(r++, c, "  1. Hit the ball back");
    mvprintw(r++, c, "  2. Do not miss the ball");
    mvprintw(r++, c, "  3. See tip 1");
    r++;
    mvprintw(r++, c, "Advanced technique:");
    mvprintw(r++, c, "  Move the paddle to where the ball is going.");
    mvprintw(r++, c, "  This is the entire game.");
    r++;
    attron(COLOR_PAIR(CP_BORDER));
    mvprintw(r, c, "  Press any key to close this invaluable resource.");
    attroff(COLOR_PAIR(CP_SCORE) | COLOR_PAIR(CP_BORDER));
    refresh();
    getch();
    nodelay(stdscr, TRUE);
}

static void play_game(float paddle_speed) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    if (rows < 20) rows = 20;
    if (cols < 40) cols = 40;

    GameState g;
    game_init(&g, rows, cols);
    g.ai_difficulty = save.ai_difficulty;  /* carry difficulty across matches */
    g.rainbow_mode = g_rainbow_mode;
    g_rainbow_mode = 0;

    /* Float accumulator for player paddle — avoids truncation to 0 at 60fps */
    float player_y_f = (float)g.player.y;

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
            if (ch == ' ') {
                powerup_activate(&g);
            }
            float move = paddle_speed * g.player_speed_mult * (float)dt;
            if (ch == KEY_UP   || ch == 'w' || ch == 'W' || ch == 'k' || ch == 'K')
                player_y_f -= move;
            if (ch == KEY_DOWN || ch == 's' || ch == 'S' || ch == 'j' || ch == 'J')
                player_y_f += move;
            if (player_y_f < 1.0f) player_y_f = 1.0f;
            if (player_y_f + g.player_paddle_height > rows - 2)
                player_y_f = (float)(rows - 2 - g.player_paddle_height);
            g.player.y = (int)player_y_f;
        }

        int scored_before_update = (g.player.score != prev_player_score ||
                                    g.ai.score    != prev_ai_score);

        game_update(&g, (float)dt);
        ai_update(&g, (float)dt);
        powerup_update(&g, (float)dt);

        /* Detect a point just scored */
        if (g.player.score != prev_player_score || g.ai.score != prev_ai_score) {
            int player_just_scored = (g.player.score > prev_player_score);
            save.total_balls_returned++;
            float old_difficulty = g.ai_difficulty;
            ai_record_point(&g, player_just_scored);
            if (g.ai_difficulty != old_difficulty)
                g.diff_flash_frames = 45;

            /* Easter: rally of exactly 42 */
            if (g.rally_count == 42) {
                snprintf(g.banner_text, sizeof(g.banner_text), "Don't Panic!");
                g.banner_frames = 180;
            }
            /* Easter: AI winning 7-0 */
            if (!g.shown_7_banner && g.ai.score == 7 && g.player.score == 0) {
                snprintf(g.banner_text, sizeof(g.banner_text),
                         "The AI is not even trying anymore...");
                g.banner_frames = 200;
                g.shown_7_banner = 1;
            }
            /* Easter: 100 cumulative wall bounces in one match */
            if (g.cumulative_wall_bounces == 100) {
                snprintf(g.banner_text, sizeof(g.banner_text), "Pinball Wizard!");
                g.banner_frames = 180;
            }

            /* Adaptive paddle size */
            if (save.adaptive_paddle && g.last_10_count >= 1) {
                int cnt = g.last_10_count < 10 ? g.last_10_count : 10;
                int pw = 0;
                for (int i = 0; i < cnt; i++) pw += (g.last_10[i] == 0);
                float wr = (float)pw / (float)cnt;
                int old_paddle_height = g.player_paddle_height;
                /* Only shrink on a point the player just won; only grow on a miss */
                if (player_just_scored && wr > 0.55f && g.player_paddle_height > 2)
                    g.player_paddle_height--;   /* winning → shrink */
                else if (!player_just_scored && wr < 0.45f && g.player_paddle_height < 7)
                    g.player_paddle_height++;   /* losing  → grow   */
                if (g.player_paddle_height != old_paddle_height)
                    g.paddle_flash_frames = 45;
                /* re-clamp position in case paddle shrank under the ball */
                if (player_y_f + g.player_paddle_height > rows - 2)
                    player_y_f = (float)(rows - 2 - g.player_paddle_height);
                g.player.y = (int)player_y_f;
            }

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

    /* Adjust difficulty based on match outcome and persist it */
    {
        float d = g.ai_difficulty;
        if (player_won) d += 0.15f;
        else            d -= 0.10f;
        if (d < 0.1f) d = 0.1f;
        if (d > 1.0f) d = 1.0f;
        save.ai_difficulty = d;
    }

    if (ai_exceeded_50 == 0) {
        /* Keep difficulty low for Unbeatable check */
    } else {
        /* force off: tag game state so Unbeatable can't fire */
        g.ai_difficulty = 0.99f;
    }

    double match_elapsed = get_time_sec() - match_start;

    /* Easter: 3 AM */
    {
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        if (tm->tm_hour == 3) {
            snprintf(g.banner_text, sizeof(g.banner_text), "Seriously. Go to bed.");
            g.banner_frames = 300;
        }
    }

    Achievement *new_ach[MAX_NEW_ACH];
    int n = achievement_check(ach_list, &g, &save,
                              new_ach, MAX_NEW_ACH, 1, match_elapsed);
    save_write(&save);

    int choice = 0;
    render_post_match(&g, new_ach, n, &choice);
    if (choice == 1) return;  /* back to menu */
    /* else play again: tail-recurse */
    play_game(paddle_speed);
}

/* ─────────────── main menu loop ─────────────── */

static void main_menu(void) {
    int sel = 0;
    nodelay(stdscr, FALSE);

    static const int konami[8] = {
        KEY_UP, KEY_UP, KEY_DOWN, KEY_DOWN,
        KEY_LEFT, KEY_RIGHT, KEY_LEFT, KEY_RIGHT
    };
    int konami_idx = 0;
    char word_buf[5] = {0};

    while (1) {
        render_menu(sel, save.paddle_speed_idx, save.adaptive_paddle);
        int ch = getch();

        /* Konami code */
        if (ch == konami[konami_idx]) {
            konami_idx++;
            if (konami_idx == 8) {
                g_rainbow_mode = 1;
                konami_idx = 0;
                attron(COLOR_PAIR(CP_BANNER) | A_BOLD);
                int rows2, cols2; getmaxyx(stdscr, rows2, cols2);
                mvprintw(rows2 / 2, cols2 / 2 - 8, "  RAINBOW MODE ON!  ");
                attroff(COLOR_PAIR(CP_BANNER) | A_BOLD);
                refresh();
                sleep_ms(900);
            }
        } else {
            konami_idx = (ch == konami[0]) ? 1 : 0;
        }

        /* HELP typed */
        if (ch >= 'a' && ch <= 'z') {
            memmove(word_buf, word_buf + 1, 3);
            word_buf[3] = (char)ch;
            word_buf[4] = '\0';
            if (strcmp(word_buf, "help") == 0) {
                show_help_screen();
                memset(word_buf, 0, sizeof(word_buf));
            }
        }

        if (ch == KEY_UP   || ch == 'w' || ch == 'W') { if (sel > 0) sel--; }
        if (ch == KEY_DOWN || ch == 's' || ch == 'S') { if (sel < 4) sel++; }

        if (sel == 2) {
            /* Paddle Speed: Left/Right or Enter to cycle */
            if (ch == KEY_LEFT  || ch == 'a' || ch == 'A') {
                if (save.paddle_speed_idx > 0) { save.paddle_speed_idx--; save_write(&save); }
            }
            if (ch == KEY_RIGHT || ch == 'd' || ch == 'D' ||
                ch == '\n' || ch == '\r' || ch == ' ') {
                save.paddle_speed_idx = (save.paddle_speed_idx + 1) % 5;
                save_write(&save);
            }
        } else if (sel == 3) {
            /* Adaptive Paddle: any directional key or Enter toggles */
            if (ch == KEY_LEFT  || ch == 'a' || ch == 'A' ||
                ch == KEY_RIGHT || ch == 'd' || ch == 'D' ||
                ch == '\n' || ch == '\r' || ch == ' ') {
                save.adaptive_paddle = !save.adaptive_paddle;
                save_write(&save);
            }
        } else if (ch == '\n' || ch == '\r' || ch == ' ') {
            if (sel == 0) {
                nodelay(stdscr, TRUE);
                play_game(PADDLE_SPEEDS[save.paddle_speed_idx]);
                nodelay(stdscr, FALSE);
            } else if (sel == 1) {
                render_achievements(ach_list);
            } else if (sel == 4) {
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
