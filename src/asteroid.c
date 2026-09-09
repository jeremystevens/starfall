#include "asteroid.h"

#include "game_config.h"

#include <stdlib.h>

// Large asteroids are the normal hazard that spawns naturally.
#define ASTEROID_LARGE_WIDTH 14
#define ASTEROID_LARGE_HEIGHT 14
#define ASTEROID_LARGE_HEALTH 3

// How fast large asteroids drift left, and how much that varies from
// one spawn to the next so they don't all move in lockstep.
#define ASTEROID_MIN_SPEED 0.3f
#define ASTEROID_MAX_SPEED 0.7f

// Maximum vertical drift in either direction, so some asteroids cut
// diagonally across the screen instead of flying dead level.
#define ASTEROID_MAX_DRIFT 0.15f

// How much random variance to apply around the Wave Director's target
// spawn delay, so asteroids don't arrive on a predictable clock.
#define ASTEROID_SPAWN_VARIANCE_MS 300

// Rolls a small random jitter around spawn_delay, clamped so the
// actual delay it produces can never drop below min_delay. See
// enemy.c's identical helper for why this is rolled once per spawn
// rather than re-rolled every frame while waiting.
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

// Minimum vertical gap enforced between a newly spawned large asteroid
// and any asteroid already on screen - without this, two that spawn
// close together can drift at similar enough speed to look like one
// stacked blob instead of two separate rocks.
#define MIN_ASTEROID_SPAWN_GAP 16.0f

// How many times to re-roll a spawn Y before accepting the last roll.
#define SPAWN_Y_MAX_ATTEMPTS 5

// Picks a Y position in [min_y, max_y] for a newly spawned asteroid,
// retrying if the roll lands too close to an already-active one. Only
// used for naturally-spawned large asteroids - fragments intentionally
// spawn at their parent's position, not a random one.
static float pick_spawn_y(
    const Asteroid asteroids[],
    float min_y,
    float max_y
)
{
    float y = min_y;

    for (int attempt = 0; attempt < SPAWN_Y_MAX_ATTEMPTS; attempt++)
    {
        y = min_y + (float)(rand() % (int)(max_y - min_y + 1.0f));

        int too_close = 0;

        for (int i = 0; i < MAX_ASTEROIDS; i++)
        {
            if (!asteroids[i].active)
            {
                continue;
            }

            float gap = asteroids[i].y - y;

            if (gap < 0)
            {
                gap = -gap;
            }

            if (gap < MIN_ASTEROID_SPAWN_GAP)
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

// Small fragments left behind when a large asteroid is shot apart.
// They don't split any further.
#define ASTEROID_SMALL_WIDTH 7
#define ASTEROID_SMALL_HEIGHT 7
#define ASTEROID_SMALL_HEALTH 1

// How many fragments a destroyed large asteroid produces, and how
// much extra vertical speed each one picks up so they visibly spread
// apart instead of overlapping.
#define ASTEROID_FRAGMENT_COUNT 2
#define ASTEROID_FRAGMENT_DRIFT 0.3f


// Initialize the asteroid pool. All asteroids start inactive.
void asteroids_init(Asteroid asteroids[])
{
    for (int i = 0; i < MAX_ASTEROIDS; i++)
    {
        asteroids[i].active = 0;
    }
}


// Fill in every field a freshly spawned large asteroid needs. y is
// chosen by the caller (via pick_spawn_y()) so it can check against
// other active asteroids first.
static void asteroid_spawn_large(Asteroid *asteroid, float y)
{
    asteroid->type = ASTEROID_LARGE;

    asteroid->width = ASTEROID_LARGE_WIDTH;
    asteroid->height = ASTEROID_LARGE_HEIGHT;

    asteroid->x = (float)SCREEN_WIDTH - 1.0f;
    asteroid->y = y;

    // Randomize speed so asteroids don't all travel at the same rate.
    float speed =
        ASTEROID_MIN_SPEED +
        ((float)(rand() % 100) / 100.0f) * (ASTEROID_MAX_SPEED - ASTEROID_MIN_SPEED);

    asteroid->dx = -speed;

    // Small vertical drift in either direction for diagonal movement.
    asteroid->dy =
        -ASTEROID_MAX_DRIFT +
        ((float)(rand() % 100) / 100.0f) * (2.0f * ASTEROID_MAX_DRIFT);

    asteroid->health = ASTEROID_LARGE_HEALTH;
    asteroid->hit_flash_until = 0;
}


// Spawn a new large asteroid at the right edge of the screen once its
// own spawn timer has expired.
void asteroids_spawn(
    Asteroid asteroids[],
    Uint32 current_time,
    Uint32 *last_asteroid_spawn,
    Uint32 spawn_delay
)
{
    // Don't spawn until enough time has passed.
    if (current_time - *last_asteroid_spawn < spawn_delay)
    {
        return;
    }

    // Find an unused asteroid slot.
    for (int i = 0; i < MAX_ASTEROIDS; i++)
    {
        if (!asteroids[i].active)
        {
            float y = pick_spawn_y(
                          asteroids,
                          (float)HUD_HEIGHT,
                          (float)(SCREEN_HEIGHT - ASTEROID_LARGE_HEIGHT)
                      );

            asteroid_spawn_large(&asteroids[i], y);

            asteroids[i].active = 1;

            *last_asteroid_spawn = current_time + jittered_spawn_offset(
                                       spawn_delay,
                                       ASTEROID_SPAWN_VARIANCE_MS,
                                       MIN_ASTEROID_SPAWN_DELAY
                                   );

            break;
        }
    }
}


// Move active asteroids and recycle any that leave the screen.
void asteroids_update(Asteroid asteroids[])
{
    for (int i = 0; i < MAX_ASTEROIDS; i++)
    {
        if (!asteroids[i].active)
        {
            continue;
        }

        asteroids[i].x += asteroids[i].dx;
        asteroids[i].y += asteroids[i].dy;

        // Keep asteroids out of the HUD strip, same rule the player
        // follows. Bounce the drift back downward instead of just
        // pinning y at the boundary, so a clamped asteroid still
        // reads as drifting rather than sliding sideways along the
        // HUD divider.
        if (asteroids[i].y < HUD_HEIGHT)
        {
            asteroids[i].y = HUD_HEIGHT;
            asteroids[i].dy = -asteroids[i].dy;
        }

        // Recycle once fully off any edge of the screen. The small
        // vertical drift means an asteroid could in principle leave
        // through the top or bottom instead of the left, so check
        // every side rather than just x.
        if (asteroids[i].x + asteroids[i].width < 0 ||
                asteroids[i].x > SCREEN_WIDTH ||
                asteroids[i].y + asteroids[i].height < 0 ||
                asteroids[i].y > SCREEN_HEIGHT)
        {
            asteroids[i].active = 0;
        }
    }
}


// Fill in every field a freshly spawned small fragment needs. Inherits
// the parent's horizontal drive (with a little randomness) and adds
// dy_offset on top of the parent's own vertical drift so fragments
// visibly spread apart from one another.
static void asteroid_spawn_small(
    Asteroid *asteroid,
    float center_x,
    float center_y,
    float parent_dx,
    float parent_dy,
    float dy_offset
)
{
    asteroid->type = ASTEROID_SMALL;

    asteroid->width = ASTEROID_SMALL_WIDTH;
    asteroid->height = ASTEROID_SMALL_HEIGHT;

    asteroid->x = center_x - asteroid->width / 2.0f;
    asteroid->y = center_y - asteroid->height / 2.0f;

    // Vary the inherited speed a little so fragments don't move in
    // perfect lockstep with each other.
    asteroid->dx = parent_dx * (0.8f + ((float)(rand() % 40) / 100.0f));
    asteroid->dy = parent_dy + dy_offset;

    asteroid->health = ASTEROID_SMALL_HEALTH;
    asteroid->hit_flash_until = 0;
}

// Split a destroyed large asteroid into up to two small fragments,
// reusing free slots from the same pool. Never writes outside the
// pool - if fewer than two slots are free, spawns as many fragments
// as it can.
void asteroids_split(
    Asteroid asteroids[],
    const Asteroid *parent
)
{
    float center_x = parent->x + parent->width / 2.0f;
    float center_y = parent->y + parent->height / 2.0f;

    int fragments_spawned = 0;

    for (int i = 0; i < MAX_ASTEROIDS && fragments_spawned < ASTEROID_FRAGMENT_COUNT; i++)
    {
        if (asteroids[i].active)
        {
            continue;
        }

        // First fragment drifts upward, second drifts downward, each
        // with a little randomness so they don't mirror perfectly.
        float dy_offset;

        if (fragments_spawned == 0)
        {
            dy_offset = -ASTEROID_FRAGMENT_DRIFT -
                        ((float)(rand() % 50) / 100.0f) * ASTEROID_FRAGMENT_DRIFT;
        }
        else
        {
            dy_offset = ASTEROID_FRAGMENT_DRIFT +
                        ((float)(rand() % 50) / 100.0f) * ASTEROID_FRAGMENT_DRIFT;
        }

        asteroid_spawn_small(
            &asteroids[i],
            center_x,
            center_y,
            parent->dx,
            parent->dy,
            dy_offset
        );

        asteroids[i].active = 1;

        fragments_spawned++;
    }
}

// Sets the draw color to flash-white while a hit flash is active, or
// the given color otherwise - same convention boss.c's
// set_boss_draw_color() and enemy.c's set_enemy_draw_color() already
// use for their own flashes.
static void set_asteroid_draw_color(
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

// Draw a large asteroid as a chunky, irregular rock - rows of varying
// width give it a lumpy silhouette instead of a perfect circle.
// Flashes white briefly when hit without being destroyed (see
// hit_flash_until in asteroid.h) - a large asteroid survives multiple
// hits, so this is the only feedback a non-lethal shot gets.
static void asteroid_render_large(SDL_Renderer *renderer, int x, int y, int flashing)
{
    // Rocky gray-brown body.
    set_asteroid_draw_color(renderer, flashing, 130, 110, 90);

    SDL_RenderDrawLine(renderer, x + 4,  y,      x + 7,  y);
    SDL_RenderDrawLine(renderer, x + 2,  y + 1,  x + 9,  y + 1);
    SDL_RenderDrawLine(renderer, x + 1,  y + 2,  x + 11, y + 2);
    SDL_RenderDrawLine(renderer, x,      y + 3,  x + 12, y + 3);
    SDL_RenderDrawLine(renderer, x,      y + 4,  x + 13, y + 4);
    SDL_RenderDrawLine(renderer, x,      y + 5,  x + 13, y + 5);
    SDL_RenderDrawLine(renderer, x + 1,  y + 6,  x + 13, y + 6);
    SDL_RenderDrawLine(renderer, x,      y + 7,  x + 12, y + 7);
    SDL_RenderDrawLine(renderer, x + 1,  y + 8,  x + 13, y + 8);
    SDL_RenderDrawLine(renderer, x + 1,  y + 9,  x + 11, y + 9);
    SDL_RenderDrawLine(renderer, x + 2,  y + 10, x + 10, y + 10);
    SDL_RenderDrawLine(renderer, x + 3,  y + 11, x + 9,  y + 11);
    SDL_RenderDrawLine(renderer, x + 4,  y + 12, x + 8,  y + 12);
    SDL_RenderDrawLine(renderer, x + 5,  y + 13, x + 7,  y + 13);

    // A couple of darker crater shadows for surface texture - skipped
    // entirely while flashing so the burst reads as one clean flash of
    // white rather than a white rock with dark speckles still showing.
    if (!flashing)
    {
        SDL_SetRenderDrawColor(renderer, 80, 65, 50, 255);
        SDL_RenderDrawPoint(renderer, x + 4, y + 4);
        SDL_RenderDrawPoint(renderer, x + 9, y + 5);
        SDL_RenderDrawPoint(renderer, x + 6, y + 9);
    }

    // A pale highlight to suggest a light source and add depth - also
    // already white, so only worth drawing separately when not flashing.
    if (!flashing)
    {
        SDL_SetRenderDrawColor(renderer, 170, 150, 120, 255);
        SDL_RenderDrawPoint(renderer, x + 3, y + 2);
        SDL_RenderDrawPoint(renderer, x + 10, y + 8);
    }
}

// Draw a small asteroid fragment - the same rocky look as a large
// asteroid, just scaled down to a handful of rows.
static void asteroid_render_small(SDL_Renderer *renderer, int x, int y)
{
    SDL_SetRenderDrawColor(renderer, 130, 110, 90, 255);

    SDL_RenderDrawLine(renderer, x + 2, y,     x + 4, y);
    SDL_RenderDrawLine(renderer, x + 1, y + 1, x + 5, y + 1);
    SDL_RenderDrawLine(renderer, x,     y + 2, x + 6, y + 2);
    SDL_RenderDrawLine(renderer, x,     y + 3, x + 6, y + 3);
    SDL_RenderDrawLine(renderer, x + 1, y + 4, x + 6, y + 4);
    SDL_RenderDrawLine(renderer, x + 1, y + 5, x + 5, y + 5);
    SDL_RenderDrawLine(renderer, x + 2, y + 6, x + 4, y + 6);

    SDL_SetRenderDrawColor(renderer, 80, 65, 50, 255);
    SDL_RenderDrawPoint(renderer, x + 4, y + 2);
}

// Draw every active asteroid, picking the right look for its size.
void asteroids_render(
    SDL_Renderer *renderer,
    const Asteroid asteroids[]
)
{
    Uint32 current_time = SDL_GetTicks();

    for (int i = 0; i < MAX_ASTEROIDS; i++)
    {
        if (!asteroids[i].active)
        {
            continue;
        }

        int x = (int)asteroids[i].x;
        int y = (int)asteroids[i].y;

        if (asteroids[i].type == ASTEROID_LARGE)
        {
            int flashing = current_time < asteroids[i].hit_flash_until;
            asteroid_render_large(renderer, x, y, flashing);
        }
        else
        {
            asteroid_render_small(renderer, x, y);
        }
    }
}
