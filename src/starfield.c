#include "starfield.h"

#include <stdlib.h>

// Scatter every star to a random position with a random scroll speed.
void starfield_init(Star stars[])
{
    for (int i = 0; i < MAX_STARS; i++)
    {
        stars[i].x = rand() % 160;
        stars[i].y = rand() % 120;

        // Faster stars read as closer to the camera (cheap parallax).
        int speed_level = (rand() % 3) + 1;
        stars[i].speed = speed_level * 0.5f;
    }
}

// Scroll each star left and recycle it once it scrolls off-screen.
void starfield_update(Star stars[])
{
    for (int i = 0; i < MAX_STARS; i++)
    {
        stars[i].x -= stars[i].speed;

        if (stars[i].x < 0)
        {
            stars[i].x = 159;
            stars[i].y = rand() % 120;
            stars[i].speed = ((rand() % 3) + 1) * 0.5f;
        }
    }
}

// Draw every star as a single white point.
void starfield_render(SDL_Renderer *renderer, const Star stars[])
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    for (int i = 0; i < MAX_STARS; i++)
    {
        SDL_RenderDrawPoint(
            renderer,
            (int)stars[i].x,
            (int)stars[i].y
        );
    }
}
