#ifndef ENEMY_BULLET_H
#define ENEMY_BULLET_H

#include <SDL.h>

#define MAX_ENEMY_BULLETS 30

// The two kinds of projectile an enemy can put on screen. They share
// one pool, but move, collide, and render a little differently.
typedef enum
{
    ENEMY_BULLET_BOLT, // Scout's laser bolt - flies straight left.
    ENEMY_BULLET_BOMB  // Bomber's bomb - drops straight down under gravity.

} EnemyBulletType;

// A single enemy projectile - either a Scout's laser bolt or a
// Bomber's bomb, depending on "type".
typedef struct
{
    float x;
    float y;

    float dx;
    float dy;

    EnemyBulletType type;

    int active;

} EnemyBullet;


// Initialize the enemy bullet pool.
void enemy_bullets_init(EnemyBullet bullets[]);

// Fire a Scout's laser bolt from an enemy position. Travels straight
// toward the left edge of the screen.
int enemy_bullets_fire(
    EnemyBullet bullets[],
    float x,
    float y
);

// Drop a Bomber's bomb from an enemy position. Starts nearly
// stationary and picks up downward speed as it falls.
int enemy_bullets_fire_bomb(
    EnemyBullet bullets[],
    float x,
    float y
);

// Fire a bolt with a custom trajectory instead of the fixed
// straight-left path enemy_bullets_fire() always uses - needed for
// the boss's spread attack. Shares the same pool and BOLT type (so it
// still recycles and renders exactly like a normal bolt), just lets
// the caller choose dx/dy directly instead of it being hardcoded.
int enemy_bullets_fire_angled(
    EnemyBullet bullets[],
    float x,
    float y,
    float dx,
    float dy
);

// Update all active enemy bullets.
void enemy_bullets_update(EnemyBullet bullets[]);


// Render all active enemy bullets.
void enemy_bullets_render(
    SDL_Renderer *renderer,
    const EnemyBullet bullets[]
);

#endif
