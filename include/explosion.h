#ifndef EXPLOSION_H
#define EXPLOSION_H

#include <SDL.h>

#define MAX_EXPLOSION_PARTICLES 200

// A single retro debris/spark particle belonging to an explosion burst.
typedef struct
{
    float x;
    float y;

    float dx;
    float dy;

    int life;
    int max_life;

    Uint8 r;
    Uint8 g;
    Uint8 b;

    int active;

} ExplosionParticle;


// Initialize the explosion particle pool.
void explosions_init(ExplosionParticle particles[]);

// Spawn a burst of "count" particles centered on (x, y) using the given color.
void explosions_spawn(
    ExplosionParticle particles[],
    float x,
    float y,
    Uint8 r,
    Uint8 g,
    Uint8 b,
    int count
);

// Update all active explosion particles.
void explosions_update(ExplosionParticle particles[]);

// Render all active explosion particles.
void explosions_render(
    SDL_Renderer *renderer,
    const ExplosionParticle particles[]
);

#endif
