#ifndef SCREEN_EFFECTS_H
#define SCREEN_EFFECTS_H

#include <SDL.h>

// Render-only presentation effects layered on top of the simulation.
// Nothing in this module ever touches gameplay coordinates - its only
// inputs are timestamps and tunable constants, and its only output is
// a render offset. Collision, movement, and every other system remain
// completely unaware this exists; only main.c's render section reads
// it, right before drawing the frame.
typedef struct
{
    // Magnitude (in logical pixels) the current shake started at, and
    // the timestamp it ends at. A magnitude of 0, or current_time at
    // or past shake_until, both mean idle -
    // screen_effects_get_shake_offset() then always returns (0, 0).
    int shake_magnitude;
    Uint32 shake_duration_ms;
    Uint32 shake_until;

    // A brief, translucent full-screen color flash - color, peak
    // alpha, how long the current flash lasts in total, and when it
    // ends. current_time at or past flash_until means idle -
    // screen_effects_render_flash() then draws nothing.
    Uint8 flash_r, flash_g, flash_b, flash_alpha;
    Uint32 flash_duration_ms;
    Uint32 flash_until;

} ScreenEffects;

// Reset to idle - no shake in progress. Called once at startup and
// again by game_start_new() for every fresh run, so a shake still
// playing out at the exact moment a new game begins can never bleed
// into it.
void screen_effects_init(ScreenEffects *effects);

// Start (or replace) a shake of the given magnitude, in logical
// pixels, lasting duration_ms from current_time. A weaker shake
// arriving while a stronger one is still playing is ignored rather
// than cutting the stronger one short - so a small player-hit shake
// can never undercut, say, an already-playing Dreadnought-destruction
// shake.
void screen_effects_shake(
    ScreenEffects *effects,
    int magnitude,
    Uint32 duration_ms,
    Uint32 current_time
);

// This frame's shake offset. Rolls a fresh random jitter every call
// (so the shake visibly rattles rather than holding one static
// displacement), with its magnitude decaying linearly to 0 as the
// shake approaches shake_until. Returns (0, 0) once idle, so it is
// always safe to call and apply unconditionally every frame.
void screen_effects_get_shake_offset(
    const ScreenEffects *effects,
    Uint32 current_time,
    int *offset_x,
    int *offset_y
);

// Start (or replace) a full-screen flash in the given color, peaking
// at alpha and fading linearly to 0 over duration_ms. Always
// overrides whatever flash is already playing - there's currently
// only one trigger (the player's final death), so no "don't let a
// weaker flash cut off a stronger one" rule like screen_effects_shake()
// has is needed yet.
void screen_effects_flash(
    ScreenEffects *effects,
    Uint8 r,
    Uint8 g,
    Uint8 b,
    Uint8 alpha,
    Uint32 duration_ms,
    Uint32 current_time
);

// Draw the current flash as a translucent rect covering the full
// logical screen, if one is active - a no-op otherwise, so it's safe
// to call unconditionally every frame. Temporarily switches the
// renderer to alpha blending and restores whatever blend mode it had
// before, since nothing else in this project ever changes it away
// from the default.
void screen_effects_render_flash(
    SDL_Renderer *renderer,
    const ScreenEffects *effects,
    Uint32 current_time
);

#endif
