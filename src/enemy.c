#include "enemy.h"

#include "game_config.h"

#include <math.h>
#include <stdlib.h>

// Named here (rather than left as inline literals) because both the
// per-type spawn helpers below and the spawn-Y range calculation in
// enemies_spawn_scout()/enemies_spawn_bomber() need the same values.
#define SCOUT_HEIGHT 8
#define BOMBER_HEIGHT 12

// How far a Bomber drifts above and below its resting height while
// it bobs across the screen.
#define BOMBER_WOBBLE_AMPLITUDE 8.0f

// How fast the bob cycles - bigger steps mean a quicker bounce.
#define BOMBER_WOBBLE_SPEED 0.05f

// How much random variance to apply around the Wave Director's target
// spawn delay, so Scouts/Bombers don't arrive like clockwork.
#define SCOUT_SPAWN_VARIANCE_MS 200
#define BOMBER_SPAWN_VARIANCE_MS 400

// Rolls a small random jitter around spawn_delay, clamped so the
// actual delay it produces can never drop below min_delay. Returns an
// offset to add to the current timestamp when recording "last spawn
// time" - rolling this once per spawn (not every frame while waiting)
// is what keeps the result an even spread around the target instead
// of biasing toward the shortest possible delay.
static int jittered_spawn_offset(
    Uint32 spawn_delay,
    int variance_ms,
    Uint32 min_delay
)
{
    int jitter = (rand() % (2 * variance_ms + 1)) - variance_ms;
    int actual_delay = (int)spawn_delay + jitter;

    if (actual_delay < (int)min_delay)
    {
        actual_delay = (int)min_delay;
    }

    return actual_delay - (int)spawn_delay;
}

// Minimum vertical gap enforced between a newly spawned enemy and any
// enemy already on screen. Scouts in particular always travel at the
// exact same dx, so without this two that spawn close together never
// diverge and just ride together looking like one stacked blob.
#define MIN_ENEMY_SPAWN_GAP 14.0f

// How many times to re-roll a spawn Y before giving up and accepting
// whatever the last roll was. Five tries is enough to almost always
// find a clear spot without the pool ever getting crowded enough to
// loop for long.
#define SPAWN_Y_MAX_ATTEMPTS 5

// Picks a Y position in [min_y, max_y] for a newly spawned enemy,
// retrying if the roll lands too close to an already-active enemy.
static float pick_spawn_y(
    const Enemy enemies[],
    float min_y,
    float max_y
)
{
    float y = min_y;

    for (int attempt = 0; attempt < SPAWN_Y_MAX_ATTEMPTS; attempt++)
    {
        y = min_y + (float)(rand() % (int)(max_y - min_y + 1.0f));

        int too_close = 0;

        for (int i = 0; i < MAX_ENEMIES; i++)
        {
            if (!enemies[i].active)
            {
                continue;
            }

            float gap = enemies[i].y - y;

            if (gap < 0)
            {
                gap = -gap;
            }

            if (gap < MIN_ENEMY_SPAWN_GAP)
            {
                too_close = 1;
                break;
            }
        }

        if (!too_close)
        {
            break;
        }
    }

    return y;
}


// Initialize the enemy pool. All enemies start inactive.
void enemies_init(Enemy enemies[])
{
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        enemies[i].active = 0;
    }
}


// Fill in every field a freshly spawned Scout needs. y is chosen by
// the caller (via pick_spawn_y()) so it can check against other
// active enemies first.
static void enemy_spawn_scout(Enemy *enemy, Uint32 fire_delay, float y)
{
    enemy->type = ENEMY_TYPE_SCOUT;

    enemy->width = 12;
    enemy->height = SCOUT_HEIGHT;

    enemy->x = 159.0f;
    enemy->y = y;

    // Scouts just fly straight across, so the wobble fields are unused.
    enemy->wobble_phase = 0.0f;
    enemy->wobble_center_y = enemy->y;

    enemy->dx = -1.0f;
    enemy->dy = 0.0f;

    enemy->health = 1;
    enemy->fire_delay = fire_delay;
    enemy->hit_flash_until = 0;
}

// Fill in every field a freshly spawned Bomber needs. wobble_center is
// chosen by the caller (via pick_spawn_y()) for the same reason.
static void enemy_spawn_bomber(Enemy *enemy, Uint32 fire_delay, float wobble_center)
{
    enemy->type = ENEMY_TYPE_BOMBER;

    // Bombers are noticeably bigger than Scouts - they should look
    // like the heavier, tougher threat that they are.
    enemy->width = 16;
    enemy->height = BOMBER_HEIGHT;

    enemy->x = 159.0f;

    enemy->wobble_center_y = wobble_center;
    enemy->wobble_phase = 0.0f;
    enemy->y = enemy->wobble_center_y;

    // Horizontal creep only - the up/down motion comes entirely from
    // the wobble in enemies_update(), not from dy.
    enemy->dx = -0.4f;
    enemy->dy = 0.0f;

    enemy->health = 3;
    enemy->fire_delay = fire_delay;
    enemy->hit_flash_until = 0;
}

// Spawn a new Scout at the right edge of the screen once its own
// spawn timer has expired. The Wave Director supplies spawn_delay and
// decides whether to call this at all (Scouts can be disabled for a
// given wave) - this function itself doesn't know waves exist.
void enemies_spawn_scout(
    Enemy enemies[],
    Uint32 current_time,
    Uint32 *last_scout_spawn,
    Uint32 spawn_delay,
    Uint32 fire_delay
)
{
    if (current_time - *last_scout_spawn < spawn_delay)
    {
        return;
    }

    // Find an unused enemy slot.
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (!enemies[i].active)
        {
            float y = pick_spawn_y(
                          enemies,
                          (float)HUD_HEIGHT,
                          (float)(SCREEN_HEIGHT - SCOUT_HEIGHT)
                      );

            enemy_spawn_scout(&enemies[i], fire_delay, y);

            enemies[i].active = 1;
            enemies[i].last_shot_time = current_time;

            *last_scout_spawn = current_time + jittered_spawn_offset(
                                     spawn_delay,
                                     SCOUT_SPAWN_VARIANCE_MS,
                                     MIN_SCOUT_SPAWN_DELAY
                                 );

            break;
        }
    }
}

// Spawn a new Bomber at the right edge of the screen once its own
// spawn timer has expired. Same shape as enemies_spawn_scout(), on an
// independent timer so Scout and Bomber spawn rates can be tuned
// separately by wave.
void enemies_spawn_bomber(
    Enemy enemies[],
    Uint32 current_time,
    Uint32 *last_bomber_spawn,
    Uint32 spawn_delay,
    Uint32 fire_delay
)
{
    if (current_time - *last_bomber_spawn < spawn_delay)
    {
        return;
    }

    // Find an unused enemy slot.
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (!enemies[i].active)
        {
            // Same safe wobble-center range enemy_spawn_bomber() used
            // to compute internally, now shared with pick_spawn_y().
            float min_center = (float)HUD_HEIGHT + BOMBER_WOBBLE_AMPLITUDE;
            float max_center = (float)(SCREEN_HEIGHT - BOMBER_HEIGHT) - BOMBER_WOBBLE_AMPLITUDE;

            float wobble_center = pick_spawn_y(enemies, min_center, max_center);

            enemy_spawn_bomber(&enemies[i], fire_delay, wobble_center);

            enemies[i].active = 1;
            enemies[i].last_shot_time = current_time;

            *last_bomber_spawn = current_time + jittered_spawn_offset(
                                      spawn_delay,
                                      BOMBER_SPAWN_VARIANCE_MS,
                                      MIN_BOMBER_SPAWN_DELAY
                                  );

            break;
        }
    }
}


// Move active enemies and recycle any that leave the screen.
void enemies_update(Enemy enemies[])
{
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (enemies[i].active)
        {
            enemies[i].x += enemies[i].dx;

            if (enemies[i].type == ENEMY_TYPE_BOMBER)
            {
                // Bob up and down around the resting height instead
                // of flying in a straight line like a Scout does.
                enemies[i].wobble_phase += BOMBER_WOBBLE_SPEED;

                enemies[i].y =
                    enemies[i].wobble_center_y +
                    sinf(enemies[i].wobble_phase) * BOMBER_WOBBLE_AMPLITUDE;
            }
            else
            {
                enemies[i].y += enemies[i].dy;
            }

            // Recycle enemies once completely off-screen.
            if (enemies[i].x + enemies[i].width < 0)
            {
                enemies[i].active = 0;
            }
        }
    }
}

// Let each enemy fire if its own cooldown has expired. Scouts fire a
// laser bolt from their nose; Bombers drop a bomb from their belly.
int enemies_fire(
    Enemy enemies[],
    EnemyBullet bullets[],
    Uint32 current_time
)
{
    int fired_mask = 0;

    for (int i = 0; i < MAX_ENEMIES; i++)
    {

        // Ignore unused enemy slots.
        if (!enemies[i].active)
        {
            continue;
        }

        // Has this enemy waited long enough to fire again?
        if (current_time - enemies[i].last_shot_time < enemies[i].fire_delay)
        {
            continue;
        }

        int shot_fired = 0;

        if (enemies[i].type == ENEMY_TYPE_BOMBER)
        {
            // Bombers don't aim at the player - they just release a
            // bomb straight down from underneath as they fly over.
            float bomb_x = enemies[i].x + (enemies[i].width / 2.0f);
            float bomb_y = enemies[i].y + enemies[i].height;

            if (enemy_bullets_fire_bomb(bullets, bomb_x, bomb_y))
            {
                shot_fired = 1;
                fired_mask |= ENEMY_FIRED_BOMB;
            }
        }
        else
        {
            float bullet_x = enemies[i].x;
            float bullet_y =
                enemies[i].y + (enemies[i].height / 2.0f);

            if (enemy_bullets_fire(bullets, bullet_x, bullet_y))
            {
                shot_fired = 1;
                fired_mask |= ENEMY_FIRED_BOLT;
            }
        }

        // Only reset the timer if a bullet was actually created.
        if (shot_fired)
        {
            enemies[i].last_shot_time = current_time;
        }
    }

    return fired_mask;
}

// Sets the draw color to flash-white while a hit flash is active, or
// the given color otherwise - same convention boss.c's
// set_boss_draw_color() already uses for its own flash.
static void set_enemy_draw_color(
    SDL_Renderer *renderer,
    int flashing,
    Uint8 r,
    Uint8 g,
    Uint8 b
)
{
    if (flashing)
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    }
}

// Draw a single Bomber as a bulky, rounded hull with an orange bomb
// bay slung underneath - it should read as heavier and slower than
// a Scout at a glance, before the player even sees it move. Flashes
// white briefly when hit without being destroyed (see hit_flash_until
// in enemy.h) - a Bomber survives multiple hits, so this is the only
// feedback a non-lethal shot gets. Scouts don't get this treatment:
// with only 1 HP, every hit destroys one outright, so there's never a
// "survived a hit" moment to flash for.
static void enemy_render_bomber(SDL_Renderer *renderer, int x, int y, int flashing)
{
    // Dark, armored green hull.
    set_enemy_draw_color(renderer, flashing, 70, 110, 90);

    SDL_RenderDrawLine(renderer, x + 5, y,      x + 10, y);
    SDL_RenderDrawLine(renderer, x + 3, y + 1,  x + 12, y + 1);
    SDL_RenderDrawLine(renderer, x + 2, y + 2,  x + 13, y + 2);
    SDL_RenderDrawLine(renderer, x + 1, y + 3,  x + 14, y + 3);
    SDL_RenderDrawLine(renderer, x,     y + 4,  x + 15, y + 4);
    SDL_RenderDrawLine(renderer, x,     y + 5,  x + 15, y + 5);
    SDL_RenderDrawLine(renderer, x,     y + 6,  x + 15, y + 6);
    SDL_RenderDrawLine(renderer, x,     y + 7,  x + 15, y + 7);
    SDL_RenderDrawLine(renderer, x + 1, y + 8,  x + 14, y + 8);
    SDL_RenderDrawLine(renderer, x + 2, y + 9,  x + 13, y + 9);
    SDL_RenderDrawLine(renderer, x + 3, y + 10, x + 12, y + 10);
    SDL_RenderDrawLine(renderer, x + 5, y + 11, x + 10, y + 11);

    // Bright orange bomb bay - a visual hint that this is the enemy
    // that drops bombs instead of firing lasers.
    set_enemy_draw_color(renderer, flashing, 255, 150, 60);

    SDL_Rect bomb_bay = { x + 5, y + 5, 6, 3 };
    SDL_RenderFillRect(renderer, &bomb_bay);

    // Small pale cockpit highlight up top.
    set_enemy_draw_color(renderer, flashing, 200, 255, 220);
    SDL_RenderDrawPoint(renderer, x + 7, y + 2);
    SDL_RenderDrawPoint(renderer, x + 8, y + 2);
}

// Draw a single Scout as a small red ship with a yellow core.
static void enemy_render_scout(SDL_Renderer *renderer, int x, int y)
{
    // Red Scout body.
    SDL_SetRenderDrawColor(renderer, 255, 80, 80, 255);

    SDL_RenderDrawLine(renderer, x + 2,  y,     x + 7,  y);
    SDL_RenderDrawLine(renderer, x + 1,  y + 1, x + 9,  y + 1);
    SDL_RenderDrawLine(renderer, x,      y + 2, x + 11, y + 2);

    SDL_RenderDrawLine(renderer, x + 2,  y + 3, x + 10, y + 3);
    SDL_RenderDrawLine(renderer, x + 2,  y + 4, x + 10, y + 4);

    SDL_RenderDrawLine(renderer, x,      y + 5, x + 11, y + 5);
    SDL_RenderDrawLine(renderer, x + 1,  y + 6, x + 9,  y + 6);
    SDL_RenderDrawLine(renderer, x + 2,  y + 7, x + 7,  y + 7);


    // Yellow/orange center detail.
    SDL_SetRenderDrawColor(renderer, 255, 200, 50, 255);

    SDL_Rect center =
    {
        x + 2,
        y + 3,
        3,
        2
    };

    SDL_RenderFillRect(renderer, &center);
}

// Draw every active enemy, picking the right look for its type.
void enemies_render(
    SDL_Renderer *renderer,
    const Enemy enemies[]
)
{
    Uint32 current_time = SDL_GetTicks();

    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (!enemies[i].active)
        {
            continue;
        }

        int x = (int)enemies[i].x;
        int y = (int)enemies[i].y;

        if (enemies[i].type == ENEMY_TYPE_BOMBER)
        {
            int flashing = current_time < enemies[i].hit_flash_until;
            enemy_render_bomber(renderer, x, y, flashing);
        }
        else
        {
            enemy_render_scout(renderer, x, y);
        }
    }
}

