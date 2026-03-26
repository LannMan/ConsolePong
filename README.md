# ConsolePong

A single-player console Pong game written in C. You play against an adaptive AI opponent that adjusts its difficulty based on your win rate. Includes 10 power-ups, 23 achievements, and a handful of hidden easter eggs.

---

## Features

- **Adaptive AI** — tracks your win rate over the last 10 points and adjusts speed and accuracy dynamically; difficulty persists between sessions
- **Power-up system** — charge up by hitting the ball, press space to unleash one of 10 effects
- **23 achievements** — unlocked in-game, saved to disk and displayed in the achievements screen
- **Configurable paddle speed** — Slow / Normal / Fast / Blazing / Ludicrous
- **Adaptive paddle** — optional mode that grows or shrinks your paddle based on performance
- **Full colour** — yellow ball, cyan player paddle, red AI paddle, gold achievement banners

---

## Controls

| Key | Action |
|-----|--------|
| `W` / `↑` | Move paddle up |
| `S` / `↓` | Move paddle down |
| `K` / `J` | Vim-style up / down |
| `Space` | Activate power-up (when charged) |
| `P` | Pause / unpause |
| `Q` | Quit to menu |

Menu navigation: `↑`/`↓` to move, `Enter`/`Space` to select, `←`/`→` to change a setting.

---

## Power-Ups

Build charge by hitting the ball with your paddle. After **8 hits** the bar is full — press **Space** to fire. The next power-up is shown on the HUD while you charge so you always know what you're getting.

| Power-Up | Effect | Duration |
|----------|--------|----------|
| Paddle Stretch | Your paddle grows 3 cells taller | 6 s |
| Ball Slow | Ball drops to half speed | 5 s |
| Curve Shot | Your next hit sends the ball at an extreme angle | one hit |
| Speed Burst | Your paddle moves twice as fast | 5 s |
| Ball Split | A second ball spawns — AI must defend both | 6 s |
| AI Freeze | The AI paddle locks in place | 3 s |
| Shrink AI | The AI paddle shrinks by 2 cells | 6 s |
| Ghost Ball | Ball passes through the AI paddle and scores | one pass |
| Time Warp | Ball and AI move at half speed; you are unaffected | 5 s |
| Magnet | Ball curves toward the centre of your paddle | 4 s |

---

## Achievements

| # | Name | Condition |
|---|------|-----------|
| 1 | First Blood | Score your first ever point |
| 2 | Hat Trick | Score 3 consecutive points in a row |
| 3 | Comeback Kid | Win a match after being down 0–5 |
| 4 | Shutout | Win 11–0 |
| 5 | Nail-Biter | Win a match 11–10 |
| 6 | Rally Starter | Achieve a 10-hit rally |
| 7 | Marathon Man | Achieve a 20-hit rally |
| 8 | Bounce Master | 5 wall bounces in a single rally |
| 9 | Century | Return 100 balls (lifetime) |
| 10 | Veteran | Return 500 balls (lifetime) |
| 11 | First Win | Win your first match |
| 12 | On a Roll | Win 3 matches in a row |
| 13 | Dominant | Win 10 total matches |
| 14 | Champion | Win 50 total matches |
| 15 | Quick Match | Win in under 2 minutes |
| 16 | Underdog | Win when AI difficulty ≥ 90% |
| 17 | Speed Demon | Hit the ball at maximum speed |
| 18 | Unbeatable | Win without AI ever exceeding 50% difficulty |
| 19 | Humbled | Lose 5 consecutive matches |
| 20 | Night Owl | Play a match after midnight |
| 21 | Persistence | Lose 10 total matches and keep coming back |
| 22 | Speed Runner | Win in under 60 seconds |
| 23 | Dedicated | Play 100 total matches |

Achievement progress is saved automatically to `~/.consolepong_save` after every point and after every match.

---

## Building

### Prerequisites by platform

| Platform | Requirement |
|----------|-------------|
| Linux 64-bit | `gcc`, `make`, `libncurses-dev` (or `ncurses-devel`) |
| macOS (Apple Silicon / Intel) | Xcode Command Line Tools (`xcode-select --install`) |
| Windows | MSYS2 with MinGW-w64, PDCurses |

---

### Linux (64-bit)

**Install dependencies**

Debian / Ubuntu:
```bash
sudo apt update
sudo apt install build-essential libncurses-dev
```

Fedora / RHEL:
```bash
sudo dnf install gcc make ncurses-devel
```

Arch:
```bash
sudo pacman -S base-devel ncurses
```

**Build and run**
```bash
git clone <repo-url> ConsolePong
cd ConsolePong
make
./pong
```

---

### macOS (Apple Silicon — M1/M2/M3/M4)

macOS ships with ncurses as part of the Xcode Command Line Tools. No additional package manager is required.

**Install Command Line Tools** (if not already installed):
```bash
xcode-select --install
```

**Build and run**
```bash
git clone <repo-url> ConsolePong
cd ConsolePong
make
./pong
```

The Makefile detects `Darwin` via `uname` and links `-lncurses` automatically.

> **Terminal size:** The game uses your terminal's current dimensions. A minimum of 40 columns × 20 rows is required; 80×24 or larger is recommended. On macOS, iTerm2 and the built-in Terminal both work well.

---

### Windows

Windows requires [PDCurses](https://pdcurses.org/) and [MSYS2](https://www.msys2.org/) with the MinGW-w64 toolchain.

**Step 1 — Install MSYS2**

Download and run the installer from https://www.msys2.org/. Open the **MSYS2 MinGW64** shell (not the plain MSYS2 shell).

**Step 2 — Install the toolchain and PDCurses**
```bash
pacman -Syu
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-pdcurses
```

**Step 3 — Build**
```bash
git clone <repo-url> ConsolePong
cd ConsolePong
mingw32-make
```

The Makefile detects Windows via `uname` returning something other than `Darwin` or `Linux` and links `-lpdcurses` instead of `-lncurses`. The output binary will be `pong.exe`.

**Step 4 — Run**
```bash
./pong.exe
```

> **Note:** Run inside the MSYS2 MinGW64 terminal or a Windows Terminal configured to use it. PDCurses requires a Windows console with at least 80×24 characters.

> **Windows Defender:** On first launch, Windows may show a SmartScreen warning because the binary is unsigned. Click "More info" → "Run anyway", or right-click the `.exe` and choose Properties → Unblock.

---

## Project Structure

```
ConsolePong/
├── Makefile
├── README.md
├── src/
│   ├── main.c          — entry point, game loop, menus, input
│   ├── game.c / .h     — ball physics, paddle collision, scoring, game state
│   ├── ai.c / .h       — adaptive AI controller
│   ├── powerup.c / .h  — charge-based power-up system (10 effects)
│   ├── render.c / .h   — ncurses/PDCurses rendering, colour pairs, HUD
│   ├── achievements.c / .h — 23 achievement definitions and unlock logic
│   └── save.c / .h     — persistent save file (~/.consolepong_save)
└── docs/
    └── superpowers/
        └── specs/      — design documents
```

---

## Save File

Progress is stored in a plain-text key=value file:

| Platform | Path |
|----------|------|
| Linux / macOS | `~/.consolepong_save` |
| Windows | `%USERPROFILE%\.consolepong_save` |

To reset all progress, delete this file.

---

## Makefile Targets

| Target | Description |
|--------|-------------|
| `make` / `make all` | Build the `pong` binary |
| `make clean` | Remove the compiled binary |
