#include "render.h"
#include "powerup.h"
#include <string.h>
#include <stdio.h>

void render_init(void) {
    initscr();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    start_color();
    use_default_colors();

    init_pair(CP_BALL,   COLOR_YELLOW,  COLOR_BLACK);
    init_pair(CP_PLAYER, COLOR_CYAN,    COLOR_BLACK);
    init_pair(CP_AI,     COLOR_RED,     COLOR_BLACK);
    init_pair(CP_SCORE,  COLOR_WHITE,   COLOR_BLACK);
    init_pair(CP_BORDER, COLOR_WHITE,   COLOR_BLACK);
    init_pair(CP_BANNER, COLOR_YELLOW,  COLOR_BLUE);
    init_pair(CP_WIN,    COLOR_GREEN,   COLOR_BLACK);
    init_pair(CP_LOSS,   COLOR_RED,     COLOR_BLACK);
    init_pair(CP_DIM,    COLOR_BLACK,   COLOR_BLACK);  /* "dark" for locked */
    init_pair(CP_GOLD,   COLOR_YELLOW,  COLOR_BLACK);
    init_pair(CP_TITLE,  COLOR_CYAN,    COLOR_BLACK);
}

void render_cleanup(void) {
    endwin();
}

/* ───────────── game screen ───────────── */

void render_game(const GameState *g) {
    erase();

    int rows = g->rows;
    int cols = g->cols;

    /* Border */
    attron(COLOR_PAIR(CP_BORDER));
    for (int c = 0; c < cols; c++) {
        mvaddch(0,        c, ACS_HLINE);
        mvaddch(rows - 1, c, ACS_HLINE);
    }
    for (int r = 1; r < rows - 1; r++) {
        mvaddch(r, 0,        ACS_VLINE);
        mvaddch(r, cols - 1, ACS_VLINE);
    }
    mvaddch(0,        0,        ACS_ULCORNER);
    mvaddch(0,        cols - 1, ACS_URCORNER);
    mvaddch(rows - 1, 0,        ACS_LLCORNER);
    mvaddch(rows - 1, cols - 1, ACS_LRCORNER);

    /* Centre dashed line */
    for (int r = 1; r < rows - 1; r += 2) {
        mvaddch(r, cols / 2, ACS_VLINE);
    }
    attroff(COLOR_PAIR(CP_BORDER));

    /* Score */
    attron(COLOR_PAIR(CP_SCORE) | A_BOLD);
    mvprintw(0, cols / 2 - 6, " %2d  :  %2d ", g->player.score, g->ai.score);
    attroff(COLOR_PAIR(CP_SCORE) | A_BOLD);

    /* Player paddle (left) — height is dynamic when adaptive mode is on */
    attron(COLOR_PAIR(CP_PLAYER) | A_BOLD);
    for (int i = 0; i < g->player_paddle_height; i++) {
        mvaddch(g->player.y + i, 2, ACS_BLOCK);
    }
    attroff(COLOR_PAIR(CP_PLAYER) | A_BOLD);

    /* AI paddle (right) — respects Shrink AI power-up */
    {
        int render_ai_h = PADDLE_HEIGHT - (g->ai_paddle_shrunk ? 2 : 0);
        attron(COLOR_PAIR(CP_AI) | A_BOLD);
        for (int i = 0; i < render_ai_h; i++) {
            mvaddch(g->ai.y + i, cols - 3, ACS_BLOCK);
        }
        attroff(COLOR_PAIR(CP_AI) | A_BOLD);
    }

    /* Ball */
    int bx = (int)g->ball.x;
    int by = (int)g->ball.y;
    if (bx > 0 && bx < cols - 1 && by > 0 && by < rows - 1) {
        int ball_cp = CP_BALL;
        char ball_ch = 'O';
        if (g->rainbow_mode) {
            int cyc = (int)(g->match_elapsed * 5.0) % 5;
            int pairs[] = { CP_BALL, CP_PLAYER, CP_AI, CP_WIN, CP_GOLD };
            ball_cp = pairs[cyc];
            ball_ch = "*+O@#"[cyc];
        }
        attron(COLOR_PAIR(ball_cp) | A_BOLD);
        mvaddch(by, bx, ball_ch);
        attroff(COLOR_PAIR(ball_cp) | A_BOLD);
    }

    /* Split ball */
    if (g->split_active) {
        int sbx = (int)g->split_ball.x;
        int sby = (int)g->split_ball.y;
        if (sbx > 0 && sbx < cols - 1 && sby > 0 && sby < rows - 1) {
            attron(COLOR_PAIR(CP_BALL) | A_BOLD);
            mvaddch(sby, sbx, '*');
            attroff(COLOR_PAIR(CP_BALL) | A_BOLD);
        }
    }

    /* Difficulty label */
    {
        float d = g->ai_difficulty;
        const char *label;
        int label_cp;
        if      (d < 0.30f) { label = "EASY";   label_cp = CP_WIN;    }
        else if (d < 0.55f) { label = "MEDIUM";  label_cp = CP_GOLD;   }
        else if (d < 0.80f) { label = "HARD";    label_cp = CP_BALL;   }
        else                { label = "EXPERT";  label_cp = CP_AI;     }
        int pct = (int)(d * 100.0f);
        attron(COLOR_PAIR(CP_SCORE));
        mvprintw(rows - 1, 2, "AI:");
        attroff(COLOR_PAIR(CP_SCORE));
        attron(COLOR_PAIR(label_cp) | A_BOLD);
        mvprintw(rows - 1, 6, "%-6s", label);
        attroff(COLOR_PAIR(label_cp) | A_BOLD);
        attron(COLOR_PAIR(CP_BORDER));
        mvprintw(rows - 1, 13, "%3d%%", pct);
        attroff(COLOR_PAIR(CP_BORDER));
    }

    /* Power-up HUD — centred on bottom border row */
    {
        const PowerUpState *pu = &g->powerup;
        int hud_col = cols / 2 - 17;
        if (hud_col < 1) hud_col = 1;

        if (pu->active != PU_NONE) {
            attron(COLOR_PAIR(CP_BANNER) | A_BOLD);
            mvprintw(rows - 1, hud_col, "ACTIVE:%-14s %4.1fs",
                     powerup_name(pu->active),
                     pu->active_timer > 0.0f ? pu->active_timer : 0.0f);
            attroff(COLOR_PAIR(CP_BANNER) | A_BOLD);
        } else {
            attron(COLOR_PAIR(CP_GOLD));
            mvprintw(rows - 1, hud_col, "PWR:%-14s [", powerup_name(pu->queued));
            attroff(COLOR_PAIR(CP_GOLD));
            attron(COLOR_PAIR(CP_PLAYER) | A_BOLD);
            for (int i = 0; i < PU_CHARGE_MAX; i++)
                addch(i < pu->charge ? '|' : ' ');
            attroff(COLOR_PAIR(CP_PLAYER) | A_BOLD);
            attron(COLOR_PAIR(CP_GOLD));
            if (pu->charge >= PU_CHARGE_MAX)
                addstr("] SPACE!");
            else
                addstr("]");
            attroff(COLOR_PAIR(CP_GOLD));
        }
    }

    /* Controls hint */
    attron(COLOR_PAIR(CP_SCORE));
    mvprintw(rows - 1, cols - 20, "W/S: Move  P: Pause");
    attroff(COLOR_PAIR(CP_SCORE));

    /* Pause overlay */
    if (g->paused) {
        attron(COLOR_PAIR(CP_BANNER) | A_BOLD);
        int py = rows / 2;
        int px = cols / 2 - 5;
        mvprintw(py,     px, "           ");
        mvprintw(py,     px, "  PAUSED   ");
        mvprintw(py + 1, px, " P to resume");
        attroff(COLOR_PAIR(CP_BANNER) | A_BOLD);
    }

    /* Achievement banner */
    if (g->banner_frames > 0) {
        attron(COLOR_PAIR(CP_BANNER) | A_BOLD);
        int bw = (int)strlen(g->banner_text) + 4;
        int bstart = cols / 2 - bw / 2;
        if (bstart < 1) bstart = 1;
        mvprintw(1, bstart, "[ %s ]", g->banner_text);
        attroff(COLOR_PAIR(CP_BANNER) | A_BOLD);
    }

    refresh();
}

/* ───────────── main menu ───────────── */

static const char *SPEED_LABELS[] = { "Slow", "Normal", "Fast", "Blazing", "Ludicrous" };
#define MENU_COUNT 5

void render_menu(int selected, int speed_idx, int adaptive_paddle) {
    clear();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    /* Title */
    attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
    const char *title = "CONSOLE  PONG";
    mvprintw(rows / 2 - 5, cols / 2 - (int)strlen(title) / 2, "%s", title);
    attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);

    /* Subtitle */
    attron(COLOR_PAIR(CP_SCORE));
    const char *sub = "1 Player vs Adaptive AI";
    mvprintw(rows / 2 - 3, cols / 2 - (int)strlen(sub) / 2, "%s", sub);
    attroff(COLOR_PAIR(CP_SCORE));

    /* Menu items: Play, Achievements, Paddle Speed, Adaptive Paddle, Quit */
    const char *labels[MENU_COUNT] = {
        "Play", "Achievements", "Paddle Speed", "Adaptive Paddle", "Quit"
    };
    int base_r = rows / 2;
    int base_c = cols / 2 - 12;

    for (int i = 0; i < MENU_COUNT; i++) {
        int r = base_r + i;
        int active = (i == selected);
        if (active) attron(COLOR_PAIR(CP_PLAYER) | A_BOLD | A_REVERSE);
        else        attron(COLOR_PAIR(CP_SCORE));

        if (i == 2) {
            mvprintw(r, base_c, "  %-16s< %-9s>", labels[i], SPEED_LABELS[speed_idx]);
        } else if (i == 3) {
            mvprintw(r, base_c, "  %-16s< %-9s>", labels[i],
                     adaptive_paddle ? "ON " : "OFF");
        } else {
            mvprintw(r, base_c, "  %-26s", labels[i]);
        }

        if (active) attroff(COLOR_PAIR(CP_PLAYER) | A_BOLD | A_REVERSE);
        else        attroff(COLOR_PAIR(CP_SCORE));
    }

    attron(COLOR_PAIR(CP_BORDER));
    mvprintw(rows / 2 + MENU_COUNT + 1,
             cols / 2 - 20,
             "UP/DOWN: navigate   LEFT/RIGHT or ENTER: change setting");
    attroff(COLOR_PAIR(CP_BORDER));

    refresh();
}

/* ───────────── achievement screen ───────────── */

void render_achievements(Achievement list[ACH_COUNT]) {
    nodelay(stdscr, FALSE);  /* blocking input for this screen */
    int rows, cols;
    int scroll = 0;
    int visible;

    while (1) {
        getmaxyx(stdscr, rows, cols);
        visible = rows - 6;
        clear();

        attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
        const char *hdr = "ACHIEVEMENTS";
        mvprintw(1, cols / 2 - (int)strlen(hdr) / 2, "%s", hdr);
        attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);

        /* Count unlocked */
        int unlocked_count = 0;
        for (int i = 0; i < ACH_COUNT; i++)
            if (list[i].unlocked) unlocked_count++;

        attron(COLOR_PAIR(CP_SCORE));
        mvprintw(2, cols / 2 - 8, "%d / %d Unlocked", unlocked_count, ACH_COUNT);
        attroff(COLOR_PAIR(CP_SCORE));

        for (int i = 0; i < visible && (scroll + i) < ACH_COUNT; i++) {
            int idx = scroll + i;
            int r = 4 + i;
            if (list[idx].unlocked) {
                attron(COLOR_PAIR(CP_GOLD) | A_BOLD);
                mvprintw(r, 3, "[*] %-20s", list[idx].name);
                attroff(A_BOLD);
                attron(COLOR_PAIR(CP_SCORE));
                mvprintw(r, 28, "%-35s", list[idx].description);
                if (list[idx].unlock_date[0]) {
                    attron(COLOR_PAIR(CP_BORDER));
                    mvprintw(r, 64, "%s", list[idx].unlock_date);
                    attroff(COLOR_PAIR(CP_BORDER));
                }
                attroff(COLOR_PAIR(CP_SCORE) | COLOR_PAIR(CP_GOLD));
            } else {
                attron(COLOR_PAIR(CP_BORDER));
                mvprintw(r, 3, "[ ] %-20s", list[idx].name);
                mvprintw(r, 28, "%-35s", list[idx].description);
                attroff(COLOR_PAIR(CP_BORDER));
            }
        }

        attron(COLOR_PAIR(CP_BORDER));
        mvprintw(rows - 1, 3, "UP/DOWN to scroll   Q or ESC to go back");
        attroff(COLOR_PAIR(CP_BORDER));

        refresh();

        int ch = getch();
        if (ch == 'q' || ch == 'Q' || ch == 27) break;
        if ((ch == KEY_UP || ch == 'w' || ch == 'W') && scroll > 0) scroll--;
        if ((ch == KEY_DOWN || ch == 's' || ch == 'S') && scroll + visible < ACH_COUNT) scroll++;
    }

    nodelay(stdscr, TRUE);
}

/* ───────────── post-match screen ───────────── */

void render_post_match(const GameState *g,
                       Achievement *newly_unlocked[],
                       int          newly_count,
                       int         *choice)
{
    nodelay(stdscr, FALSE);
    int rows, cols;

    while (1) {
        getmaxyx(stdscr, rows, cols);
        clear();

        int player_won = (g->player.score >= SCORE_TO_WIN);

        /* Big result banner */
        if (player_won) {
            attron(COLOR_PAIR(CP_WIN) | A_BOLD);
            const char *msg = "  YOU WIN!  ";
            mvprintw(rows / 2 - 5, cols / 2 - (int)strlen(msg) / 2, "%s", msg);
            attroff(COLOR_PAIR(CP_WIN) | A_BOLD);
        } else {
            attron(COLOR_PAIR(CP_LOSS) | A_BOLD);
            const char *msg = "  YOU LOSE  ";
            mvprintw(rows / 2 - 5, cols / 2 - (int)strlen(msg) / 2, "%s", msg);
            attroff(COLOR_PAIR(CP_LOSS) | A_BOLD);
        }

        /* Score */
        attron(COLOR_PAIR(CP_SCORE) | A_BOLD);
        mvprintw(rows / 2 - 3, cols / 2 - 6,
                 "  %d  —  %d  ", g->player.score, g->ai.score);
        attroff(COLOR_PAIR(CP_SCORE) | A_BOLD);

        attron(COLOR_PAIR(CP_BORDER));
        mvprintw(rows / 2 - 2, cols / 2 - 8, "  You         AI  ");
        attroff(COLOR_PAIR(CP_BORDER));

        /* Newly unlocked achievements */
        if (newly_count > 0) {
            attron(COLOR_PAIR(CP_GOLD) | A_BOLD);
            mvprintw(rows / 2 - 0, cols / 2 - 10, "  Achievements Unlocked!  ");
            attroff(COLOR_PAIR(CP_GOLD) | A_BOLD);
            for (int i = 0; i < newly_count && i < 5; i++) {
                attron(COLOR_PAIR(CP_GOLD));
                mvprintw(rows / 2 + 1 + i,
                         cols / 2 - 12,
                         "  [*] %-20s", newly_unlocked[i]->name);
                attroff(COLOR_PAIR(CP_GOLD));
            }
        }

        /* Options */
        int base_row = rows / 2 + (newly_count > 0 ? newly_count + 3 : 2);
        static int sel = 0;

        attron(sel == 0 ? (COLOR_PAIR(CP_PLAYER) | A_BOLD | A_REVERSE) : COLOR_PAIR(CP_SCORE));
        mvprintw(base_row,     cols / 2 - 8, "  Play Again  ");
        attroff(sel == 0 ? (COLOR_PAIR(CP_PLAYER) | A_BOLD | A_REVERSE) : COLOR_PAIR(CP_SCORE));

        attron(sel == 1 ? (COLOR_PAIR(CP_PLAYER) | A_BOLD | A_REVERSE) : COLOR_PAIR(CP_SCORE));
        mvprintw(base_row + 1, cols / 2 - 8, "  Main Menu   ");
        attroff(sel == 1 ? (COLOR_PAIR(CP_PLAYER) | A_BOLD | A_REVERSE) : COLOR_PAIR(CP_SCORE));

        refresh();

        int ch = getch();
        if (ch == KEY_UP   || ch == 'w' || ch == 'W') { if (sel > 0) sel--; }
        if (ch == KEY_DOWN || ch == 's' || ch == 'S') { if (sel < 1) sel++; }
        if (ch == '\n' || ch == '\r' || ch == ' ') {
            *choice = sel;
            sel = 0;
            break;
        }
    }

    nodelay(stdscr, TRUE);
}
