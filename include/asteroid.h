#ifndef ASTEROID_H
#define ASTEROID_H

#include <SDL.h>

#define MAX_ASTEROIDS 12

// Hard floor on asteroid spawn timing - same idea as enemy.h's
// MIN_SCOUT_SPAWN_DELAY/MIN_BOMBER_SPAWN_DELAY.
#define MIN_ASTEROID_SPAWN_DELAY 600

// Large asteroids are the normal environmental hazard. Small ones are
// the fragments left behind when a large asteroid is shot apart, and
// don't split any further themselves.
typedef enum
{
    ASTEROID_LARGE,
    ASTEROID_SMALL

} AsteroidType;

// A single asteroid, whether a full-size rock or a fragment.
typedef struct
{
    float x;
    float y;

    float dx;
    float dy;

    int width;
    int height;

    int health;
    int active;

    AsteroidType type;

    // Set by collision.c whenever a bullet damages this asteroid
    // without destroying it, so asteroids_render() can briefly flash
    // it white. 0 (the default) means "not currently flashing". Only
    // meaningful for large asteroids in practice, since a small
    // fragment's single hit point always destroys it outright.
    Uint32 hit_flash_until;

} Asteroid;

// Initialize the asteroid pool. All asteroids start inactive.
void asteroids_init(Asteroid asteroids[]);

// Spawn a new large asteroid from the right edge once its own spawn
// timer has expired. Vertical position, speed, and drift vary per
// spawn so asteroids don't all follow the same path. The caller (the
// Wave Director, via main.c) supplies the delay and decides whether
// to call this at all for the current wave.
void asteroids_spawn(
    Asteroid asteroids[],
    Uint32 current_time,
    Uint32 *last_asteroid_spawn,
    Uint32 spawn_delay
);

// Move active asteroids and recycle any that leave the screen.
void asteroids_update(Asteroid asteroids[]);

// Draw all active asteroids.
void asteroids_render(
    SDL_Renderer *renderer,
    const Asteroid asteroids[]
);

// Split a destroyed large asteroid into up to two small fragments,
// using free slots in the same pool. If fewer than two slots are
// free, spawns as many fragments as the pool can accommodate.
void asteroids_split(
    Asteroid asteroids[],
    const Asteroid *parent
);

#endif
