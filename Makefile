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
    # MSYS2/MinGW on Windows
    LIBS    = -lpdcurses
    TARGET  = pong.exe
else ifeq ($(UNAME), Darwin)
    # macOS: use system ncurses (ships with Xcode CLT)
    LIBS    = -lncurses -lm
else
    # Linux: ncurses + math library
    LIBS    = -lncurses -lm
endif

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(SRCS) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
