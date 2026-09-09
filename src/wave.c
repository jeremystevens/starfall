#include "wave.h"

#include "game_config.h"
#include "text.h"
#include "enemy.h"
#include "asteroid.h"

#include <stdio.h>

// Wave 6+ difficulty scaling: how much each spawn/fire delay shrinks
// per difficulty_level (1 on Wave 6, 2 on Wave 7, and so on). The hard
// floors themselves (MIN_SCOUT_SPAWN_DELAY, MIN_SCOUT_FIRE_DELAY,
// etc.) live in enemy.h and asteroid.h - each system owns its own
// safety limit, and Phase 7's spawn-jitter code needs the same spawn
// constants at the point jitter is applied, so there's a single
// source of truth instead of copies that could drift apart.
#define SCOUT_SPAWN_SCALING_MS 30
#define BOMBER_SPAWN_SCALING_MS 50
#define ASTEROID_SPAWN_SCALING_MS 25
#define SCOUT_FIRE_SCALING_MS 20
#define BOMBER_FIRE_SCALING_MS 30

// Applies linear difficulty scaling to a base delay and clamps it to
// min_delay. Used for both spawn delays and fire delays - same shape
// of problem either way. The subtraction happens in a signed int, not
// the Uint32 the result is eventually stored as - doing it directly
// in unsigned arithmetic would let a high enough difficulty_level
// wrap the result around to a huge positive number instead of going
// negative, which would make spawns/firing nearly stop instead of
// intensify.
static Uint32 scale_delay_by_difficulty(
    int base_delay,
    int difficulty_level,
    int scaling_ms,
    Uint32 min_delay
)
{
    int scaled = base_delay - (scaling_ms * difficulty_level);

    if (scaled < (int)min_delay)
    {
        return min_delay;
    }

    return (Uint32)scaled;
}

// Hand-tuned settings for the five introductory waves. Wave 6+ scales
// mathematically from Wave 5's baseline instead of being hand-tuned
// per wave, with hard-clamped minimums so difficulty can climb
// indefinitely without ever producing an unsafe spawn delay.
WaveDifficulty wave_get_difficulty(const WaveState *wave)
{
    WaveDifficulty difficulty;

    // Every normal wave clears this immediately; only a boss wave
    // (currently just Wave 5) sets it below.
    difficulty.boss_wave = 0;

    switch (wave->current_wave)
    {
    case 1:
        // FIRST CONTACT - Scouts only, deliberately sparse so the
        // player has room to learn movement/shooting without feeling
        // outnumbered from the first few seconds.
        difficulty.scouts_enabled = 1;
        difficulty.bombers_enabled = 0;
        difficulty.asteroids_enabled = 0;
        difficulty.scout_spawn_delay = 2600;
        difficulty.bomber_spawn_delay = 0;
        difficulty.asteroid_spawn_delay = 0;
        difficulty.scout_fire_delay = 1700;
        difficulty.bomber_fire_delay = 0;
        break;

    case 2:
        // ASTEROID BELT - asteroids join, Scouts pick up only
        // slightly from Wave 1. Both delays kept well above Wave 3's
        // so this stays a gentle introduction, not a swarm - it still
        // ramps up plenty from here.
        difficulty.scouts_enabled = 1;
        difficulty.bombers_enabled = 0;
        difficulty.asteroids_enabled = 1;
        difficulty.scout_spawn_delay = 2200;
        difficulty.bomber_spawn_delay = 0;
        difficulty.asteroid_spawn_delay = 4500;
        difficulty.scout_fire_delay = 1600;
        difficulty.bomber_fire_delay = 0;
        break;

    case 3:
        // HEAVY CONTACT - Bombers arrive. Previously this wave also
        // slashed the scout and asteroid delays most of the way to
        // their Wave 4 values in the same step as introducing a brand
        // new enemy type, which stacked three difficulty increases
        // (new threat + faster scouts + far more asteroids) into one
        // jump and made this the wall most runs died on. Eased back
        // so introducing Bombers doesn't coincide with the biggest
        // spawn-rate jump in the whole progression - Wave 4 now picks
        // up more of that ramp instead.
        difficulty.scouts_enabled = 1;
        difficulty.bombers_enabled = 1;
        difficulty.asteroids_enabled = 1;
        difficulty.scout_spawn_delay = 1800;
        difficulty.bomber_spawn_delay = 4200;
        difficulty.asteroid_spawn_delay = 3200;
        difficulty.scout_fire_delay = 1400;
        difficulty.bomber_fire_delay = 2200;
        break;

    case 4:
        // CROSS FIRE - across-the-board pressure increase, but now a
        // step up from an already-survivable Wave 3 rather than
        // stacking onto one that was already close to overwhelming.
        difficulty.scouts_enabled = 1;
        difficulty.bombers_enabled = 1;
        difficulty.asteroids_enabled = 1;
        difficulty.scout_spawn_delay = 1400;
        difficulty.bomber_spawn_delay = 3600;
        difficulty.asteroid_spawn_delay = 2600;
        difficulty.scout_fire_delay = 1250;
        difficulty.bomber_fire_delay = 2100;
        break;

    case 5:
        // DREADNOUGHT - a boss encounter, not a normal wave. Normal
        // threat spawning is suppressed entirely (the boss controls
        // its own attacks, and later its own support Scouts), and it
        // does not advance on the normal timer at all - see
        // wave_update()'s boss_wave check below. The Wave 6+ scaling
        // baseline lives independently in the default case below, so
        // zeroing these out here doesn't affect it.
        difficulty.scouts_enabled = 0;
        difficulty.bombers_enabled = 0;
        difficulty.asteroids_enabled = 0;
        difficulty.scout_spawn_delay = 0;
        difficulty.bomber_spawn_delay = 0;
        difficulty.asteroid_spawn_delay = 0;
        difficulty.scout_fire_delay = 0;
        difficulty.bomber_fire_delay = 0;
        difficulty.boss_wave = 1;
        break;

    default:
    {
        // Wave 6+: everything Wave 5 already unlocked stays enabled;
        // spawn AND fire delays now shrink mathematically instead of
        // being hand-tuned per wave. Level 1 on Wave 6, level 2 on
        // Wave 7, and so on - scale_delay_by_difficulty() guarantees
        // none of these can ever go below their minimum, however high
        // the wave number gets, so higher waves get denser fire
        // without ever producing continuous projectile spam.
        int difficulty_level = wave->current_wave - 5;

        difficulty.scouts_enabled = 1;
        difficulty.bombers_enabled = 1;
        difficulty.asteroids_enabled = 1;

        difficulty.scout_spawn_delay = scale_delay_by_difficulty(
                                            800,
                                            difficulty_level,
                                            SCOUT_SPAWN_SCALING_MS,
                                            MIN_SCOUT_SPAWN_DELAY
                                        );

        difficulty.bomber_spawn_delay = scale_delay_by_difficulty(
                                             2500,
                                             difficulty_level,
                                             BOMBER_SPAWN_SCALING_MS,
                                             MIN_BOMBER_SPAWN_DELAY
                                         );

        difficulty.asteroid_spawn_delay = scale_delay_by_difficulty(
                                               1500,
                                               difficulty_level,
                                               ASTEROID_SPAWN_SCALING_MS,
                                               MIN_ASTEROID_SPAWN_DELAY
                                           );

        difficulty.scout_fire_delay = scale_delay_by_difficulty(
                                           1000,
                                           difficulty_level,
                                           SCOUT_FIRE_SCALING_MS,
                                           MIN_SCOUT_FIRE_DELAY
                                       );

        difficulty.bomber_fire_delay = scale_delay_by_difficulty(
                                            1800,
                                            difficulty_level,
                                            BOMBER_FIRE_SCALING_MS,
                                            MIN_BOMBER_FIRE_DELAY
                                        );

        break;
    }
    }

    return difficulty;
}

// Reset wave progression to Wave 1. Shared by game start and restart
// so the two can never drift apart.
void wave_init(WaveState *wave, Uint32 current_time)
{
    wave->current_wave = 1;
    wave->wave_start_time = current_time;

    // Wave 1 announces itself immediately, same as every wave after it.
    wave->announcement_active = 1;
    wave->announcement_start_time = current_time;

    printf("Wave %d begins!\n", wave->current_wave);
}

// Shared by both ways a wave can begin: bumps the wave number, starts
// its announcement, and logs it. Callers decide new_start_time - a
// drift-free increment for the normal timer (wave_update()), or "right
// now" for a boss that was just defeated (wave_advance_after_boss()),
// since a completion-based wave has no fixed cadence to stay aligned
// with.
static void begin_wave(WaveState *wave, Uint32 new_start_time)
{
    wave->current_wave++;
    wave->wave_start_time = new_start_time;

    wave->announcement_active = 1;
    wave->announcement_start_time = new_start_time;

    printf("Wave %d begins!\n", wave->current_wave);
}

// Advance to the next wave once the current one's duration has
// elapsed - unless the current wave is a boss encounter, in which
// case it never auto-advances on this timer at all; only
// wave_advance_after_boss() can move it forward. A while loop (rather
// than if) correctly catches up if an unusually long stall ever let
// more than one wave's worth of time pass between calls, instead of
// silently losing the extra time.
void wave_update(WaveState *wave, Uint32 current_time)
{
    while (!wave_get_difficulty(wave).boss_wave &&
            current_time - wave->wave_start_time >= WAVE_DURATION_MS)
    {
        begin_wave(wave, wave->wave_start_time + WAVE_DURATION_MS);
    }

    // Hide the announcement once it's been shown long enough - same
    // "check elapsed time, clear the flag" shape as the player's
    // invulnerability timer in player_update().
    if (wave->announcement_active &&
            current_time - wave->announcement_start_time >= WAVE_ANNOUNCEMENT_DURATION_MS)
    {
        wave->announcement_active = 0;
    }
}

// Called once a boss encounter has been defeated, to move on to the
// next wave. Boss waves never advance on the normal timer (see
// wave_update() above), so this is the only way out of one. The game
// coordinator (main.c) calls this once boss.c reports the encounter
// as defeated - wave.c deliberately never inspects boss health or
// state directly.
void wave_advance_after_boss(WaveState *wave, Uint32 current_time)
{
    begin_wave(wave, current_time);
}

// TEMPORARY DEBUG: jump straight to target_wave, skipping everything
// before it. Purely for fast iteration while testing later waves (the
// boss especially) without replaying the whole progression every
// time. Reuses begin_wave() so the jump behaves exactly like a real
// transition - same announcement, same console log. Not part of any
// real gameplay path; remove before release.
void wave_debug_jump(WaveState *wave, int target_wave, Uint32 current_time)
{
    wave->current_wave = target_wave - 1;
    begin_wave(wave, current_time);
}

// Name for each hand-tuned introductory wave. Waves past this point
// don't get a unique name - just a generated "THREAT LEVEL N" line -
// so this table never needs to grow.
static const char *wave_name(int wave_number)
{
    switch (wave_number)
    {
    case 1:
        return "FIRST CONTACT";
    case 2:
        return "ASTEROID BELT";
    case 3:
        return "HEAVY CONTACT";
    case 4:
        return "CROSS FIRE";
    case 5:
        return "DREADNOUGHT";
    default:
        return NULL;
    }
}

// How long the slide-in and slide-out phases each take, carved out of
// the existing WAVE_ANNOUNCEMENT_DURATION_MS window rather than adding
// to it - the announcement's total on-screen time is unchanged, only
// how it enters and leaves is.
#define WAVE_ANNOUNCEMENT_SLIDE_MS 300

// Where a line of text should currently be drawn: sliding in from just
// off the right edge, held at its centered position, then continuing
// off the left edge - all within the same window the announcement
// already uses, so this never touches wave timing itself.
// centered_x/text_w describe where the line settles once fully in
// view, from the same text_width()-based centering every other
// announcement in the game already uses.
static int announcement_slide_x(int centered_x, int text_w, Uint32 elapsed)
{
    if (elapsed < WAVE_ANNOUNCEMENT_SLIDE_MS)
    {
        // Sliding in: from just off the right edge to centered.
        float fraction = (float)elapsed / (float)WAVE_ANNOUNCEMENT_SLIDE_MS;
        int start_x = SCREEN_WIDTH;

        return start_x + (int)((float)(centered_x - start_x) * fraction);
    }

    Uint32 slide_out_start =
        WAVE_ANNOUNCEMENT_DURATION_MS - WAVE_ANNOUNCEMENT_SLIDE_MS;

    if (elapsed >= slide_out_start)
    {
        // Sliding out: from centered to just off the left edge,
        // continuing the same left-moving direction it entered with.
        float fraction =
            (float)(elapsed - slide_out_start) / (float)WAVE_ANNOUNCEMENT_SLIDE_MS;
        int end_x = -text_w;

        return centered_x + (int)((float)(end_x - centered_x) * fraction);
    }

    // Holding steady in the middle.
    return centered_x;
}

// Draw the wave announcement overlay: slides in from the right,
// holds centered, then slides off to the left (see
// announcement_slide_x() above).
void wave_render_announcement(
    SDL_Renderer *renderer,
    const WaveState *wave
)
{
    if (!wave->announcement_active)
    {
        return;
    }

    Uint32 elapsed = SDL_GetTicks() - wave->announcement_start_time;

    char wave_line[16];
    snprintf(wave_line, sizeof(wave_line), "WAVE %d", wave->current_wave);

    char name_line[32];
    const char *name = wave_name(wave->current_wave);

    if (name != NULL)
    {
        snprintf(name_line, sizeof(name_line), "%s", name);
    }
    else
    {
        // Wave 6 -> THREAT LEVEL 1, Wave 7 -> THREAT LEVEL 2, etc.
        snprintf(
            name_line,
            sizeof(name_line),
            "THREAT LEVEL %d",
            wave->current_wave - 5
        );
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    int wave_scale = 2;
    int wave_text_w = text_width(wave_line, wave_scale);
    int wave_centered_x = (SCREEN_WIDTH - wave_text_w) / 2;
    int wave_x = announcement_slide_x(wave_centered_x, wave_text_w, elapsed);
    text_draw(renderer, wave_line, wave_x, 40, wave_scale);

    int name_scale = 1;
    int name_text_w = text_width(name_line, name_scale);
    int name_centered_x = (SCREEN_WIDTH - name_text_w) / 2;
    int name_x = announcement_slide_x(name_centered_x, name_text_w, elapsed);
    text_draw(renderer, name_line, name_x, 58, name_scale);
}
