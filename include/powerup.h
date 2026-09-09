#ifndef POWERUP_H
#define POWERUP_H

#include <SDL.h>

#define MAX_POWERUPS 8

// The kinds of power-up pickup that can appear on screen. More may be
// added later, but every one of them shares this same pool and the
// generic x/y/active fields below - only the type-specific behavior
// (collection effect, rendering, HUD display) branches on this.
typedef enum
{
    POWERUP_RAPID_FIRE,
    POWERUP_SHIELD,
    POWERUP_SPREAD_SHOT

} PowerUpType;

// A single power-up pickup drifting across the screen, waiting to be
// collected by the player.
typedef struct
{
    float x;
    float y;

    // Pickups only ever drift left, so a single dx is enough - unlike
    // enemies/asteroids there's no need for vertical drift.
    float dx;

    int width;
    int height;

    PowerUpType type;
    int active;

} PowerUp;

// How long a timed effect lasts once collected. Separate constants
// per type, even though they currently match, so durations can be
// tuned independently later.
#define RAPID_FIRE_DURATION 10000
#define SPREAD_SHOT_DURATION 10000

// Firing cooldown while Rapid Fire is active, replacing the normal
// FIRE_COOLDOWN from bullet.h for as long as the effect lasts.
#define RAPID_FIRE_COOLDOWN 70

// Tracks which power-up effects are currently active for the player.
// A single shared struct instead of scattering timer variables
// through main.c - Rapid Fire and Spread Shot are timed, Shield is a
// simple charge (either the player has one or doesn't).
typedef struct
{
    int rapid_fire_active;
    Uint32 rapid_fire_until;

    int spread_shot_active;
    Uint32 spread_shot_until;

    int shield_active;

} PowerUpState;

// Reset all power-up effects to inactive.
void powerup_state_init(PowerUpState *state);

// Activate the effect for a collected power-up type. Collecting a
// timed effect that's already active refreshes it back to full
// duration rather than stacking. Collecting Shield while already
// active simply leaves the player with one charge.
void powerup_state_collect(
    PowerUpState *state,
    PowerUpType type,
    Uint32 current_time
);

// Expire any timed effects whose duration has elapsed.
void powerup_state_update(
    PowerUpState *state,
    Uint32 current_time
);

// Short HUD-friendly label for a power-up type ("RAPID", "SPREAD",
// "SHIELD"). Centralized here so the pickup icon and HUD bar can't
// drift out of sync with each other.
const char *powerup_type_name(PowerUpType type);

// Color and pickup-icon letter for a power-up type. Both the pickup
// icon and the HUD countdown bar pull from these so they can't drift
// out of sync with each other.
void powerup_color(PowerUpType type, Uint8 *r, Uint8 *g, Uint8 *b);
char powerup_letter(PowerUpType type);

// How much of a timed effect's duration remains, in milliseconds.
// Returns 0 once "until" has already passed rather than underflowing
// (Uint32 subtraction wraps around instead of going negative).
Uint32 powerup_time_remaining(Uint32 until, Uint32 current_time);

// Position of each power-up's compact HUD indicator. They share the
// same row as the score/lives text (y=2) rather than growing the HUD
// strip - there's already dead horizontal space between "SCORE ####"
// and "LIVES #" at 160px wide, and reusing it keeps the playfield
// exactly as large as it was before power-ups existed.
#define POWERUP_HUD_Y 2
#define POWERUP_HUD_RAPID_X 62
#define POWERUP_HUD_SPREAD_X 86

// Shield is charge-based, not timed, so it gets just its letter (no
// bar) in the one remaining sliver of space before the lives counter.
#define POWERUP_HUD_SHIELD_X 107

// Draws one compact HUD indicator: the power-up's pickup letter
// followed by a small bordered bar that shrinks from full to empty as
// remaining_time runs out. Blinks the whole indicator once remaining
// time drops below ~25% of total_duration, as a simple expiration
// warning.
void powerup_hud_bar_render(
    SDL_Renderer *renderer,
    int x,
    int y,
    char letter,
    Uint8 r,
    Uint8 g,
    Uint8 b,
    Uint32 remaining_time,
    Uint32 total_duration,
    Uint32 current_time
);

// Draws a simple retro energy-field outline around the player's
// bounding box while Shield is active. Takes plain position/size
// rather than a Player pointer so this header doesn't need to depend
// on player.h (which already depends on this one for PowerUpState).
void powerup_render_shield(
    SDL_Renderer *renderer,
    float player_x,
    float player_y,
    int player_width,
    int player_height
);

// Initialize the power-up pool. All pickups start inactive.
void powerups_init(PowerUp powerups[]);

// Spawn a pickup of the given type at (x, y) using a free pool slot.
// If the pool is full, the drop is silently skipped.
void powerups_spawn(
    PowerUp powerups[],
    float x,
    float y,
    PowerUpType type
);

// Move active pickups and recycle any that drift off the left edge.
void powerups_update(PowerUp powerups[]);

// Draw all active pickups.
void powerups_render(
    SDL_Renderer *renderer,
    const PowerUp powerups[]
);

#endif
