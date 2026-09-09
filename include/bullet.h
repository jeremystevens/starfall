#ifndef BULLET_H
#define BULLET_H

#include <SDL.h>
#include "player.h"

#define MAX_BULLETS 50
#define FIRE_COOLDOWN 150 // Minimum milliseconds between player shots (normal rate).

// A single player projectile.
typedef struct
{
    float x;
    float y;
    float dx;
    float dy;
    int active;
} Bullet;

// Initialize the bullet pool. All bullets start inactive.
void bullets_init(Bullet bullets[]);

// Fire a bullet from the player's position if the given cooldown
// allows it. The caller decides which cooldown to pass (e.g. the
// normal FIRE_COOLDOWN, or a shorter one while Rapid Fire is active),
// so this function stays unaware of power-ups entirely.
// Returns 1 if a bullet was fired, 0 otherwise.
int bullets_fire(
    Bullet bullets[],
    const Player *player,
    Uint32 current_time,
    Uint32 *last_shot_time,
    Uint32 fire_cooldown
);

// Fire three bullets in a spread pattern (straight, angled up, angled
// down) from the player's position if the given cooldown allows it -
// same idea as bullets_fire(), used while Spread Shot is active.
// Uses the same bullet pool and cooldown gate; if fewer than three
// slots are free, fires as many of the three as the pool can
// accommodate rather than skipping the whole burst.
// Returns 1 if at least one bullet was fired, 0 otherwise.
int bullets_fire_spread(
    Bullet bullets[],
    const Player *player,
    Uint32 current_time,
    Uint32 *last_shot_time,
    Uint32 fire_cooldown
);

// Move active bullets and recycle any that leave the screen.
void bullets_update(Bullet bullets[]);

// Draw all active bullets.
void bullets_render(
    SDL_Renderer *renderer,
    const Bullet bullets[]
);

#endif
