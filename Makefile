# Detect the Operating System
ifeq ($(OS),Windows_NT)
    # Windows Settings (MinGW / Code::Blocks environment)
    CC = gcc
    CFLAGS = -Iinclude -IC:/mingw64/include
    LDFLAGS = -Linclude -LC:/mingw64/lib -lmingw32 -lSDL2main -lSDL2 -lm
    DEV_TARGET = bin/Debug/starfall.exe
    RELEASE_TARGET = bin/Release/starfall.exe
    RM = del /Q
    MKDIR_DEV = if not exist bin\Debug mkdir bin\Debug
    MKDIR_RELEASE = if not exist bin\Release mkdir bin\Release
else
    # Linux Settings (Standard Shell environment)
    CC = gcc
    CFLAGS = -Iinclude `sdl2-config --cflags`
    LDFLAGS = `sdl2-config --libs` -lm
    DEV_TARGET = bin/Debug/starfall
    RELEASE_TARGET = bin/Release/starfall
    RM = rm -f
    MKDIR_DEV = mkdir -p bin/Debug
    MKDIR_RELEASE = mkdir -p bin/Release
endif

# Source files every build (dev or release) compiles.
SRCS = main.c src/bullet.c src/player.c src/starfield.c src/audio.c src/music.c src/soundtrack.c src/enemy.c src/collision.c src/enemy_bullet.c src/explosion.c src/text.c src/asteroid.c src/powerup.c src/wave.c src/boss.c src/highscore.c src/screen_effects.c src/popup.c src/game_render.c

# The developer toolkit's own source is only ever compiled into a dev
# build - a release build never sees this file at all, not just a
# disabled #ifdef inside it. See include/dev_tools.h for the
# compile-time boundary this depends on (STARFALL_DEV_TOOLS).
DEV_SRCS = $(SRCS) src/dev_tools.c

DEV_CFLAGS = $(CFLAGS) -DSTARFALL_DEV_TOOLS

# Code::Blocks Target Catchers
# These alias rules map Code::Blocks explicit target commands to our main compilation recipes -
# Debug matches the .cbp's Debug target (dev build, toolkit included), Release matches its
# Release target (toolkit-free).
Debug: dev
Release: release

# Default build rule - the dev build, since that's what active
# development against this project needs day to day. Run `make
# release` explicitly to build the toolkit-free variant.
all: dev

dev: $(DEV_TARGET)

release: $(RELEASE_TARGET)

$(DEV_TARGET): $(DEV_SRCS)
	$(MKDIR_DEV)
	$(CC) $(DEV_SRCS) $(DEV_CFLAGS) $(LDFLAGS) -o $(DEV_TARGET)

$(RELEASE_TARGET): $(SRCS)
	$(MKDIR_RELEASE)
	$(CC) $(SRCS) $(CFLAGS) $(LDFLAGS) -o $(RELEASE_TARGET)

# Clean rule to clear old builds
clean:
	$(RM) $(DEV_TARGET)
	$(RM) $(RELEASE_TARGET)
