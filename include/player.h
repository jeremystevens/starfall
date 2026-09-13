#ifndef PLAYER_H
#define PLAYER_H

#include <SDL.h>

#include "powerup.h"

// The player's ship and its current state.
typedef struct
{
    float x;
    float y;

    int width;
    int height;

    float speed;
    int lives;

    // Damage / invulnerability state. While invulnerable is set,
    // the player ignores further hits and blinks on screen until
    // invulnerable_until is reached.
    int invulnerable;
    Uint32 invulnerable_until;

    // Developer-only invulnerability (v0.9.0 Developer Toolkit) -
    // deliberately a completely separate flag from invulnerable above
    // and from Shield (PowerUpState.shield_active), so toggling this
    // for testing can never start/consume/corrupt either of those.
    // Unlike invulnerable, this has no timer and no blink - it stays
    // set until explicitly toggled off again.
    //
    // Compiled out entirely in a toolkit-free release build (v0.9.0
    // Phase 8 release compile-out audit) - a Phase 5/6 version of this
    // field left it here unconditionally, reachable only in theory
    // (nothing outside the developer panel's own dispatch, itself
    // unreachable there, ever set it); Phase 8 upgraded that to
    // physical exclusion instead. This is a deliberate, narrow
    // exception to "normal gameplay modules don't check
    // STARFALL_DEV_TOOLS" (see dev_tools.h's top comment): the macro
    // is a plain build-flag check, not a #include of dev_tools.h
    // itself, so player.h still has no type/API dependency on the
    // toolkit - and there's no way to "concentrate" state that has to
    // live ON Player without either this, or teaching dev_tools.c to
    // reach into gameplay structs directly (exactly what that
    // module's own design rules out).
#ifdef STARFALL_DEV_TOOLS
    int dev_invulnerable;
#endif

    // The score total that triggers the next extra life. Advances
    // (see player_check_extra_life()) every time it's reached, so
    // this is always "how far away is the next one", not a fixed
    // list of thresholds.
    int next_extra_life_score;

    // Set by player_check_extra_life() whenever it awards at least one
    // life, so player_render_extra_life_notification() can show a
    // brief "EXTRA LIFE" readout. count is normally 1, but can be
    // higher if a single score jump crossed more than one threshold at
    // once (see player_check_extra_life()'s while loop). 0 means "not
    // currently showing".
    Uint32 extra_life_notification_until;
    int extra_life_notification_count;

} Player;

// Reset the player to its starting position, lives, and state.
void player_init(Player *player);

// Move the player based on held keys and clamp it inside the play
// area. min_y is the top boundary to clamp against - normally
// HUD_HEIGHT, but the caller can pass something lower (e.g. below a
// boss health bar overlay) without player.c needing to know why.
void player_update(
    Player *player,
    const Uint8 *keyboard,
    Uint32 current_time,
    int min_y
);

// Apply a hit to the player, removing one life and starting a brief
// invulnerability window - unless an active Shield absorbs the hit
// instead, in which case the shield is consumed and no life is lost.
// Centralizing the shield check here means every hazard (enemy
// bullets, ramming, asteroids) gets the same protection without each
// collision handler duplicating the check.
// Returns 1 if a life was actually lost, or 0 if the hit was ignored
// (e.g. the player was already invulnerable) or absorbed by Shield.
int player_take_damage(
    Player *player,
    PowerUpState *powerup_state,
    Uint32 current_time
);

// Award an extra life for every score threshold reached since the
// last call. The first threshold is close (EXTRA_LIFE_FIRST_THRESHOLD
// in player.c) so an average run earns one quickly; every one after
// that costs more (EXTRA_LIFE_INTERVAL), so lives don't pile up as
// the game gets harder. Call this once per frame with the current
// total score - a while loop internally means a single frame that
// crosses more than one threshold at once still awards every life it
// should. Also arms the extra_life_notification_* fields above when at
// least one life is awarded, purely as a side effect for rendering -
// awarding lives is still the only thing this function actually does.
// Returns the number of extra lives awarded this call (usually 0 or 1).
int player_check_extra_life(Player *player, int score, Uint32 current_time);

// Draw the player's ship (skipped every other blink while invulnerable).
void player_render(SDL_Renderer *renderer, const Player *player);

// Draw a brief "EXTRA LIFE" readout while
// extra_life_notification_until hasn't elapsed yet. A no-op otherwise,
// so it's safe to call unconditionally every frame. Purely
// presentational - never awards a life itself; only
// player_check_extra_life() does that.
void player_render_extra_life_notification(
    SDL_Renderer *renderer,
    const Player *player
);
#endif
