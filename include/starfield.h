#ifndef STARFIELD_H
#define STARFIELD_H

#include <SDL.h>

#define MAX_STARS 30

// A single star in the scrolling parallax background.
typedef struct
{
    float x;
    float y;
    float speed;
} Star;

// Scatter every star to a random position with a random scroll speed.
void starfield_init(Star stars[]);

// Scroll each star left and wrap it back to the right edge once it
// scrolls off-screen.
void starfield_update(Star stars[]);

// Draw every star as a single point.
void starfield_render(SDL_Renderer *renderer, const Star stars[]);

#endif
