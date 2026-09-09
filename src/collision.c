#include "collision.h"

#include <stdlib.h>

// Distinct destruction-effect palettes so Scout fireballs read
// differently from the player's ship breaking apart.
#define ENEMY_EXPLOSION_PARTICLES 14
#define PLAYER_EXPLOSION_PARTICLES 18

// Bombers are bigger and tougher, so they go out with a bigger,
// darker fireball to match.
#define BOMBER_EXPLOSION_PARTICLES 26

// How many points each enemy type is worth when destroyed.
#define SCOUT_SCORE_VALUE 10
#define BOMBER_SCORE_VALUE 30

// Chance (1 in N) that a destroyed enemy drops a power-up. Bombers
// are tougher/rarer, so they're somewhat more generous than Scouts.
// Asteroids intentionally have no equivalent - they never drop
// power-ups.
#define SCOUT_POWERUP_DROP_CHANCE 20  // 1 in 20 = 5%
#define BOMBER_POWERUP_DROP_CHANCE 10 // 1 in 10 = 10%

// Asteroid destruction effects and scoring. A large asteroid is worth
// more on its own, but splits into two fragments worth more combined
// - rewarding the player for finishing off every piece.
#define ASTEROID_LARGE_EXPLOSION_PARTICLES 24
#define ASTEROID_SMALL_EXPLOSION_PARTICLES 10

#define ASTEROID_LARGE_SCORE_VALUE 50
#define ASTEROID_SMALL_SCORE_VALUE 20

// How long a damaged-but-not-destroyed target flashes white for. Kept
// short (tens of milliseconds) so it reads as an instant impact cue
// rather than a lingering animation - see hit_flash_until in
// enemy.h/asteroid.h/boss.h. The boss gets its own, shorter constant:
// Rapid Fire's cooldown is only 70ms, so a flash anywhere near that
// long would keep it looking permanently white during sustained fire
// instead of visibly flickering with each individual hit.
#define ENEMY_HIT_FLASH_DURATION_MS 60
#define ASTEROID_HIT_FLASH_DURATION_MS 60
#define BOSS_HIT_FLASH_DURATION_MS 45


// Spawns the right destruction effect for whichever enemy type was
// just destroyed, and hands back how many points it's worth.
// Shared by both the bullet-kill and ramming collision paths below
// so the two don't drift apart over time.
static int enemy_destroyed(
    const Enemy *enemy,
    ExplosionParticle explosions[],
    PowerUp powerups[]
)
{
    float center_x = enemy->x + enemy->width / 2.0f;
    float center_y = enemy->y + enemy->height / 2.0f;

    int score;
    int drop_chance;

    if (enemy->type == ENEMY_TYPE_BOMBER)
    {
        // Bomber destruction effect - bigger burst, deeper orange.
        explosions_spawn(
            explosions,
            center_x,
            center_y,
            255, 90, 30,
            BOMBER_EXPLOSION_PARTICLES
        );

        score = BOMBER_SCORE_VALUE;
        drop_chance = BOMBER_POWERUP_DROP_CHANCE;
    }
    else
    {
        // Scout destruction effect.
        explosions_spawn(
            explosions,
            center_x,
            center_y,
            255, 120, 40,
            ENEMY_EXPLOSION_PARTICLES
        );

        score = SCOUT_SCORE_VALUE;
        drop_chance = SCOUT_POWERUP_DROP_CHANCE;
    }

    // Occasionally drop a random power-up - a reward, not a
    // guarantee, so it stays a nice surprise rather than the norm.
    if (rand() % drop_chance == 0)
    {
        PowerUpType dropped_type = (PowerUpType)(rand() % 3);
        powerups_spawn(powerups, center_x, center_y, dropped_type);
    }

    return score;
}


// Spawns the right destruction effect for whichever asteroid size was
// just destroyed, and hands back how many points it's worth. Shared
// by both the bullet-kill and ramming collision paths below, same as
// enemy_destroyed() above.
static int asteroid_destroyed(
    const Asteroid *asteroid,
    ExplosionParticle explosions[]
)
{
    float center_x = asteroid->x + asteroid->width / 2.0f;
    float center_y = asteroid->y + asteroid->height / 2.0f;

    if (asteroid->type == ASTEROID_LARGE)
    {
        // Big rocky burst - gray-orange debris.
        explosions_spawn(
            explosions,
            center_x,
            center_y,
            210, 140, 80,
            ASTEROID_LARGE_EXPLOSION_PARTICLES
        );

        return ASTEROID_LARGE_SCORE_VALUE;
    }

    // Small fragment - a quicker, smaller puff of debris.
    explosions_spawn(
        explosions,
        center_x,
        center_y,
        180, 160, 140,
        ASTEROID_SMALL_EXPLOSION_PARTICLES
    );

    return ASTEROID_SMALL_SCORE_VALUE;
}


// Simple AABB overlap test used by every collision check below.
int check_collision(
    const SDL_Rect *a,
    const SDL_Rect *b
)
{
    return (
        a->x < b->x + b->w &&
        a->x + a->w > b->x &&
        a->y < b->y + b->h &&
        a->y + a->h > b->y
    );
}


// Checks every active bullet against every active enemy.
int collisions_bullets_enemies(
    Bullet bullets[],
    Enemy enemies[],
    ExplosionParticle explosions[],
    PowerUp powerups[],
    ScorePopup score_popups[],
    Uint32 current_time
)
{
    int points_earned = 0;

    for (int b = 0; b < MAX_BULLETS; b++)
    {
        if (!bullets[b].active)
        {
            continue;
        }

        SDL_Rect bullet_rect =
        {
            (int)bullets[b].x,
            (int)bullets[b].y,
            3,
            1
        };

        for (int e = 0; e < MAX_ENEMIES; e++)
        {
            if (!enemies[e].active)
            {
                continue;
            }

            SDL_Rect enemy_rect =
            {
                (int)enemies[e].x,
                (int)enemies[e].y,
                enemies[e].width,
                enemies[e].height
            };

            if (check_collision(&bullet_rect, &enemy_rect))
            {
                // Bullet is consumed by the collision.
                bullets[b].active = 0;

                // Damage the enemy.
                enemies[e].health--;

                if (enemies[e].health <= 0)
                {
                    // Captured before active is cleared, and before
                    // enemy_destroyed() runs - it doesn't move the
                    // enemy, but reads clearer this way regardless.
                    float center_x = enemies[e].x + enemies[e].width / 2.0f;
                    float center_y = enemies[e].y + enemies[e].height / 2.0f;

                    enemies[e].active = 0;

                    int enemy_value = enemy_destroyed(&enemies[e], explosions, powerups);
                    points_earned += enemy_value;

                    // Visualizes the value just added above - never
                    // awards it. enemy_value already came from the
                    // same SCOUT_SCORE_VALUE/BOMBER_SCORE_VALUE
                    // constants the HUD score itself is built from.
                    popups_spawn(
                        score_popups,
                        center_x,
                        center_y,
                        enemy_value,
                        current_time
                    );
                }
                else
                {
                    // Survived the hit - flash white briefly rather
                    // than vanishing, so a Bomber taking damage is
                    // just as visible as one being destroyed.
                    enemies[e].hit_flash_until =
                        current_time + ENEMY_HIT_FLASH_DURATION_MS;
                }

                // This bullet cannot hit another enemy.
                break;
            }
        }
    }

    return points_earned;
}

// Handles the player ramming into an enemy.
int collisions_player_enemies(
    Player *player,
    Enemy enemies[],
    ExplosionParticle explosions[],
    PowerUp powerups[],
    PowerUpState *powerup_state,
    Uint32 current_time,
    int *player_hit
)
{
    *player_hit = 0;

    int enemies_destroyed = 0;

    // Create a collision rectangle for the player.
    SDL_Rect player_rect =
    {
        (int)player->x,
        (int)player->y,
        player->width,
        player->height
    };

    // Check the player against every active enemy.
    for (int e = 0; e < MAX_ENEMIES; e++)
    {
        if (!enemies[e].active)
        {
            continue;
        }

        SDL_Rect enemy_rect =
        {
            (int)enemies[e].x,
            (int)enemies[e].y,
            enemies[e].width,
            enemies[e].height
        };

        if (check_collision(&player_rect, &enemy_rect))
        {
            // Damage the player.
            if (player_take_damage(player, powerup_state, current_time))
            {
                *player_hit = 1;

                // Player ship destruction effect.
                explosions_spawn(
                    explosions,
                    player->x + player->width / 2.0f,
                    player->y + player->height / 2.0f,
                    150, 190, 255,
                    PLAYER_EXPLOSION_PARTICLES
                );
            }

            // Enemy destruction effect - ramming destroys the enemy
            // outright regardless of remaining health, same as before.
            enemy_destroyed(&enemies[e], explosions, powerups);

            enemies[e].active = 0;
            enemies_destroyed++;
        }
    }

    return enemies_destroyed;
}

// Handles enemy bullets hitting the player.
int collisions_player_enemy_bullets(
    Player *player,
    EnemyBullet bullets[],
    ExplosionParticle explosions[],
    PowerUpState *powerup_state,
    Uint32 current_time
)
{
    int player_hit = 0;

    // Create a collision rectangle for the player.
    SDL_Rect player_rect =
    {
        (int)player->x,
        (int)player->y,
        player->width,
        player->height
    };

    // Check the player against every active enemy bullet.
    for (int b = 0; b < MAX_ENEMY_BULLETS; b++)
    {
        if (!bullets[b].active)
        {
            continue;
        }

        // Bombs are drawn bigger than laser bolts, so give them a
        // bigger hitbox to match - otherwise they'd feel like they
        // "should" have hit when they visually clip the player.
        SDL_Rect bullet_rect;

        if (bullets[b].type == ENEMY_BULLET_BOMB)
        {
            bullet_rect.x = (int)bullets[b].x - 1;
            bullet_rect.y = (int)bullets[b].y - 1;
            bullet_rect.w = 3;
            bullet_rect.h = 3;
        }
        else
        {
            bullet_rect.x = (int)bullets[b].x;
            bullet_rect.y = (int)bullets[b].y;
            bullet_rect.w = 2;
            bullet_rect.h = 1;
        }

        if (check_collision(&player_rect, &bullet_rect))
        {
            // Damage the player.
            if (player_take_damage(player, powerup_state, current_time))
            {
                player_hit = 1;

                // Player ship destruction effect.
                explosions_spawn(
                    explosions,
                    player->x + player->width / 2.0f,
                    player->y + player->height / 2.0f,
                    150, 190, 255,
                    PLAYER_EXPLOSION_PARTICLES
                );
            }

            // The bullet is consumed by the collision.
            bullets[b].active = 0;
        }
    }

    return player_hit;
}

// Checks every active bullet against every active asteroid.
int collisions_bullets_asteroids(
    Bullet bullets[],
    Asteroid asteroids[],
    ExplosionParticle explosions[],
    ScorePopup score_popups[],
    Uint32 current_time
)
{
    int points_earned = 0;

    for (int b = 0; b < MAX_BULLETS; b++)
    {
        if (!bullets[b].active)
        {
            continue;
        }

        SDL_Rect bullet_rect =
        {
            (int)bullets[b].x,
            (int)bullets[b].y,
            3,
            1
        };

        for (int a = 0; a < MAX_ASTEROIDS; a++)
        {
            if (!asteroids[a].active)
            {
                continue;
            }

            SDL_Rect asteroid_rect =
            {
                (int)asteroids[a].x,
                (int)asteroids[a].y,
                asteroids[a].width,
                asteroids[a].height
            };

            if (check_collision(&bullet_rect, &asteroid_rect))
            {
                // Bullet is consumed by the collision.
                bullets[b].active = 0;

                // Damage the asteroid.
                asteroids[a].health--;

                if (asteroids[a].health <= 0)
                {
                    float center_x = asteroids[a].x + asteroids[a].width / 2.0f;
                    float center_y = asteroids[a].y + asteroids[a].height / 2.0f;

                    int asteroid_value = asteroid_destroyed(&asteroids[a], explosions);
                    points_earned += asteroid_value;

                    // Visualizes the value just added above - never
                    // awards it. asteroid_value already came from
                    // ASTEROID_LARGE_SCORE_VALUE/ASTEROID_SMALL_SCORE_VALUE.
                    popups_spawn(
                        score_popups,
                        center_x,
                        center_y,
                        asteroid_value,
                        current_time
                    );

                    // Only large asteroids shot down by the player split
                    // into fragments - small ones are gone for good.
                    if (asteroids[a].type == ASTEROID_LARGE)
                    {
                        asteroids_split(asteroids, &asteroids[a]);
                    }

                    asteroids[a].active = 0;
                }
                else
                {
                    // Survived the hit - flash white briefly. Only
                    // large asteroids ever actually reach this branch,
                    // since a small fragment's single hit point always
                    // destroys it outright above.
                    asteroids[a].hit_flash_until =
                        current_time + ASTEROID_HIT_FLASH_DURATION_MS;
                }

                // This bullet cannot hit another asteroid.
                break;
            }
        }
    }

    return points_earned;
}

// Handles the player ramming into an asteroid.
int collisions_player_asteroids(
    Player *player,
    Asteroid asteroids[],
    ExplosionParticle explosions[],
    PowerUpState *powerup_state,
    Uint32 current_time,
    int *player_hit
)
{
    *player_hit = 0;

    int asteroids_destroyed = 0;

    // Create a collision rectangle for the player.
    SDL_Rect player_rect =
    {
        (int)player->x,
        (int)player->y,
        player->width,
        player->height
    };

    // Check the player against every active asteroid.
    for (int a = 0; a < MAX_ASTEROIDS; a++)
    {
        if (!asteroids[a].active)
        {
            continue;
        }

        SDL_Rect asteroid_rect =
        {
            (int)asteroids[a].x,
            (int)asteroids[a].y,
            asteroids[a].width,
            asteroids[a].height
        };

        if (check_collision(&player_rect, &asteroid_rect))
        {
            // Damage the player.
            if (player_take_damage(player, powerup_state, current_time))
            {
                *player_hit = 1;

                // Player ship destruction effect.
                explosions_spawn(
                    explosions,
                    player->x + player->width / 2.0f,
                    player->y + player->height / 2.0f,
                    150, 190, 255,
                    PLAYER_EXPLOSION_PARTICLES
                );
            }

            // Crashing destroys the asteroid outright, same as ramming
            // an enemy. No score and no splitting here - that only
            // happens when a large asteroid is shot down by the
            // player's own weapon, not when it hits the player.
            asteroid_destroyed(&asteroids[a], explosions);

            asteroids[a].active = 0;
            asteroids_destroyed++;
        }
    }

    return asteroids_destroyed;
}

// Handles the player picking up a power-up.
// Small colored burst around the player when a pickup is collected -
// noticeably smaller than any destruction effect, since nothing was
// destroyed here, just picked up.
#define POWERUP_COLLECT_PARTICLES 12

int collisions_player_powerups(
    Player *player,
    PowerUp powerups[],
    PowerUpState *powerup_state,
    ExplosionParticle explosions[],
    Uint32 current_time
)
{
    int collected = 0;

    // Create a collision rectangle for the player.
    SDL_Rect player_rect =
    {
        (int)player->x,
        (int)player->y,
        player->width,
        player->height
    };

    // Check the player against every active pickup.
    for (int p = 0; p < MAX_POWERUPS; p++)
    {
        if (!powerups[p].active)
        {
            continue;
        }

        SDL_Rect powerup_rect =
        {
            (int)powerups[p].x,
            (int)powerups[p].y,
            powerups[p].width,
            powerups[p].height
        };

        if (check_collision(&player_rect, &powerup_rect))
        {
            powerups[p].active = 0;

            powerup_state_collect(powerup_state, powerups[p].type, current_time);

            // Collection feedback: a small burst in the power-up's own
            // color, centered on the player rather than the pickup's
            // old position, so it reads as "the player just gained
            // something" rather than marking where the icon used to
            // be. Reuses powerup_color() - the same color already
            // shown on the HUD bar for this type - so the two can
            // never drift apart.
            Uint8 r, g, b;
            powerup_color(powerups[p].type, &r, &g, &b);

            explosions_spawn(
                explosions,
                player->x + player->width / 2.0f,
                player->y + player->height / 2.0f,
                r, g, b,
                POWERUP_COLLECT_PARTICLES
            );

            collected++;
        }
    }

    return collected;
}

// Small spark burst for a single boss hit - noticeably smaller than
// any full destruction effect, since it's just one bullet landing on
// a much bigger target, not a kill.
#define BOSS_HIT_PARTICLES 4

// Checks every active bullet against the boss.
int collisions_bullets_boss(
    Bullet bullets[],
    Boss *boss,
    ExplosionParticle explosions[],
    Uint32 current_time
)
{
    // The boss isn't a valid target during its entrance, and stops
    // being one once it's dying - phase transitions and the
    // destruction sequence itself are handled elsewhere once those
    // systems exist.
    if (boss->state != BOSS_STATE_ACTIVE)
    {
        return 0;
    }

    int hits = 0;

    SDL_Rect boss_rect =
    {
        (int)boss->x,
        (int)boss->y,
        boss->width,
        boss->height
    };

    for (int b = 0; b < MAX_BULLETS; b++)
    {
        if (!bullets[b].active)
        {
            continue;
        }

        SDL_Rect bullet_rect =
        {
            (int)bullets[b].x,
            (int)bullets[b].y,
            3,
            1
        };

        if (check_collision(&bullet_rect, &boss_rect))
        {
            bullets[b].active = 0;

            boss->health--;

            if (boss->health < 0)
            {
                boss->health = 0;
            }

            // Flash white briefly on every hit, lethal or not - a
            // killing blow immediately transitions the boss into its
            // destruction sequence anyway, so there's no real harm in
            // setting this unconditionally rather than checking health
            // first.
            boss->hit_flash_until = current_time + BOSS_HIT_FLASH_DURATION_MS;

            // Spark at the impact point rather than the boss's own
            // center, so it reads as hitting the hull where the
            // bullet actually landed.
            explosions_spawn(
                explosions,
                bullets[b].x,
                bullets[b].y,
                255, 200, 120,
                BOSS_HIT_PARTICLES
            );

            hits++;
        }
    }

    return hits;
}

// Handles the player colliding with the boss. Unlike ramming a normal
// enemy or asteroid (which always destroys the target outright, just
// without score), the boss is a persistent, health-gated encounter -
// colliding with its hull damages the player through the common
// damage path (Shield and invulnerability both apply automatically)
// but never damages the boss itself. Bullets are the only thing that
// can bring its health down (see collisions_bullets_boss()), so
// ramming can never be used to defeat it, invulnerable or not.
// Only applies while the boss is in active combat, matching
// collisions_bullets_boss().
// Returns 1 if the player was hit this call, otherwise 0.
int collisions_player_boss(
    Player *player,
    Boss *boss,
    ExplosionParticle explosions[],
    PowerUpState *powerup_state,
    Uint32 current_time
)
{
    if (boss->state != BOSS_STATE_ACTIVE)
    {
        return 0;
    }

    SDL_Rect player_rect =
    {
        (int)player->x,
        (int)player->y,
        player->width,
        player->height
    };

    SDL_Rect boss_rect =
    {
        (int)boss->x,
        (int)boss->y,
        boss->width,
        boss->height
    };

    if (!check_collision(&player_rect, &boss_rect))
    {
        return 0;
    }

    if (player_take_damage(player, powerup_state, current_time))
    {
        explosions_spawn(
            explosions,
            player->x + player->width / 2.0f,
            player->y + player->height / 2.0f,
            150, 190, 255,
            PLAYER_EXPLOSION_PARTICLES
        );

        return 1;
    }

    return 0;
}
