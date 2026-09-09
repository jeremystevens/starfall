#ifndef ENEMY_H
#define ENEMY_H

#include <SDL.h>
#include "enemy_bullet.h"

#define MAX_ENEMIES 20

// Hard floor on Scout/Bomber spawn timing. No matter how the Wave
// Director tunes spawn_delay, or how much random jitter gets applied
// around it, the actual interval between spawns can never drop below
// these values.
#define MIN_SCOUT_SPAWN_DELAY 300
#define MIN_BOMBER_SPAWN_DELAY 1000

// Hard floor on Scout/Bomber firing frequency - keeps the Wave
// Director's firing-pressure scaling from ever producing continuous
// projectile spam at high wave numbers.
#define MIN_SCOUT_FIRE_DELAY 400
#define MIN_BOMBER_FIRE_DELAY 800

// Every enemy on screen is one of these archetypes. They all share the
// same pool and struct below, but branch out into different movement,
// weapons, and rendering depending on which type they are.
typedef enum
{
    ENEMY_TYPE_SCOUT,  // Small, fast, fires straight at the player.
    ENEMY_TYPE_BOMBER  // Bigger, slower, tankier, drops bombs instead.

} EnemyType;

// Bits returned by enemies_fire() so the caller knows which weapon
// sound(s) to play this frame. More than one can be set at once if a
// Scout and a Bomber both happen to fire on the same update.
#define ENEMY_FIRED_BOLT 0x1
#define ENEMY_FIRED_BOMB 0x2

// A single enemy, whether it's a Scout or a Bomber.
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

    EnemyType type;

    // Bombers don't fly in a straight line - they drift up and down as
    // they cross the screen. These two fields drive that bobbing motion.
    // Scouts leave them alone entirely and just use dy instead.
    float wobble_phase;
    float wobble_center_y;

    Uint32 last_shot_time;
    Uint32 fire_delay;

    // Set by collision.c whenever a bullet damages this enemy without
    // destroying it, so enemies_render() can briefly flash it white -
    // visible confirmation of a hit for anything tougher than a
    // one-hit Scout. 0 (the default) means "not currently flashing".
    Uint32 hit_flash_until;

} Enemy;

// Initialize the enemy pool. All enemies start inactive.
void enemies_init(Enemy enemies[]);


// Spawn a new Scout once its own spawn timer has expired. The caller
// (the Wave Director, via main.c) supplies spawn_delay and decides
// whether to call this at all for the current wave. fire_delay is
// baked into the new Scout at spawn time and stays fixed for that
// enemy's lifetime, even if the wave (and its fire delay) changes
// later - the same way an asteroid's speed never changes after spawn.
void enemies_spawn_scout(
    Enemy enemies[],
    Uint32 current_time,
    Uint32 *last_scout_spawn,
    Uint32 spawn_delay,
    Uint32 fire_delay
);

// Spawn a new Bomber once its own spawn timer has expired. Same idea
// as enemies_spawn_scout(), on an independent timer.
void enemies_spawn_bomber(
    Enemy enemies[],
    Uint32 current_time,
    Uint32 *last_bomber_spawn,
    Uint32 spawn_delay,
    Uint32 fire_delay
);


// Let each enemy fire if its own cooldown has expired. Scouts fire a
// laser bolt straight ahead; Bombers drop a bomb from underneath them.
// Returns a combination of the ENEMY_FIRED_* flags above, or 0 if
// nothing fired this call.
int enemies_fire(
    Enemy enemies[],
    EnemyBullet bullets[],
    Uint32 current_time
);

// Move active enemies and recycle any that leave the screen.
void enemies_update(Enemy enemies[]);


// Draw all active enemies.
void enemies_render(
    SDL_Renderer *renderer,
    const Enemy enemies[]
);

#endif
