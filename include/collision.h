#ifndef COLLISION_H
#define COLLISION_H

#include <SDL.h>

#include "bullet.h"
#include "enemy.h"
#include "enemy_bullet.h"
#include "asteroid.h"
#include "powerup.h"
#include "boss.h"
#include "explosion.h"
#include "player.h"
#include "popup.h"


// Returns 1 if two rectangles overlap, otherwise 0.
int check_collision(
    const SDL_Rect *a,
    const SDL_Rect *b
);


// Handle collisions between player bullets and enemies.
// Spawns a Scout destruction effect for each enemy destroyed, and may
// occasionally drop a power-up pickup at the destroyed enemy's
// position. An enemy that survives a hit (a Bomber below full health)
// gets a brief hit_flash_until instead. Each kill also spawns a
// floating "+value" popup at its position, purely to visualize the
// points this function already returns - it never awards score itself.
// Returns the number of score points earned.
int collisions_bullets_enemies(
    Bullet bullets[],
    Enemy enemies[],
    ExplosionParticle explosions[],
    PowerUp powerups[],
    ScorePopup score_popups[],
    Uint32 current_time
);

// Handle collisions between the player and enemies.
// Spawns destruction effects for the player (if hit) and for any
// Scout destroyed by ramming, which may also drop a power-up pickup.
// Returns the number of enemies destroyed this call.
// *player_hit is set to 1 if the player actually took damage.
int collisions_player_enemies(
    Player *player,
    Enemy enemies[],
    ExplosionParticle explosions[],
    PowerUp powerups[],
    PowerUpState *powerup_state,
    Uint32 current_time,
    int *player_hit
);

// Handle collisions between enemy bullets and the player.
// Spawns a player destruction effect when the player is hit.
// Returns 1 if the player was hit this call, otherwise 0.
int collisions_player_enemy_bullets(
    Player *player,
    EnemyBullet bullets[],
    ExplosionParticle explosions[],
    PowerUpState *powerup_state,
    Uint32 current_time
);

// Handle collisions between player bullets and asteroids.
// Large asteroids split into two small fragments when destroyed this
// way; small fragments do not split further. Spawns the appropriate
// destruction effect for whichever asteroid was destroyed. A large
// asteroid that survives a hit gets a brief hit_flash_until instead.
// Each kill also spawns a floating "+value" popup at its position,
// purely to visualize the points this function already returns.
// Returns the number of score points earned.
int collisions_bullets_asteroids(
    Bullet bullets[],
    Asteroid asteroids[],
    ExplosionParticle explosions[],
    ScorePopup score_popups[],
    Uint32 current_time
);

// Handle collisions between the player and asteroids.
// Spawns destruction effects for the player (if hit) and for any
// asteroid destroyed by the collision. Asteroids destroyed this way
// do not split and do not award score.
// Returns the number of asteroids destroyed this call.
// *player_hit is set to 1 if the player actually took damage.
int collisions_player_asteroids(
    Player *player,
    Asteroid asteroids[],
    ExplosionParticle explosions[],
    PowerUpState *powerup_state,
    Uint32 current_time,
    int *player_hit
);

// Handle collisions between the player and power-up pickups.
// Collecting a pickup deactivates it, activates (or refreshes) its
// effect in the given PowerUpState, and spawns a small colored burst
// around the player (reusing the shared explosion pool, in the
// power-up's own color from powerup_color()) as collection feedback -
// purely visual, so this never affects damage or health.
// Returns the number of pickups collected this call.
int collisions_player_powerups(
    Player *player,
    PowerUp powerups[],
    PowerUpState *powerup_state,
    ExplosionParticle explosions[],
    Uint32 current_time
);

// Handle collisions between player bullets and the boss. Only applies
// while the boss is in active combat (not during its entrance, and
// not once it's dying) - the boss itself is not a valid bullet target
// outside BOSS_STATE_ACTIVE. Works identically for a normal shot,
// Rapid Fire, or each individual Spread Shot pellet, since they're
// all just Bullets from the same pool. Spawns a small hit-spark
// effect for every successful hit, and briefly sets hit_flash_until
// so boss_render() flashes the hull white.
// Returns the number of hits landed this call.
int collisions_bullets_boss(
    Bullet bullets[],
    Boss *boss,
    ExplosionParticle explosions[],
    Uint32 current_time
);

// Handle the player colliding with the boss. Damages the player only
// (via the common player_take_damage() path, so Shield and
// invulnerability both apply) - never the boss itself, unlike ramming
// a normal enemy or asteroid. Only bullets can bring the boss's
// health down, so ramming can never be used to defeat it.
// Returns 1 if the player was hit this call, otherwise 0.
int collisions_player_boss(
    Player *player,
    Boss *boss,
    ExplosionParticle explosions[],
    PowerUpState *powerup_state,
    Uint32 current_time
);
#endif
