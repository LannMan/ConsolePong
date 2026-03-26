# ConsolePong — Power-Ups Design

**Date:** 2026-03-25

## Overview

Add a charge-based power-up system to ConsolePong. Every player paddle hit charges a meter. After 8 hits the player presses **space** to activate the queued power-up. The next power-up is randomly drawn and shown on the HUD while the next charge builds.

---

## Power-Up List

| ID | Name | Effect | Duration |
|----|------|--------|----------|
| 1 | Paddle Stretch | Player paddle grows 3 cells taller | 6s |
| 2 | Ball Slow | Ball drops to half speed | 5s |
| 3 | Curve Shot | Next player hit sends ball at an extreme angle (beyond normal ±45°) | consumed on next hit |
| 4 | Speed Burst | Player paddle moves twice as fast | 5s |
| 5 | Ball Split | A second ball spawns with mirrored vy; AI must defend both; each scores independently | 6s |
| 6 | AI Freeze | AI paddle locks in place | 3s |
| 7 | Shrink AI | AI paddle shrinks by 2 cells | 6s |
| 8 | Ghost Ball | Ball ignores AI paddle collision; sails through to the right wall and scores | one pass |
| 9 | Time Warp | Ball and AI move at half speed; player paddle is unaffected | 5s |
| 10 | Magnet | Ball curves gently toward player paddle center each frame | 4s |

---

## Charge Mechanic

- `PU_CHARGE_MAX = 8` — number of player paddle hits to fill the bar
- Charge accumulates across rallies (not reset on point scored)
- Charge resets to 0 on activation
- If a power-up is already active when space is pressed, the activation is ignored (player must wait)
- Immediately after activation, a new power-up is drawn at random and shown

---

## HUD Display

Rendered at bottom-left of the game screen (same row as AI difficulty label, but on the right side to avoid overlap):

```
PWR: BALL SLOW  [||||||||]   ← full, press SPACE
PWR: BALL SLOW  [|||||   ]   ← charging (5/8)
ACTIVE: BALL SLOW  3.2s      ← power-up in effect
```

- Bar is 8 characters wide (one `|` per charge point)
- When `active_timer > 0`, the HUD switches to the "ACTIVE" display showing name and remaining seconds
- Color: power-up name in `CP_GOLD`, bar in `CP_PLAYER`, "ACTIVE" in `CP_BANNER`

---

## Architecture

### New Files

**`src/powerup.h`**
```c
typedef enum {
    PU_NONE = 0,
    PU_PADDLE_STRETCH,
    PU_BALL_SLOW,
    PU_CURVE_SHOT,
    PU_SPEED_BURST,
    PU_BALL_SPLIT,
    PU_AI_FREEZE,
    PU_SHRINK_AI,
    PU_GHOST_BALL,
    PU_TIME_WARP,
    PU_MAGNET,
    PU_COUNT
} PowerUpType;

#define PU_CHARGE_MAX 8

typedef struct {
    PowerUpType queued;        /* shown while charging */
    int         charge;        /* 0..PU_CHARGE_MAX */
    PowerUpType active;        /* PU_NONE when idle */
    float       active_timer;  /* seconds remaining */
    int         curve_pending; /* set when Curve Shot is active; cleared on next player hit */
    int         ghost_pending; /* set when Ghost Ball is active; cleared on ball pass */
} PowerUpState;

void  powerup_init(struct GameState *g);
void  powerup_on_player_hit(struct GameState *g);
void  powerup_activate(struct GameState *g);
void  powerup_update(struct GameState *g, float dt);
const char *powerup_name(PowerUpType t);
```

**`src/powerup.c`**

- `powerup_init`: draws first queued power-up via `rand() % (PU_COUNT - 1) + 1`
- `powerup_on_player_hit`: increment `charge`; if `curve_pending`, applies curve to the hit in-flight (called from `handle_paddle_hit` in `game.c`)
- `powerup_activate`: if charge < max or active != PU_NONE, return. Apply effect to GameState fields. Reset charge to 0. Draw next queued power-up.
- `powerup_update`: decrement `active_timer` by dt. On expiry, reverse effects. Handle per-frame effects (magnet vy nudge, split ball movement/scoring).

### Modified Files

**`src/game.h`** — additions to `GameState`:
```c
PowerUpState powerup;

/* Split ball (Ball Split power-up) */
Ball  split_ball;
int   split_active;
float split_timer;

/* Effect flags */
int   ai_frozen;           /* AI Freeze: ai.c skips movement */
int   ai_paddle_shrunk;    /* Shrink AI: ai paddle uses PADDLE_HEIGHT-2 */
float time_warp_factor;    /* 1.0 normal / 0.5 Time Warp */
float player_speed_mult;   /* 1.0 normal / 2.0 Speed Burst */
```

**`src/game.c`**
- `game_update`: scale `dt` passed to ball movement by `time_warp_factor`
- AI paddle collision: skip when `powerup.ghost_pending` is set; clear `ghost_pending` after ball passes AI column
- Split ball: `powerup_update` handles movement and scoring for the split ball (calls `g->ai.score++` / `g->player.score++` directly and clears `split_active` on exit)
- `handle_paddle_hit`: if `powerup.curve_pending`, extend angle range to ±70° instead of ±45° and clear the flag

**`src/ai.c`**
- `ai_update`: early return (after the `!running || paused` check) when `g->ai_frozen`
- `ai_speed` multiplied by `g->time_warp_factor`
- AI paddle height: use `PADDLE_HEIGHT - (g->ai_paddle_shrunk ? 2 : 0)` for clamping and collision

**`src/render.c`**
- Draw split ball as `*` using `CP_BALL` when `g->split_active`
- Draw power-up HUD at bottom-right of screen (before controls hint)

**`src/main.c`**
- Space key in game loop → `powerup_activate(&g)`
- Player paddle movement uses `paddle_speed * g.player_speed_mult`
- Call `powerup_update(&g, (float)dt)` each frame (after `ai_update`)
- Call `powerup_on_player_hit(&g)` from the per-point detection block when player just scored (already has player-hit detection via `rally_count`) — actually hook into `handle_paddle_hit` in `game.c` via a flag, or detect in main loop by comparing `rally_count`

---

## Effect Implementation Details

| Power-Up | On Activate | Per Frame | On Expire |
|----------|------------|-----------|-----------|
| Paddle Stretch | `player_paddle_height += 3` | — | `player_paddle_height -= 3` (clamped to PADDLE_HEIGHT) |
| Ball Slow | `ball.vx *= 0.5; ball.vy *= 0.5; ball.speed *= 0.5` | — | `ball.vx *= 2; ball.vy *= 2; ball.speed *= 2` |
| Curve Shot | `curve_pending = 1` | — | auto-clears on hit |
| Speed Burst | `player_speed_mult = 2.0f` | — | `player_speed_mult = 1.0f` |
| Ball Split | copy ball to split_ball, flip split_ball.vy; `split_active = 1` | move split_ball, bounce, score | `split_active = 0` |
| AI Freeze | `ai_frozen = 1` | — | `ai_frozen = 0` |
| Shrink AI | `ai_paddle_shrunk = 1` | — | `ai_paddle_shrunk = 0` |
| Ghost Ball | `ghost_pending = 1` | skip AI collision | `ghost_pending = 0` on pass |
| Time Warp | `time_warp_factor = 0.5f` | — | `time_warp_factor = 1.0f` |
| Magnet | `(active_timer set)` | nudge `ball.vy` toward paddle center by `2.0 * dt` | (timer expiry sufficient) |

---

## Edge Cases

- **Adaptive paddle + Paddle Stretch**: Stretch is additive on top of adaptive height. On expire, only the stretch amount (+3) is removed, leaving adaptive height intact.
- **Ball Split + scoring**: If split ball exits left wall, AI scores (no serve — split ball just disappears). If it exits right wall, player scores (no serve). Main ball scoring is unaffected.
- **Ball Slow + Time Warp stacked**: These can both be active. Ball Slow halves speed on activate/expire; Time Warp scales the movement dt. They are independent.
- **Ghost Ball + split ball**: Ghost only applies to the main ball. Split ball obeys normal AI collision.
- **Power-up activation while paused**: Ignored — space key input is gated behind `!g.paused`.

---

## Files Changed Summary

| File | Change |
|------|--------|
| `src/powerup.h` | **new** |
| `src/powerup.c` | **new** |
| `src/game.h` | add 8 fields to GameState |
| `src/game.c` | time warp dt scaling, ghost skip, curve angle extension |
| `src/ai.c` | ai_frozen check, time_warp_factor, shrink height |
| `src/render.c` | split ball rendering, power-up HUD |
| `src/main.c` | space key, speed mult, powerup_update call |
| `Makefile` | add powerup.c to sources |
