# ConsolePong — Design Spec

**Date:** 2026-03-25
**Status:** Implemented

## Overview

Single-player console Pong in C. Player vs adaptive AI opponent. Full color terminal UI via ncurses (Unix) / PDCurses (Windows). 23 achievements persisted to `~/.consolepong_save`.

## Requirements

| Requirement | Decision |
|---|---|
| Language | C99 |
| Platform | Cross-platform: macOS, Linux, Windows |
| Rendering | ncurses (Unix) / PDCurses (Windows) |
| AI | Adaptive — adjusts based on last 10 points |
| Win condition | First to 11 points |
| Achievements | 23, saved to file |
| Color | Ball=yellow, player=cyan, AI=red, border=white, banner=gold-on-blue |

## Architecture

```
src/main.c          — entry point, main menu loop, play loop, frame timing
src/game.c/h        — GameState struct, ball physics, paddle collision, scoring
src/ai.c/h          — adaptive AI paddle controller
src/render.c/h      — ncurses drawing, all screens (game, menu, achievements, post-match)
src/achievements.c/h — 23 achievement defs, unlock logic
src/save.c/h        — ~/.consolepong_save read/write (plain text key=value)
Makefile            — platform-detecting build
```

## Game Mechanics

- **Board:** dynamic — uses terminal COLS × LINES
- **Ball:** starts at center, random ±30° angle; speeds up 5% per paddle hit; max 2× start speed
- **Paddle angle:** hit zone maps to ±45° deflection
- **Controls:** W/S or ↑/↓ to move; P to pause; Q to quit

## Adaptive AI

- Tracks last 10 points in a circular buffer
- If player wins >6 of last 10 → AI difficulty +0.05
- If player wins <4 of last 10 → AI difficulty -0.05
- Difficulty range: 0.1–1.0; affects paddle speed and reaction noise

## 23 Achievements

| ID | Name | Condition |
|----|------|-----------|
| 1 | First Blood | Score a point |
| 2 | Hat Trick | 3 consecutive points |
| 3 | Comeback Kid | Win after being 0–5 down |
| 4 | Shutout | Win 11–0 |
| 5 | Nail-Biter | Win 11–10 |
| 6 | Rally Starter | 10-hit rally |
| 7 | Marathon Man | 20-hit rally |
| 8 | Bounce Master | 5 wall bounces in one rally |
| 9 | Century | 100 lifetime balls returned |
| 10 | Veteran | 500 lifetime balls returned |
| 11 | First Win | Win first match |
| 12 | On a Roll | Win 3 matches in a row |
| 13 | Dominant | Win 10 total matches |
| 14 | Champion | Win 50 total matches |
| 15 | Quick Match | Win in under 2 minutes |
| 16 | Underdog | Win when AI difficulty ≥ 90% |
| 17 | Speed Demon | Hit ball at max speed |
| 18 | Unbeatable | Win without AI ever exceeding 50% |
| 19 | Humbled | Lose 5 consecutive matches |
| 20 | Night Owl | Play after midnight |
| 21 | Persistence | Lose 10 total matches |
| 22 | Speed Runner | Win in under 60 seconds |
| 23 | Dedicated | Play 100 total matches |

## Save File Format

Plain text at `~/.consolepong_save`:
```
total_matches=42
total_wins=28
total_balls_returned=1847
win_streak=3
best_win_streak=7
total_losses=14
loss_streak=0
best_loss_streak=3
achievements=1:2026-03-25,11:2026-03-25
```

## Build

```sh
# macOS/Linux
make

# Windows (MinGW + PDCurses)
make   # Makefile auto-detects _WIN32
```
