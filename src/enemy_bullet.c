#include "enemy_bullet.h"

#include "game_config.h"

// How quickly a dropped bomb speeds up as it falls. Applied to dy
// every frame so bombs start slow and get faster, just like gravity.
#define BOMB_GRAVITY 0.15f

// Initialize the enemy bullet pool. All bullets start inactive.
void enemy_bullets_init(EnemyBullet bullets[])
{
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
    {
        bullets[i].active = 0;
    }
}

// Fire a laser bolt from a Scout's position.
// Returns 1 if a bullet was fired, 0 if the pool was full.
int enemy_bullets_fire(
    EnemyBullet bullets[],
    float x,
    float y
)
{
    // Find an unused bullet slot.
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
    {
        if (!bullets[i].active)
        {
            bullets[i].x = x;
            bullets[i].y = y;

            // Scout bolts travel in a straight line toward the left.
            bullets[i].dx = -2.0f;
            bullets[i].dy = 0.0f;

            bullets[i].type = ENEMY_BULLET_BOLT;
            bullets[i].active = 1;

            return 1;
        }
    }

    // No free bullet slot was available.
    return 0;
}

// Drop a bomb from a Bomber's position.
// Returns 1 if the bomb was created, 0 if the pool was full.
int enemy_bullets_fire_bomb(
    EnemyBullet bullets[],
    float x,
    float y
)
{
    // Find an unused bullet slot, same pool the Scout bolts use.
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
    {
        if (!bullets[i].active)
        {
            bullets[i].x = x;
            bullets[i].y = y;

            // The bomb keeps a little of the Bomber's leftward drift
            // and starts falling slowly - BOMB_GRAVITY speeds it up
            // over time in enemy_bullets_update().
            bullets[i].dx = -0.5f;
            bullets[i].dy = 0.2f;

            bullets[i].type = ENEMY_BULLET_BOMB;
            bullets[i].active = 1;

            return 1;
        }
    }

    // No free bullet slot was available.
    return 0;
}

// Fire a bolt with a custom trajectory instead of a fixed one.
// Returns 1 if the bullet was fired, 0 if the pool was full.
int enemy_bullets_fire_angled(
    EnemyBullet bullets[],
    float x,
    float y,
    float dx,
    float dy
)
{
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
    {
        if (!bullets[i].active)
        {
            bullets[i].x = x;
            bullets[i].y = y;

            bullets[i].dx = dx;
            bullets[i].dy = dy;

            bullets[i].type = ENEMY_BULLET_BOLT;
            bullets[i].active = 1;

            return 1;
        }
    }

    return 0;
}

// Move active bullets and recycle any that have left the screen.
// Bolts fly off the left edge; bombs fall off the bottom.
void enemy_bullets_update(EnemyBullet bullets[])
{
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
    {
        if (!bullets[i].active)
        {
            continue;
        }

        if (bullets[i].type == ENEMY_BULLET_BOMB)
        {
            // Let the bomb keep accelerating downward as it falls.
            bullets[i].dy += BOMB_GRAVITY;
        }

        // Move the bullet.
        bullets[i].x += bullets[i].dx;
        bullets[i].y += bullets[i].dy;

        if (bullets[i].type == ENEMY_BULLET_BOMB)
        {
            // A bomb that reaches the bottom of the screen missed -
            // recycle it back into the pool.
            if (bullets[i].y > SCREEN_HEIGHT)
            {
                bullets[i].active = 0;
            }
        }
        else
        {
            // A bolt that reaches the left edge missed - recycle it too.
            if (bullets[i].x < 0)
            {
                bullets[i].active = 0;
            }
        }
    }
}

// Draw all active enemy bullets. Bolts and bombs get distinct looks so
// the player can tell at a glance which one they need to dodge.
void enemy_bullets_render(
    SDL_Renderer *renderer,
    const EnemyBullet bullets[]
)
{
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
    {
        if (!bullets[i].active)
        {
            continue;
        }

        if (bullets[i].type == ENEMY_BULLET_BOMB)
        {
            // Bombs draw as a small dark blob with a glowing orange
            // core, reading as a heavier, chunkier threat than a bolt.
            SDL_SetRenderDrawColor(renderer, 90, 45, 20, 255);

            SDL_Rect bomb_rect =
            {
                (int)bullets[i].x - 1,
                (int)bullets[i].y - 1,
                3,
                3
            };

            SDL_RenderFillRect(renderer, &bomb_rect);

            SDL_SetRenderDrawColor(renderer, 255, 150, 60, 255);
            SDL_RenderDrawPoint(renderer, (int)bullets[i].x, (int)bullets[i].y);
        }
        else
        {
            // Bright red-orange laser bolt, same look Scouts have always used.
            SDL_SetRenderDrawColor(renderer, 255, 80, 40, 255);

            SDL_Rect bullet_rect =
            {
                (int)bullets[i].x,
                (int)bullets[i].y,
                2,
                1
            };

            SDL_RenderFillRect(renderer, &bullet_rect);
        }
    }
}
