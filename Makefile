CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -O2
SRC_DIR = src
SRCS    = $(SRC_DIR)/main.c \
          $(SRC_DIR)/game.c \
          $(SRC_DIR)/ai.c \
          $(SRC_DIR)/render.c \
          $(SRC_DIR)/achievements.c \
          $(SRC_DIR)/save.c \
          $(SRC_DIR)/powerup.c
TARGET  = pong

# Platform detection
UNAME := $(shell uname 2>/dev/null || echo Windows)

ifneq (,$(findstring MINGW,$(UNAME)))
    # MSYS2/MinGW on Windows:
    # Statically link PDCurses so the .exe doesn't need libpdcurses.dll at runtime.
    # gdi32/comdlg32/winmm are PDCurses WinGUI dependencies — always present on Windows.
    # -static-libgcc bakes in libgcc so the .exe needs no extra MinGW DLLs.
    LIBS    = -Wl,-Bstatic -lpdcurses -Wl,-Bdynamic -lgdi32 -lcomdlg32 -lwinmm -static-libgcc
    TARGET  = pong.exe
else ifeq ($(UNAME), Darwin)
    # macOS: use system ncurses (ships with Xcode CLT)
    LIBS    = -lncurses -lm
else
    # Linux: use ncursesw (wide-char ncurses) — universally available across distros
    # (Arch ships only libncursesw.so.6; Ubuntu ships both but also has libncursesw.so.6)
    LIBS    = -lncursesw -lm
endif

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(SRCS) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
