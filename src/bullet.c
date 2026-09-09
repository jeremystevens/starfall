#include "bullet.h"

// Initialize the bullet pool.
// All bullets begin inactive and are available for use.
void bullets_init(Bullet bullets[])
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        bullets[i].active = 0;
    }
}


// Fire a bullet if the weapon cooldown has expired.
int bullets_fire(
    Bullet bullets[],
    const Player *player,
    Uint32 current_time,
    Uint32 *last_shot_time,
    Uint32 fire_cooldown
)
{
    // Don't fire if the cooldown has not finished yet.
    if (current_time - *last_shot_time < fire_cooldown)
    {
        return 0;
    }

    // Find an unused bullet.
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!bullets[i].active)
        {
            bullets[i].x = player->x + player->width;
            bullets[i].y = player->y + (player->height / 2);

            bullets[i].dx = 4.0f;
            bullets[i].dy = 0.0f;
            bullets[i].active = 1;

            *last_shot_time = current_time;

            // Tell the caller that a bullet was actually fired.
            return 1;
        }
    }

    // Bullet pool was full.
    return 0;
}


// Vertical speed of the upper/lower spread projectiles. The middle
// one still flies dead straight, same as a normal shot.
#define SPREAD_SHOT_DY 1.0f

// Fire three bullets in a spread pattern if the weapon cooldown has
// expired, reusing the same pool and spawn point as bullets_fire().
int bullets_fire_spread(
    Bullet bullets[],
    const Player *player,
    Uint32 current_time,
    Uint32 *last_shot_time,
    Uint32 fire_cooldown
)
{
    // Don't fire if the cooldown has not finished yet.
    if (current_time - *last_shot_time < fire_cooldown)
    {
        return 0;
    }

    float spawn_x = player->x + player->width;
    float spawn_y = player->y + (player->height / 2);

    // Straight, angled up, angled down.
    const float spread_dy[3] = { 0.0f, -SPREAD_SHOT_DY, SPREAD_SHOT_DY };

    int fired_any = 0;

    for (int s = 0; s < 3; s++)
    {
        // Find a free slot for this projectile. If the pool doesn't
        // have one, this projectile is simply skipped - the burst
        // never writes outside the array.
        for (int i = 0; i < MAX_BULLETS; i++)
        {
            if (!bullets[i].active)
            {
                bullets[i].x = spawn_x;
                bullets[i].y = spawn_y;

                bullets[i].dx = 4.0f;
                bullets[i].dy = spread_dy[s];
                bullets[i].active = 1;

                fired_any = 1;

                break;
            }
        }
    }

    // Only reset the cooldown timer if at least one projectile from
    // the burst actually made it into the pool.
    if (fired_any)
    {
        *last_shot_time = current_time;
    }

    return fired_any;
}


// Update all active bullets.
void bullets_update(Bullet bullets[])
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (bullets[i].active)
        {
            bullets[i].x += bullets[i].dx;
            bullets[i].y += bullets[i].dy;

            // Recycle bullets that leave the screen. Spread Shot's
            // angled bullets can drift off the top/bottom edge, not
            // just the right, so every side needs checking now.
            if (bullets[i].x >= 160 ||
                    bullets[i].y < 0 ||
                    bullets[i].y > 120)
            {
                bullets[i].active = 0;
            }
        }
    }
}


// Render all active bullets.
void bullets_render(
    SDL_Renderer *renderer,
    const Bullet bullets[]
)
{
    // Yellow projectile color.
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);

    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (bullets[i].active)
        {
            SDL_Rect bullet_rect =
            {
                (int)bullets[i].x,
                (int)bullets[i].y,
                3,
                1
            };

            SDL_RenderFillRect(renderer, &bullet_rect);
        }
    }
}
