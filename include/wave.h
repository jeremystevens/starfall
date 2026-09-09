#ifndef WAVE_H
#define WAVE_H

#include <SDL.h>

// How long each wave lasts before the next one begins - aiming for
// the pacing of a classic NES-era shmup stage (roughly 1.5-3 minutes)
// before a boss eventually shows up at the end of it. Once bosses
// exist, a boss-flagged wave won't auto-advance on this timer at all;
// it'll wait for the boss to be defeated instead (see WaveDifficulty's
// boss_wave field). That's future work, not implemented yet.
#define WAVE_DURATION_MS 120000

// How long the "WAVE X" announcement stays on screen once a new wave
// begins.
#define WAVE_ANNOUNCEMENT_DURATION_MS 1800

// Tracks wave progression over time. The Wave Director doesn't own or
// spawn anything itself - it just decides which wave we're currently
// on and whether the "new wave" announcement should be showing, using
// elapsed GAME_PLAYING time the same way every other timer in this
// project already does.
typedef struct
{
    int current_wave; // 1-based - the player never sees "Wave 0".

    Uint32 wave_start_time;

    int announcement_active;
    Uint32 announcement_start_time;

} WaveState;

// Describes how aggressively the existing enemy/asteroid systems
// should behave for whatever wave is currently active - which threats
// are enabled, how often they spawn, and how often Scouts/Bombers
// fire. Calculated on demand from WaveState rather than stored - it's
// derived data, not state that needs to persist on its own.
typedef struct
{
    int scouts_enabled;
    int bombers_enabled;
    int asteroids_enabled;

    Uint32 scout_spawn_delay;
    Uint32 bomber_spawn_delay;
    Uint32 asteroid_spawn_delay;

    // How long a Scout/Bomber waits between shots once spawned. Baked
    // into the enemy at spawn time (see enemies_spawn_scout()) rather
    // than applied retroactively, so an already-spawned enemy keeps
    // whatever firing rate it was given even after the wave changes.
    Uint32 scout_fire_delay;
    Uint32 bomber_fire_delay;

    // Reserved for later - lets a future wave be flagged as a boss
    // encounter without reshaping this struct when that's added.
    int boss_wave;

} WaveDifficulty;

// Calculate how aggressively Scouts, Bombers, and asteroids should
// spawn for whatever wave is currently active. Waves 1-5 are
// hand-tuned introductory steps; Wave 6 and beyond currently plateau
// at Wave 5's settings until Phase 6 adds formula-based scaling.
WaveDifficulty wave_get_difficulty(const WaveState *wave);

// Reset wave progression to Wave 1, starting from current_time. Used
// both at game start and on restart. Also starts the Wave 1
// announcement, matching how every subsequent wave announces itself
// the instant it begins.
void wave_init(WaveState *wave, Uint32 current_time);

// Advance to the next wave once the current one's duration has
// elapsed (unless it's a boss wave - see WaveDifficulty.boss_wave),
// and hide the "new wave" announcement once it's been shown long
// enough. Call this only while GAME_PLAYING - wave progression should
// stay frozen whenever the rest of gameplay is, the same way every
// other timer in this project already behaves.
void wave_update(WaveState *wave, Uint32 current_time);

// Advance past a boss wave once the encounter has been defeated. The
// game coordinator calls this - wave.c never inspects boss state
// directly, it just waits to be told the boss wave is over. Resets
// the new wave's start time to current_time so time spent fighting
// the boss doesn't count against the next wave's duration.
void wave_advance_after_boss(WaveState *wave, Uint32 current_time);

// TEMPORARY DEBUG: jump straight to target_wave for fast iteration
// while testing. Not part of any real gameplay path.
void wave_debug_jump(WaveState *wave, int target_wave, Uint32 current_time);

// Draw the "WAVE X" / wave-name announcement overlay while one is
// active - slides in from the right, holds centered, then slides off
// to the left, all within the existing WAVE_ANNOUNCEMENT_DURATION_MS
// window (the transition doesn't change how long it's shown). Does
// nothing once announcement_active is false, so it's safe to call
// unconditionally every frame.
void wave_render_announcement(
    SDL_Renderer *renderer,
    const WaveState *wave
);

#endif
