#ifndef TEXT_H
#define TEXT_H

#include <SDL.h>

// Draw a single character using our built-in bitmap font.
void text_draw_char(
    SDL_Renderer *renderer,
    char character,
    int x,
    int y,
    int scale
);

// Draw a complete string using our built-in bitmap font.
void text_draw(
    SDL_Renderer *renderer,
    const char *text,
    int x,
    int y,
    int scale
);

// Pixel width a string would occupy if drawn with text_draw() at the
// given scale - lets callers center messages without hardcoding a
// position for every possible string.
int text_width(const char *text, int scale);

#endif
