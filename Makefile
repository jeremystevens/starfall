# Detect the Operating System
ifeq ($(OS),Windows_NT)
    # Windows Settings (MinGW / Code::Blocks environment)
    CC = gcc
    CFLAGS = -Iinclude -IC:/mingw64/include
    LDFLAGS = -Linclude -LC:/mingw64/lib -lmingw32 -lSDL2main -lSDL2 -lm
    TARGET = bin/Debug/starfall.exe
    RM = del /Q
    MKDIR = if not exist bin\Debug mkdir bin\Debug
else
    # Linux Settings (Standard Shell environment)
    CC = gcc
    CFLAGS = -Iinclude `sdl2-config --cflags`
    LDFLAGS = `sdl2-config --libs` -lm
    TARGET = bin/Debug/starfall
    RM = rm -f
    MKDIR = mkdir -p bin/Debug
endif

# List all your source code C files here
SRCS = main.c src/bullet.c src/player.c src/starfield.c src/audio.c src/music.c src/soundtrack.c src/enemy.c src/collision.c src/enemy_bullet.c src/explosion.c src/text.c src/asteroid.c src/powerup.c src/wave.c src/boss.c src/highscore.c src/screen_effects.c src/popup.c src/game_render.c

# Code::Blocks Target Catchers
# These alias rules map Code::Blocks explicit target commands to our main compilation recipe
Debug: all
Release: all

# Default build rule
all: $(TARGET)

$(TARGET): $(SRCS)
	$(MKDIR)
	$(CC) $(SRCS) $(CFLAGS) $(LDFLAGS) -o $(TARGET)

# Clean rule to clear old builds
clean:
	$(RM) $(TARGET)
