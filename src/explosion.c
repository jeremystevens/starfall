#include "explosion.h"

#include <math.h>
#include <stdlib.h>

// init explosion particle pool
void explosions_init(ExplosionParticle particles[])
{
    for (int i = 0; i < MAX_EXPLOSION_PARTICLES; i++)
    {
        particles[i].active = 0;
    }
}

// spawn a burst of particles from a destroyed ship
void explosions_spawn(
    ExplosionParticle particles[],
    float x,
    float y,
    Uint8 r,
    Uint8 g,
    Uint8 b,
    int count
)
{
    int spawned = 0;

    for (int i = 0; i < MAX_EXPLOSION_PARTICLES && spawned < count; i++)
    {
        if (particles[i].active)
        {
            continue;
        }

        // Throw the particle outward from the blast center in a random direction.
        float angle = (float)(rand() % 360) * (float)M_PI / 180.0f;
        float speed = 0.3f + ((float)(rand() % 100) / 100.0f) * 0.9f;

        particles[i].x = x;
        particles[i].y = y;

        particles[i].dx = cosf(angle) * speed;
        particles[i].dy = sinf(angle) * speed;

        int life = 12 + (rand() % 10);

        particles[i].life = life;
        particles[i].max_life = life;

        particles[i].r = r;
        particles[i].g = g;
        particles[i].b = b;

        particles[i].active = 1;

        spawned++;
    }
}

// update all active explosion particles
void explosions_update(ExplosionParticle particles[])
{
    for (int i = 0; i < MAX_EXPLOSION_PARTICLES; i++)
    {
        if (!particles[i].active)
        {
            continue;
        }

        particles[i].x += particles[i].dx;
        particles[i].y += particles[i].dy;

        // Let the burst settle quickly instead of drifting forever.
        particles[i].dx *= 0.9f;
        particles[i].dy *= 0.9f;

        particles[i].life--;

        if (particles[i].life <= 0)
        {
            particles[i].active = 0;
        }
    }
}

// render all active explosion particles
void explosions_render(
    SDL_Renderer *renderer,
    const ExplosionParticle particles[]
)
{
    for (int i = 0; i < MAX_EXPLOSION_PARTICLES; i++)
    {
        if (!particles[i].active)
        {
            continue;
        }

        // Fade from a bright flash, through the blast color, to a dark ember.
        float fraction =
            (float)particles[i].life / (float)particles[i].max_life;

        Uint8 r, g, b;

        if (fraction > 0.66f)
        {
            r = 255;
            g = 255;
            b = 220;
        }
        else if (fraction > 0.33f)
        {
            r = particles[i].r;
            g = particles[i].g;
            b = particles[i].b;
        }
        else
        {
            r = particles[i].r / 3;
            g = particles[i].g / 3;
            b = particles[i].b / 3;
        }

        SDL_SetRenderDrawColor(renderer, r, g, b, 255);

        SDL_RenderDrawPoint(
            renderer,
            (int)particles[i].x,
            (int)particles[i].y
        );
    }
}
