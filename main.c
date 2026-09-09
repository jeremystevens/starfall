// SDL port of my original Pyxel/Python space shooter.
#include <stdio.h>  // printf() for console output and error messages
#include <SDL.h>    // SDL2 functions, types, and constants
#include <stdlib.h>

// Game systems, each with its own header.
#include "player.h"
#include "starfield.h"
#include "bullet.h"
#include "audio.h"
#include "enemy.h"
#include "enemy_bullet.h"
#include "asteroid.h"
#include "powerup.h"
#include "wave.h"
#include "boss.h"
#include "explosion.h"
#include "collision.h"
#include "text.h"
#include "game_config.h"
#include "highscore.h"
#include "screen_effects.h"
#include "popup.h"
#include "music.h"

// Target frame rate for the main loop.
#define TARGET_FPS 60
#define FRAME_TIME (1000 / TARGET_FPS)

// Screen shake tuning - conservative displacements sized for the
// 160x120 logical resolution (see screen_effects.h). A player hit is
// a brief, small jolt; the Dreadnought's final destruction is the
// strongest shake in the game so far, reserved for that one moment so
// it still reads as more significant than an ordinary hit.
#define PLAYER_HIT_SHAKE_MAGNITUDE 2
#define PLAYER_HIT_SHAKE_DURATION_MS 150
#define BOSS_DEFEATED_SHAKE_MAGNITUDE 4
#define BOSS_DEFEATED_SHAKE_DURATION_MS 400

// How long the player's final explosion gets to play out before
// GAME_OVER actually appears - long enough to feel dramatic, short
// enough not to feel like a stall.
#define PLAYER_DEATH_DELAY_MS 1000

// A brief, subtle full-screen flash on the exact frame the final life
// is lost - on top of the existing player-hit shake, which already
// fires from whichever collision caused this. Kept translucent (see
// screen_effects_render_flash()) rather than opaque so it doesn't
// hide the destruction effect playing out underneath it, and short
// enough to fade out well before the death delay ends.
#define PLAYER_DEATH_FLASH_R 200
#define PLAYER_DEATH_FLASH_G 40
#define PLAYER_DEATH_FLASH_B 40
#define PLAYER_DEATH_FLASH_ALPHA 120
#define PLAYER_DEATH_FLASH_DURATION_MS 300

// Title screen theme (v0.8.0 Phases 3B/4B) - the melody (voice 0,
// Phase 3B) plus its waltz accompaniment (voice 1, Phase 4B).
// Transcribed from the Mutopia Project's public-domain piano
// arrangement of Johann Strauss II's "The Blue Danube Waltz" (Op.
// 314), Music ID 519, arranged by N. Kouremenos, licensed CC BY-SA 4.0
// - the 1867 composition itself is long since public domain; this
// credits only the source arrangement this data was transcribed from.
// Bars 1-17 (including the pickup), verified note-by-note against
// both the LilyPond source and its MIDI export during Phases 3A/4A -
// see those phases' reports for the full transcription, the
// accompaniment's chord-reduction rule, and cross-checks. Do not
// hand-edit these values; regenerate from the verified source if a
// correction is ever needed.
static const MusicNote BLUE_DANUBE_MELODY[] =
{
    { 261.63, 17640 },  // C4, quarter (pickup)
    { 261.63, 17640 },  // C4, quarter
    { 329.63, 17640 },  // E4, quarter
    { 392.00, 17640 },  // G4, quarter
    { 392.00, 35280 },  // G4, half
    { 783.99, 17640 },  // G5, quarter
    { 783.99, 35280 },  // G5, half
    { 659.26, 17640 },  // E5, quarter
    { 659.26, 35280 },  // E5, half
    { 261.63, 17640 },  // C4, quarter
    { 261.63, 17640 },  // C4, quarter
    { 329.63, 17640 },  // E4, quarter
    { 392.00, 17640 },  // G4, quarter
    { 392.00, 35280 },  // G4, half
    { 783.99, 17640 },  // G5, quarter
    { 783.99, 35280 },  // G5, half
    { 698.46, 17640 },  // F5, quarter
    { 698.46, 35280 },  // F5, half
    { 246.94, 17640 },  // B3, quarter
    { 246.94, 17640 },  // B3, quarter
    { 293.66, 17640 },  // D4, quarter
    { 440.00, 17640 },  // A4, quarter
    { 440.00, 35280 },  // A4, half
    { 880.00, 17640 },  // A5, quarter
    { 880.00, 35280 },  // A5, half
    { 698.46, 17640 },  // F5, quarter
    { 698.46, 35280 },  // F5, half
    { 246.94, 17640 },  // B3, quarter
    { 246.94, 17640 },  // B3, quarter
    { 293.66, 17640 },  // D4, quarter
    { 440.00, 17640 },  // A4, quarter
    { 440.00, 35280 },  // A4, half
    { 880.00, 17640 },  // A5, quarter
    { 880.00, 35280 },  // A5, half
    { 659.26, 17640 },  // E5, quarter
    { 659.26, 35280 },  // E5, half
    { 261.63, 17640 },  // C4, quarter
};

#define BLUE_DANUBE_MELODY_COUNT \
    (sizeof(BLUE_DANUBE_MELODY) / sizeof(BLUE_DANUBE_MELODY[0]))

// Waltz accompaniment for voice 1 (Phase 4B) - a monophonic
// bass-on-beat-1 / chord-on-beats-2-and-3 reduction of the source's
// lower piano staff, source-verified during Phase 4A. Every
// beat-2/beat-3 chord in this excerpt is reduced to its top tone (G3),
// the one pitch common to both chord types the source actually uses
// here (<E3,G3> in the C-bass bars, <F3,G3> in the D-bass bars) - see
// Phase 4A's report for the full reduction rule and justification.
// The two leading rest events reproduce the source's own four beats
// of accompaniment silence under the melody's pickup and first bar;
// removing them would desync this voice from BLUE_DANUBE_MELODY.
// Deliberately the same 49 beats / 864,360 samples as voice 0 above -
// do not edit one without re-verifying the other still matches.
static const MusicNote BLUE_DANUBE_ACCOMPANIMENT[] =
{
    { 0.00, 17640 },    // bar1 pickup rest
    { 0.00, 52920 },    // bar2 full-bar rest
    { 130.81, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar3 C3 G3 G3
    { 130.81, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar4
    { 130.81, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar5
    { 130.81, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar6
    { 146.83, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar7 D3 G3 G3
    { 146.83, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar8
    { 146.83, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar9
    { 146.83, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar10
    { 146.83, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar11
    { 146.83, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar12
    { 146.83, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar13
    { 146.83, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar14
    { 130.81, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar15 C3 G3 G3
    { 130.81, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar16
    { 130.81, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar17
};

#define BLUE_DANUBE_ACCOMPANIMENT_COUNT \
    (sizeof(BLUE_DANUBE_ACCOMPANIMENT) / sizeof(BLUE_DANUBE_ACCOMPANIMENT[0]))

// Dreadnought boss theme (v0.8.0 Phase 5B) - Edvard Grieg, "In the
// Hall of the Mountain King," Op. 46 No. 4 (Peer Gynt Suite I).
// Source: Mutopia Project Music ID 1888, Grieg's own public-domain
// piano reduction (both the 1874 composition and this piano source are
// public domain - no CC attribution requirement, unlike Blue Danube's
// arrangement). Bars 2-17 (16 bars, 4/4, base tempo 138 BPM), verified
// note-by-note against the LilyPond source and its MIDI export during
// Phase 5A, independently re-summed to confirm exact totals during
// Phase 5A.1 - see those phases' reports for the full transcription,
// the two documented octave-doubling reductions in the melody, and the
// reasoning behind the bar 2-17 loop boundary. Do not hand-edit these
// values or "correct" their ~0.126ms rounding difference from
// mathematically exact 138 BPM - that rounding is intentional (see
// Phase 5A.1); regenerate from the verified source instead if a real
// correction is ever needed.
static const MusicNote MOUNTAIN_KING_MELODY[] =
{
    // bar 2
    { 61.74, 9587 },  { 69.30, 9587 },  { 73.42, 9587 },  { 82.41, 9587 },
    { 92.50, 9587 },  { 73.42, 9587 },  { 92.50, 19174 },
    // bar 3
    { 87.31, 9587 },  { 69.30, 9587 },  { 87.31, 19174 },
    { 82.41, 9587 },  { 65.41, 9587 },  { 82.41, 19174 },
    // bar 4
    { 61.74, 9587 },  { 69.30, 9587 },  { 73.42, 9587 },  { 82.41, 9587 },
    { 92.50, 9587 },  { 73.42, 9587 },  { 92.50, 9587 },  { 123.47, 9587 },
    // bar 5
    { 110.00, 9587 }, { 92.50, 9587 },  { 73.42, 9587 },  { 92.50, 9587 }, { 110.00, 38348 },
    // bar 6
    { 123.47, 9587 }, { 138.59, 9587 }, { 146.83, 9587 }, { 164.81, 9587 },
    { 185.00, 9587 }, { 146.83, 9587 }, { 185.00, 19174 },
    // bar 7
    { 174.61, 9587 }, { 138.59, 9587 }, { 174.61, 19174 },
    { 164.81, 9587 }, { 130.81, 9587 }, { 164.81, 19174 },
    // bar 8
    { 123.47, 9587 }, { 138.59, 9587 }, { 146.83, 9587 }, { 164.81, 9587 },
    { 185.00, 9587 }, { 146.83, 9587 }, { 185.00, 9587 }, { 246.94, 9587 },
    // bar 9
    { 220.00, 9587 }, { 185.00, 9587 }, { 146.83, 9587 }, { 185.00, 9587 }, { 220.00, 38348 },
    // bar 10
    { 92.50, 9587 },  { 103.83, 9587 }, { 116.54, 9587 }, { 123.47, 9587 },
    { 138.59, 9587 }, { 116.54, 9587 }, { 138.59, 19174 },
    // bar 11
    { 146.83, 9587 }, { 116.54, 9587 }, { 146.83, 19174 },
    { 138.59, 9587 }, { 116.54, 9587 }, { 138.59, 19174 },
    // bar 12
    { 92.50, 9587 },  { 103.83, 9587 }, { 116.54, 9587 }, { 123.47, 9587 },
    { 138.59, 9587 }, { 116.54, 9587 }, { 138.59, 19174 },
    // bar 13
    { 146.83, 9587 }, { 116.54, 9587 }, { 146.83, 19174 }, { 138.59, 38348 },
    // bar 14
    { 185.00, 9587 }, { 207.65, 9587 }, { 233.08, 9587 }, { 246.94, 9587 },
    { 277.18, 9587 }, { 233.08, 9587 }, { 277.18, 19174 },
    // bar 15
    { 293.66, 9587 }, { 233.08, 9587 }, { 293.66, 19174 },
    { 277.18, 9587 }, { 233.08, 9587 }, { 277.18, 19174 },
    // bar 16
    { 185.00, 9587 }, { 207.65, 9587 }, { 233.08, 9587 }, { 246.94, 9587 },
    { 277.18, 9587 }, { 233.08, 9587 }, { 277.18, 19174 },
    // bar 17
    { 293.66, 9587 }, { 233.08, 9587 }, { 293.66, 19174 }, { 277.18, 38348 },
};

#define MOUNTAIN_KING_MELODY_COUNT \
    (sizeof(MOUNTAIN_KING_MELODY) / sizeof(MOUNTAIN_KING_MELODY[0]))

// Voice 1 for the Dreadnought theme - the source's own left-hand bass
// drone (root/fifth alternation), already monophonic in the source
// with no chord reduction needed - see Phase 5A's report. Same 64
// beats / 1,227,136 samples as MOUNTAIN_KING_MELODY above - do not
// edit one without re-verifying the other still matches (Phase 5A.1
// re-confirmed this exact total independently for both arrays).
static const MusicNote MOUNTAIN_KING_ACCOMPANIMENT[] =
{
    // bars 2-4 (B minor drone)
    { 30.87, 19174 }, { 46.25, 19174 }, { 30.87, 19174 }, { 46.25, 19174 },
    { 30.87, 19174 }, { 46.25, 19174 }, { 30.87, 19174 }, { 46.25, 19174 },
    { 30.87, 19174 }, { 46.25, 19174 }, { 30.87, 19174 }, { 46.25, 19174 },
    // bar 5 (cadence)
    { 36.71, 19174 }, { 55.00, 19174 }, { 36.71, 19174 }, { 55.00, 19174 },
    // bars 6-8 (repeat)
    { 30.87, 19174 }, { 46.25, 19174 }, { 30.87, 19174 }, { 46.25, 19174 },
    { 30.87, 19174 }, { 46.25, 19174 }, { 30.87, 19174 }, { 46.25, 19174 },
    { 30.87, 19174 }, { 46.25, 19174 }, { 30.87, 19174 }, { 46.25, 19174 },
    // bar 9 (cadence)
    { 36.71, 19174 }, { 55.00, 19174 }, { 36.71, 19174 }, { 55.00, 19174 },
    // bars 10-13 (transposed sequence)
    { 46.25, 19174 }, { 69.30, 19174 }, { 46.25, 19174 }, { 69.30, 19174 },
    { 36.71, 19174 }, { 58.27, 19174 }, { 46.25, 19174 }, { 69.30, 19174 },
    { 46.25, 19174 }, { 69.30, 19174 }, { 46.25, 19174 }, { 69.30, 19174 },
    { 36.71, 19174 }, { 58.27, 19174 }, { 46.25, 19174 }, { 69.30, 19174 },
    // bars 14-17 (repeat)
    { 46.25, 19174 }, { 69.30, 19174 }, { 46.25, 19174 }, { 69.30, 19174 },
    { 36.71, 19174 }, { 58.27, 19174 }, { 46.25, 19174 }, { 69.30, 19174 },
    { 46.25, 19174 }, { 69.30, 19174 }, { 46.25, 19174 }, { 69.30, 19174 },
    { 36.71, 19174 }, { 58.27, 19174 }, { 46.25, 19174 }, { 69.30, 19174 },
};

#define MOUNTAIN_KING_ACCOMPANIMENT_COUNT \
    (sizeof(MOUNTAIN_KING_ACCOMPANIMENT) / sizeof(MOUNTAIN_KING_ACCOMPANIMENT[0]))

// Game Over theme (v0.8.0 Phase 6B) - Frederic Chopin, "Marche
// funebre," Piano Sonata No. 2 in B-flat minor, Op. 35, Mvt. III.
// Source: Chopin Online (CFEO/OCVE, Universities of Cambridge/King's
// College London), facsimile of the 1840 Breitkopf & Hartel first
// edition - public domain. Bars 1-2 only (identical bars in the
// source; verified note-by-note during Phase 6A - see that phase's
// report for the full transcription, the bass-clef notation of the
// melody, and the chord-reduction rule for the accompaniment). The
// ~20-35s target was deliberately not reached: bar 3 onward could not
// be verified to this project's required standard, so the excerpt was
// intentionally shortened rather than guessed. BASE_BPM = 60 is an
// arrangement choice (Chopin's score gives only "Lento," no numeric
// tempo) - do not treat it as a source fact. Played once, not looped
// (see Phase 6B) - bar 2 already repeats bar 1 verbatim, so looping
// this excerpt would make the repetition too obvious for a one-time
// Game Over cue.
static const MusicNote FUNERAL_MARCH_MELODY[] =
{
    { 233.08, 44100 },  // Bb3, quarter      (bar 1)
    { 233.08, 33075 },  // Bb3, dotted-eighth
    { 207.65, 11025 },  // Ab3, sixteenth
    { 261.63, 88200 },  // C4,  half
    { 233.08, 44100 },  // Bb3, quarter      (bar 2)
    { 233.08, 33075 },  // Bb3, dotted-eighth
    { 207.65, 11025 },  // Ab3, sixteenth
    { 261.63, 88200 },  // C4,  half
};

#define FUNERAL_MARCH_MELODY_COUNT \
    (sizeof(FUNERAL_MARCH_MELODY) / sizeof(FUNERAL_MARCH_MELODY[0]))

// Accompaniment: the source's low open fifth (Bb1+F2, no third)
// reduced to its root (Bb1) - see Phase 6A's report for why the root
// was chosen over the fifth. Same 8 beats / 352,800 samples as
// FUNERAL_MARCH_MELODY above - do not edit one without re-verifying
// the other still matches.
static const MusicNote FUNERAL_MARCH_ACCOMPANIMENT[] =
{
    { 58.27, 44100 },  // Bb1, quarter (bar 1, attack 1)
    { 58.27, 44100 },  // Bb1, quarter (bar 1, attack 2)
    { 58.27, 44100 },  // Bb1, quarter (bar 1, attack 3)
    { 58.27, 44100 },  // Bb1, quarter (bar 1, attack 4)
    { 58.27, 44100 },  // Bb1, quarter (bar 2, attack 1)
    { 58.27, 44100 },  // Bb1, quarter (bar 2, attack 2)
    { 58.27, 44100 },  // Bb1, quarter (bar 2, attack 3)
    { 58.27, 44100 },  // Bb1, quarter (bar 2, attack 4)
};

#define FUNERAL_MARCH_ACCOMPANIMENT_COUNT \
    (sizeof(FUNERAL_MARCH_ACCOMPANIMENT) / sizeof(FUNERAL_MARCH_ACCOMPANIMENT[0]))

int score = 0;

// The states the game can be in.
typedef enum
{
    GAME_TITLE,        // Idle on the title screen - no gameplay systems run.
    GAME_PLAYING,
    GAME_PAUSED,       // GAME_PLAYING frozen mid-run - see the P key handling below.
    GAME_PLAYER_DEATH, // Final life just lost - a short dramatic pause before GAME_OVER.
    GAME_OVER

} GameState;

// How long the game has spent paused so far, in total, folded in only
// once a pause ends (see the P key handling below) - NOT updated while
// a pause is still in progress. Every gameplay timer in this file is a
// Uint32 timestamp compared against "now" via game_ticks() below
// rather than raw SDL_GetTicks() - spawn delays, wave duration,
// invulnerability, boss timers, popups, screen effects, all of it -
// so pausing correctly freezes every one of them at once instead of
// only skipping the frame-by-frame update loop while their underlying
// timestamps keep quietly falling behind the wall clock. Without this,
// resuming from a pause would make every "time since X" check see a
// sudden jump equal to however long the game was paused, which reads
// as a burst of enemies spawning all at once, a wave ending early, or
// invulnerability/power-ups expiring instantly.
static Uint32 game_paused_offset_ms = 0;

// Whether a pause is currently in progress, and the frozen game_ticks()
// value to keep returning for as long as it is. This pair exists
// because game_paused_offset_ms above is only updated at the MOMENT a
// pause ends - while the pause is still ongoing, SDL_GetTicks() keeps
// climbing every real frame, so without freezing the return value
// directly here, anything that calls game_ticks() outside the
// GAME_PLAYING-gated update block (screen_effects_get_shake_offset()
// and screen_effects_render_flash() below both run every frame
// regardless of state) would keep seeing time pass - a shake or flash
// would silently keep animating behind the "PAUSED" text.
static int game_is_paused = 0;
static Uint32 game_paused_at_tick = 0;

// "Now", per every gameplay timer in this file - see
// game_paused_offset_ms and game_is_paused above for why this exists
// instead of calling SDL_GetTicks() directly. The one exception is the
// frame-rate cap at the bottom of the main loop, which measures real
// wall-clock frame duration and must not be shifted or frozen by this.
static Uint32 game_ticks(void)
{
    if (game_is_paused)
    {
        return game_paused_at_tick;
    }

    return SDL_GetTicks() - game_paused_offset_ms;
}

// Resets every piece of gameplay state to a fresh run. This is the
// one place both "start a new game from the title screen" and
// "restart after GAME_OVER" go through, so the two paths can never
// quietly drift apart the way two separate copies of the same reset
// list eventually do.
//
// current_time is passed in (rather than each system calling
// game_ticks() itself) so every timer this touches - Wave 1's
// announcement included - is anchored to the exact moment the run
// actually begins. That matters: wave_init() used to only run once,
// at program startup, so its "how long has this announcement been
// showing" timer started counting from launch instead of from
// whenever the player actually pressed SPACE. Sit on the title screen
// for a couple of seconds and the announcement had already silently
// expired before Wave 1 ever became visible. Calling wave_init() here
// - exactly when a run starts - fixes that.
//
// score is a global (see its declaration above) rather than a
// parameter, since every other read/write of it in this file already
// treats it that way.
static void game_start_new(
    Player *player,
    Bullet bullets[],
    Enemy enemies[],
    EnemyBullet enemy_bullets[],
    Asteroid asteroids[],
    PowerUp powerups[],
    PowerUpState *powerup_state,
    ExplosionParticle explosions[],
    WaveState *wave,
    Boss *boss,
    Uint32 *last_shot_time,
    Uint32 *last_scout_spawn,
    Uint32 *last_bomber_spawn,
    Uint32 *last_asteroid_spawn,
    int *new_high_score,
    ScreenEffects *screen_effects,
    ScorePopup score_popups[],
    LaserSound *laser,
    Uint32 current_time
)
{
    player_init(player);
    bullets_init(bullets);
    enemies_init(enemies);
    enemy_bullets_init(enemy_bullets);
    asteroids_init(asteroids);
    powerups_init(powerups);
    powerup_state_init(powerup_state);
    explosions_init(explosions);
    wave_init(wave, current_time);
    boss_init(boss);
    screen_effects_init(screen_effects);
    popups_init(score_popups);

    // Unconditionally, regardless of what state the boss was in before
    // this run started - if a restart happens mid-warning, nothing
    // else would ever turn this back off, and it would otherwise loop
    // forever into the new run.
    audio_set_boss_warning(laser, 0);

    *last_shot_time = 0;
    *last_scout_spawn = current_time;
    *last_bomber_spawn = current_time;
    *last_asteroid_spawn = current_time;

    // Cleared here too, not just where it's set in GAME_OVER - a
    // fresh run shouldn't inherit last run's "you just beat the
    // record" flag before this run's own Game Over has had a chance
    // to decide one way or the other.
    *new_high_score = 0;

    score = 0;
}

int main(void)
{
    // Create an array capable of storing all background stars.
    Star stars[MAX_STARS];
    starfield_init(stars);

    // Start on the title screen rather than dropping straight into
    // Wave 1 - see game_state's declaration for what each state means.
    GameState game_state = GAME_TITLE;

    // SDL_Init() returns 0 on success and a non-zero value on failure.
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
    {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // Load the persisted high score once, here at startup - not on
    // every frame, and not re-read from disk anywhere else. It only
    // changes in memory for the rest of the session (see the
    // GAME_OVER new-high-score check added in a later phase), which
    // is also the only time it gets written back out.
    int high_score = highscore_load();

    // Shared state for every procedurally generated sound effect.
    LaserSound laser;

    if (!audio_init(&laser))
    {
        printf("Audio initialization failed.\n");
    }

    // Start the title theme now, since game_state above already begins
    // on GAME_TITLE - this is that state's one-time entry, not
    // something the frame loop repeats. Both voices start together
    // under one lock (see audio_music_start_dual()) so the melody and
    // its accompaniment are sample-aligned from the very first
    // callback, and playback_rate is reset to 1.0 as part of that same
    // locked operation.
    audio_music_start_dual(
        &laser,
        BLUE_DANUBE_MELODY, BLUE_DANUBE_MELODY_COUNT, 1,
        BLUE_DANUBE_ACCOMPANIMENT, BLUE_DANUBE_ACCOMPANIMENT_COUNT, 1
    );

    // Create the main game window.
    // SDL_CreateWindow() returns a pointer to the new window,
    // or NULL if the window could not be created.
    SDL_Window *window = SDL_CreateWindow(
                             "Starfall",
                             SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED,
                             640,
                             480,
                             SDL_WINDOW_SHOWN
                         );

    // Always check pointers returned by SDL before using them.
    if (window == NULL)
    {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create the renderer used to draw graphics into our window.
    // -1 tells SDL to choose an appropriate graphics driver.
    // SDL_RENDERER_ACCELERATED requests hardware-accelerated rendering.
    SDL_Renderer *renderer = SDL_CreateRenderer(
                                 window,
                                 -1,
                                 SDL_RENDERER_ACCELERATED
                             );

    if (renderer == NULL)
    {
        printf("Renderer creation failed: %s\n", SDL_GetError());

        // The window was already created, destroy it before exiting.
        SDL_DestroyWindow(window);
        audio_shutdown();
        SDL_Quit();
        return 1;
    }

    // Render at the original game's 160x120 resolution; SDL scales
    // this up to fill our 640x480 window.
    if (SDL_RenderSetLogicalSize(renderer, 160, 120) != 0)
    {
        printf("Failed to set logical resolution: %s\n", SDL_GetError());

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Create and initialize the player.
    Player player;
    player_init(&player);

    // Create the player's bullet pool.
    Bullet bullets[MAX_BULLETS];
    bullets_init(bullets);

    // The game keeps running as long as this stays true.
    int running = 1;

    // SDL_Event stores information about events such as
    // keyboard input, mouse input, and closing the window.
    SDL_Event event;

    // Stores the time when the previous bullet was fired.
    Uint32 last_shot_time = 0;

    // Set whenever a game starts while SPACE is still physically held
    // down (the same press used to leave the title screen), and held
    // until that key is released. A human press easily spans several
    // frames, so skipping the fire logic on just the transition frame
    // (see start_game_requested below) isn't enough on its own - this
    // covers every frame the original press is still down, however
    // long that turns out to be, and asks nothing of how long a press
    // actually lasts.
    int suppress_fire_until_space_released = 0;

    // When GAME_PLAYER_DEATH began - compared against
    // PLAYER_DEATH_DELAY_MS to know when to actually hand off to
    // GAME_OVER. Always freshly set at the moment death is detected,
    // so there's no stale-value risk from a previous run.
    Uint32 player_death_started_at = 0;

    // When the current pause began (raw wall-clock time, not
    // game_ticks() - this is the one timestamp that must NOT be offset
    // by game_paused_offset_ms, since it's what that offset gets
    // computed from on unpause). Only meaningful while
    // game_state == GAME_PAUSED.
    Uint32 pause_started_at = 0;

    // Which Mountain King intensity tier the boss music is currently
    // playing at (1/2/3, matching boss.phase exactly - see the
    // BOSS_STATE_ACTIVE tier check below). Tracked separately from
    // boss.phase itself so the music's playback-rate setter is only
    // called on an actual tier CHANGE, not every frame - reset to 1
    // whenever Mountain King (re)starts, alongside boss.phase's own
    // reset to 1 in boss_init()/boss_spawn(), so a fresh encounter
    // never begins mid-tier from a previous fight.
    int boss_music_tier = 1;

    // Set at most once per run, at the GAME_PLAYER_DEATH -> GAME_OVER
    // handoff - never re-evaluated on later frames while GAME_OVER is
    // just sitting on screen. Read by the GAME_OVER render block to
    // decide whether to show "NEW HIGH SCORE!" instead of the normal
    // "HIGH SCORE <n>" line.
    int new_high_score = 0;

    // True while GAME_OVER is waiting for the new-high-score fanfare
    // to actually finish before starting the Funeral March (v0.8.0
    // Phase 6B) - set only at the GAME_PLAYER_DEATH -> GAME_OVER
    // handoff below when this run set a new record, and cleared
    // either once audio_new_high_score_finished() reports true (the
    // normal path) or by ENTER cancelling Game Over audio early. Never
    // set when there's no fanfare to wait for, so the Funeral March
    // starts immediately in that case instead.
    int game_over_awaiting_fanfare = 0;

    // Create the enemy and enemy-bullet pools.
    Enemy enemies[MAX_ENEMIES];
    EnemyBullet enemy_bullets[MAX_ENEMY_BULLETS];

    enemies_init(enemies);
    enemy_bullets_init(enemy_bullets);

    // Create and initialize the asteroid pool.
    Asteroid asteroids[MAX_ASTEROIDS];
    asteroids_init(asteroids);

    // Create and initialize the power-up pool.
    PowerUp powerups[MAX_POWERUPS];
    powerups_init(powerups);

    // Tracks which power-up effects are currently active for the
    // player - a single struct instead of loose timer variables.
    PowerUpState powerup_state;
    powerup_state_init(&powerup_state);

    // Create and initialize the shared explosion particle pool.
    ExplosionParticle explosions[MAX_EXPLOSION_PARTICLES];
    explosions_init(explosions);

    // Floating "+value" score popups - purely visual, never awards
    // score itself (see popup.h).
    ScorePopup score_popups[MAX_SCORE_POPUPS];
    popups_init(score_popups);

    // Tracks wave progression - decides difficulty, existing systems
    // still do all the actual spawning/moving/rendering.
    WaveState wave;
    wave_init(&wave, game_ticks());

    // The boss encounter - a single instance, since only one is ever
    // active at a time. Starts inactive; boss_spawn() is triggered
    // once the Wave Director reports a boss wave.
    Boss boss;
    boss_init(&boss);

    // Render-only presentation effects (currently just screen shake) -
    // never read by collision, movement, or any other gameplay system.
    ScreenEffects screen_effects;
    screen_effects_init(&screen_effects);

    // Scouts and Bombers now spawn on independent timers so the Wave
    // Director can tune their rates separately.
    Uint32 last_scout_spawn = 0;
    Uint32 last_bomber_spawn = 0;
    // Tracks when the last asteroid was spawned.
    Uint32 last_asteroid_spawn = 0;
    // -------------------------
    // Main Game Loop
    // -------------------------
    while (running)
    {
        Uint32 frame_start = SDL_GetTicks();

        // Set for this frame only when SPACE is first pressed (not
        // held) while on the title screen. Checked as a discrete
        // key-down event rather than the continuous keyboard[] state
        // used for firing below, and acted on only after gameplay has
        // already been processed this frame (see the bottom of the
        // loop) - so the same press that leaves the title screen can
        // never also register as the first frame's shot.
        int start_game_requested = 0;

        // Set for this frame only when ENTER is first pressed while on
        // the Game Over screen - same discrete key-down approach as
        // start_game_requested above, and deliberately a different key
        // from it. If leaving GAME_OVER used SPACE too, dying while
        // holding SPACE (very possible - that's the fire button)
        // would let a single held key walk straight through
        // GAME_OVER -> GAME_TITLE -> a brand new game, skipping the
        // title screen the player never actually chose to leave.
        int return_to_title_requested = 0;

        // Process all events currently waiting in SDL's event queue.
        // &event passes the address of our event variable so that
        // SDL_PollEvent() can write event information into it.
        while (SDL_PollEvent(&event))
        {
            // SDL_QUIT occurs when the user requests that the
            // application close, such as clicking the window's X button.
            if (event.type == SDL_QUIT)
            {
                running = 0;
            }

            // event.key.repeat is nonzero for the auto-repeated
            // key-down events SDL sends while a key stays held -
            // checking for it being 0 means this only latches once
            // per physical press.
            if (event.type == SDL_KEYDOWN && !event.key.repeat)
            {
                if (event.key.keysym.scancode == SDL_SCANCODE_SPACE &&
                        game_state == GAME_TITLE)
                {
                    start_game_requested = 1;
                }

                if (event.key.keysym.scancode == SDL_SCANCODE_RETURN &&
                        game_state == GAME_OVER)
                {
                    return_to_title_requested = 1;
                }

                // P toggles pause, but only while actually playing -
                // pausing during the title screen, the death pause, or
                // Game Over wouldn't mean anything (nothing is running
                // to freeze), so it's deliberately a no-op there rather
                // than a state jump to somewhere the rest of the game
                // doesn't expect.
                if (event.key.keysym.scancode == SDL_SCANCODE_P)
                {
                    if (game_state == GAME_PLAYING)
                    {
                        // Capture the frozen game_ticks() value BEFORE
                        // flipping game_is_paused on, since game_ticks()
                        // itself checks that flag - this has to read
                        // the last real (unpaused) reading, not the
                        // frozen one it's about to start returning.
                        game_paused_at_tick = game_ticks();
                        game_is_paused = 1;

                        game_state = GAME_PAUSED;

                        // Raw SDL_GetTicks(), not game_ticks() - see
                        // pause_started_at's declaration above.
                        pause_started_at = SDL_GetTicks();
                    }
                    else if (game_state == GAME_PAUSED)
                    {
                        // Fold however long this pause lasted into the
                        // running offset so game_ticks() resumes from
                        // exactly game_paused_at_tick with no jump, the
                        // instant game_is_paused clears below.
                        game_paused_offset_ms += SDL_GetTicks() - pause_started_at;
                        game_is_paused = 0;

                        game_state = GAME_PLAYING;
                    }
                }

            }

        }

        // Get the current state of the keyboard.
        // This lets us detect keys that are being held down.
        const Uint8 *keyboard = SDL_GetKeyboardState(NULL);

        if (game_state == GAME_PLAYING)
        {
            // Stay below the boss health bar overlay during a boss
            // fight instead of the normal (shorter) HUD strip, so the
            // player can't fly up behind it.
            int player_min_y =
                (boss.state != BOSS_STATE_INACTIVE)
                ? BOSS_FIGHT_TOP_BOUNDARY
                : HUD_HEIGHT;

            player_update(
                &player,
                keyboard,
                game_ticks(),
                player_min_y
            );

            // Once all lives are gone, the run moves into a short
            // dramatic pause (GAME_PLAYER_DEATH) rather than jumping
            // straight to GAME_OVER - see PLAYER_DEATH_DELAY_MS. The
            // high-score comparison itself doesn't happen until that
            // pause elapses (see the GAME_PLAYER_DEATH handling below,
            // after this block), not here.
            if (player.lives <= 0)
            {
                game_state = GAME_PLAYER_DEATH;
                player_death_started_at = game_ticks();

                // Cut any boss music immediately as the run leaves
                // active gameplay (v0.8.0 Phase 5B) - if this death
                // happened mid-Dreadnought-fight, Mountain King must
                // not keep playing through the death pause, GAME_OVER,
                // and beyond. A no-op the rest of the time, since
                // nothing plays during normal (non-boss) gameplay.
                audio_music_stop(&laser);

                // Wipe out any Scout projectiles still in flight.
                enemy_bullets_init(enemy_bullets);

                // A brief, subtle flash on top of the player-hit shake
                // already triggered by whichever collision caused this
                // - translucent, so it doesn't hide the destruction
                // effect playing out underneath it.
                screen_effects_flash(
                    &screen_effects,
                    PLAYER_DEATH_FLASH_R,
                    PLAYER_DEATH_FLASH_G,
                    PLAYER_DEATH_FLASH_B,
                    PLAYER_DEATH_FLASH_ALPHA,
                    PLAYER_DEATH_FLASH_DURATION_MS,
                    player_death_started_at
                );
            }

            // Once the key that started the game is finally released,
            // SPACE is free to mean "fire" again from here on.
            if (suppress_fire_until_space_released &&
                    !keyboard[SDL_SCANCODE_SPACE])
            {
                suppress_fire_until_space_released = 0;
            }

            // Fire continuously while the Space bar is held down -
            // unless this is still the same press that just started
            // the game.
            if (keyboard[SDL_SCANCODE_SPACE] &&
                    !suppress_fire_until_space_released)
            {
                Uint32 current_time = game_ticks();

                // Rapid Fire simply swaps in a shorter cooldown for as
                // long as it's active - bullets_fire() itself has no
                // idea power-ups exist.
                Uint32 fire_cooldown =
                    powerup_state.rapid_fire_active
                    ? RAPID_FIRE_COOLDOWN
                    : FIRE_COOLDOWN;

                // Rapid Fire and Spread Shot are independent: the
                // cooldown above applies either way, while this just
                // decides one straight bullet vs. a three-way burst.
                int fired;

                if (powerup_state.spread_shot_active)
                {
                    fired = bullets_fire_spread(
                                bullets,
                                &player,
                                current_time,
                                &last_shot_time,
                                fire_cooldown
                            );
                }
                else
                {
                    fired = bullets_fire(
                                bullets,
                                &player,
                                current_time,
                                &last_shot_time,
                                fire_cooldown
                            );
                }

                if (fired)
                {
                    audio_play_laser(&laser);
                }
            }

            bullets_update(bullets);
            enemy_bullets_update(enemy_bullets);

            // Player bullets vs. enemies.
            int points_earned = collisions_bullets_enemies(
                                     bullets,
                                     enemies,
                                     explosions,
                                     powerups,
                                     score_popups,
                                     game_ticks()
                                 );
            score += points_earned;

            if (points_earned > 0)
            {
                audio_play_enemy_explosion(&laser);
            }

            // Player ramming into enemies.
            int player_rammed = 0;
            int enemies_rammed = collisions_player_enemies(
                                      &player,
                                      enemies,
                                      explosions,
                                      powerups,
                                      &powerup_state,
                                      game_ticks(),
                                      &player_rammed
                                  );

            if (enemies_rammed > 0)
            {
                audio_play_enemy_explosion(&laser);
            }

            if (player_rammed)
            {
                audio_play_explosion(&laser);
                screen_effects_shake(
                    &screen_effects,
                    PLAYER_HIT_SHAKE_MAGNITUDE,
                    PLAYER_HIT_SHAKE_DURATION_MS,
                    game_ticks()
                );
            }

            // Player bullets vs. asteroids.
            int asteroid_points_earned = collisions_bullets_asteroids(
                                              bullets,
                                              asteroids,
                                              explosions,
                                              score_popups,
                                              game_ticks()
                                          );
            score += asteroid_points_earned;

            if (asteroid_points_earned > 0)
            {
                audio_play_enemy_explosion(&laser);
            }

            // Player bullets vs. the boss - works the same regardless
            // of whether it was a normal shot, Rapid Fire, or one of
            // Spread Shot's three pellets, since they're all just
            // Bullets from the same pool.
            collisions_bullets_boss(bullets, &boss, explosions, game_ticks());

            // Player ramming into asteroids.
            int player_hit_asteroid = 0;
            int asteroids_rammed = collisions_player_asteroids(
                                        &player,
                                        asteroids,
                                        explosions,
                                        &powerup_state,
                                        game_ticks(),
                                        &player_hit_asteroid
                                    );

            if (asteroids_rammed > 0)
            {
                audio_play_enemy_explosion(&laser);
            }

            if (player_hit_asteroid)
            {
                audio_play_explosion(&laser);
                screen_effects_shake(
                    &screen_effects,
                    PLAYER_HIT_SHAKE_MAGNITUDE,
                    PLAYER_HIT_SHAKE_DURATION_MS,
                    game_ticks()
                );
            }

            // Player ramming into the boss - damages the player only,
            // never the boss itself.
            if (collisions_player_boss(
                        &player,
                        &boss,
                        explosions,
                        &powerup_state,
                        game_ticks()))
            {
                audio_play_explosion(&laser);
                screen_effects_shake(
                    &screen_effects,
                    PLAYER_HIT_SHAKE_MAGNITUDE,
                    PLAYER_HIT_SHAKE_DURATION_MS,
                    game_ticks()
                );
            }

            // Enemy bullets vs. player.
            if (collisions_player_enemy_bullets(
                        &player,
                        enemy_bullets,
                        explosions,
                        &powerup_state,
                        game_ticks()))
            {
                audio_play_explosion(&laser);
                screen_effects_shake(
                    &screen_effects,
                    PLAYER_HIT_SHAKE_MAGNITUDE,
                    PLAYER_HIT_SHAKE_DURATION_MS,
                    game_ticks()
                );
            }

            Uint32 current_time = game_ticks();

            // The Wave Director decides which threats are active and
            // how aggressively they spawn; the enemy/asteroid modules
            // still do all the actual spawning.
            WaveDifficulty difficulty = wave_get_difficulty(&wave);

            // Trigger the warning sequence the first time we see a
            // boss wave while the boss is still inactive.
            // boss_begin_warning() immediately moves it out of
            // BOSS_STATE_INACTIVE, so this naturally only fires once
            // per encounter. boss_update() advances WARNING into the
            // actual boss_spawn() once its timer elapses (see
            // BOSS_WARNING_ENDED below).
            if (difficulty.boss_wave && boss.state == BOSS_STATE_INACTIVE)
            {
                boss_begin_warning(&boss, current_time);
                audio_set_boss_warning(&laser, 1);
            }

            if (difficulty.scouts_enabled)
            {
                enemies_spawn_scout(
                    enemies,
                    current_time,
                    &last_scout_spawn,
                    difficulty.scout_spawn_delay,
                    difficulty.scout_fire_delay
                );
            }

            // CRITICAL-phase support Scouts - a no-op outside phase 3,
            // and reuses the exact same Scout spawn function/pool as
            // the line above, so a support Scout is just a normal
            // enemy in every way once it exists.
            boss_spawn_support(&boss, enemies, current_time);

            if (difficulty.bombers_enabled)
            {
                enemies_spawn_bomber(
                    enemies,
                    current_time,
                    &last_bomber_spawn,
                    difficulty.bomber_spawn_delay,
                    difficulty.bomber_fire_delay
                );
            }

            // enemies_fire() reports back which weapon(s) actually went
            // off this frame, so we only ever play a sound for a shot
            // that really happened.
            int enemy_fire_result = enemies_fire(
                                         enemies,
                                         enemy_bullets,
                                         game_ticks()
                                     );

            if (enemy_fire_result & ENEMY_FIRED_BOLT)
            {
                audio_play_enemy_laser(&laser);
            }

            if (enemy_fire_result & ENEMY_FIRED_BOMB)
            {
                audio_play_bomb_drop(&laser);
            }

            // Boss weapon fire - reuses the same enemy bullet pool
            // and the Scout's laser sound, no new audio needed yet.
            if (boss_fire(&boss, enemy_bullets, game_ticks()))
            {
                audio_play_enemy_laser(&laser);
            }

            enemies_update(enemies);

            // Spawn and move asteroids alongside the regular enemies.
            if (difficulty.asteroids_enabled)
            {
                asteroids_spawn(
                    asteroids,
                    current_time,
                    &last_asteroid_spawn,
                    difficulty.asteroid_spawn_delay
                );
            }

            asteroids_update(asteroids);

            powerups_update(powerups);

            // Player collecting power-ups.
            if (collisions_player_powerups(
                        &player,
                        powerups,
                        &powerup_state,
                        explosions,
                        game_ticks()) > 0)
            {
                audio_play_pickup(&laser);
            }

            // Expire any timed effects whose duration has elapsed.
            powerup_state_update(&powerup_state, game_ticks());

            // Move the boss (entrance, then vertical bounce once in
            // combat), check phase transitions/defeat, and step
            // through the destruction sequence. A no-op while
            // inactive, so this is safe to call unconditionally -
            // only frozen because it's inside the GAME_PLAYING block,
            // same as every other system here.
            int boss_events = boss_update(&boss, explosions, game_ticks());

            if (boss_events & BOSS_JUST_DEFEATED)
            {
                // Cut Mountain King the instant the boss dies - do not
                // wait for the 27.8s loop to finish, and no fade (none
                // exists in this architecture). audio_music_stop()
                // only silences music; it doesn't touch the SFX played
                // just below for this same event.
                audio_music_stop(&laser);

                score += DREADNOUGHT_SCORE_VALUE;

                // Bigger, longer-lived popup than an ordinary kill -
                // the Dreadnought's payout should read as a much
                // bigger deal than a Scout's +10. Visualizes the exact
                // value just added above, never awards it separately.
                popups_spawn_emphasized(
                    score_popups,
                    boss.x + boss.width / 2.0f,
                    boss.y + boss.height / 2.0f,
                    DREADNOUGHT_SCORE_VALUE,
                    game_ticks()
                );
            }

            if (boss_events & BOSS_DEATH_SMALL_EXPLOSION)
            {
                audio_play_enemy_explosion(&laser);
            }

            if (boss_events & BOSS_DEATH_FINAL_EXPLOSION)
            {
                audio_play_explosion(&laser);

                // The strongest shake in the game so far - reserved
                // for the Dreadnought's own destruction so it reads
                // as clearly bigger than an ordinary player hit.
                screen_effects_shake(
                    &screen_effects,
                    BOSS_DEFEATED_SHAKE_MAGNITUDE,
                    BOSS_DEFEATED_SHAKE_DURATION_MS,
                    game_ticks()
                );
            }

            // The warning sequence just finished and the boss actually
            // spawned - stop the klaxon now that it's served its
            // purpose, and start the Dreadnought theme. BOSS_WARNING_ENDED
            // fires exactly once per encounter (boss_update() only sets
            // it on the single frame it advances WARNING -> ENTERING),
            // so this is a one-shot start, not a per-frame trigger. It's
            // deliberately not tied to the warning's own start (Blue
            // Danube is already stopped well before this point, at the
            // GAME_TITLE -> GAME_PLAYING transition, so there's nothing
            // underneath to silence) - starting once the boss actually
            // appears, rather than during the klaxon, avoids layering
            // the waltz theme's start under the warning siren.
            if (boss_events & BOSS_WARNING_ENDED)
            {
                audio_set_boss_warning(&laser, 0);

                audio_music_start_dual(
                    &laser,
                    MOUNTAIN_KING_MELODY, MOUNTAIN_KING_MELODY_COUNT, 1,
                    MOUNTAIN_KING_ACCOMPANIMENT, MOUNTAIN_KING_ACCOMPANIMENT_COUNT, 1
                );

                boss_music_tier = 1;
            }

            // Continuous HP-driven acceleration - boss.phase (1/2/3) is
            // already the game's one authoritative GUNSHIP/BARRAGE/
            // CRITICAL signal, computed from the exact same >66%/<=66%/
            // <=33% health thresholds this phase wants, so this reuses
            // it directly rather than tracking a second copy of the
            // boss's health percentage. Only calls the playback-rate
            // setter on an actual tier CHANGE (boss_music_tier differing
            // from boss.phase), never every frame - and, critically,
            // ONLY the rate setter: no start/restart function is called
            // here, so the shared MusicState's note_index/phase/
            // samples_into_note for both voices carry on completely
            // undisturbed, exactly where the performance already was.
            // boss.phase itself never regresses (see its own comment in
            // boss.c - no healing mechanic exists in this game), so
            // these tiers are naturally one-way as a consequence, not
            // because this code enforces it. A single big hit that
            // drops health from >66% straight past 33% still resolves
            // correctly: boss.c's phase check cascades 1->2->3 across
            // at most two consecutive frames (a fraction of a frame's
            // difference, inaudible as a distinct intermediate tempo),
            // and this code just mirrors whatever boss.phase says on
            // each of those frames, landing on the correct final 1.30x.
            if (boss.state == BOSS_STATE_ACTIVE && boss.phase != boss_music_tier)
            {
                boss_music_tier = boss.phase;

                double mountain_king_rate = 1.00;

                if (boss_music_tier == 2)
                {
                    mountain_king_rate = 1.15;
                }
                else if (boss_music_tier == 3)
                {
                    mountain_king_rate = 1.30;
                }

                audio_music_set_playback_rate(&laser, mountain_king_rate);
            }

            // Award extra lives for reaching score thresholds - checked
            // once per frame, after every source of score gain above
            // (enemy kills, asteroids, boss defeat) has had a chance to
            // run this frame, so nothing needs its own separate check.
            if (player_check_extra_life(&player, score, game_ticks()) > 0)
            {
                audio_play_extra_life(&laser);
            }

            // Once the defeat message has shown long enough, hand off
            // to the Wave Director and fully reset the boss - ready
            // for a hypothetical future encounter, and back to
            // BOSS_STATE_INACTIVE so its overlays stop drawing.
            if (boss_ready_for_wave_advance(&boss, game_ticks()))
            {
                wave_advance_after_boss(&wave, game_ticks());
                boss_init(&boss);
            }

            // Advance wave progression. Only ticks while playing, so
            // waves stay frozen during GAME_OVER like everything else.
            wave_update(&wave, game_ticks());

            // TEMPORARY DEBUG: press B during a boss wave to skip
            // straight past the fight instead of playing it out - the
            // real defeat path (Phase 9/10) handles this normally now,
            // this is just a fast-forward for testing. Resets the
            // boss too, so it's safe to press at any point mid-fight -
            // including mid-warning, which is why the klaxon is
            // explicitly stopped here too rather than assuming
            // BOSS_WARNING_ENDED already handled it - without leaving
            // a stale ENTERING/ACTIVE boss (or a stuck alarm) behind on
            // the next (non-boss) wave. Mountain King is stopped here
            // too (v0.8.0 Phase 5B) for the same reason - skipping the
            // fight this way never goes through BOSS_JUST_DEFEATED, so
            // without this, the boss theme would otherwise keep looping
            // straight into Wave 6. Remove before release.
            if (difficulty.boss_wave && keyboard[SDL_SCANCODE_B])
            {
                audio_set_boss_warning(&laser, 0);
                audio_music_stop(&laser);
                wave_advance_after_boss(&wave, game_ticks());
                boss_init(&boss);
            }

            // TEMPORARY DEBUG: press 5 to jump straight to Wave 5
            // instead of playing through 1-4 every time. Speeds up
            // testing the boss encounter. Remove before release.
            if (keyboard[SDL_SCANCODE_5])
            {
                wave_debug_jump(&wave, 5, game_ticks());
            }

        } // end GAME_PLAYING

        // Hand off from the death pause to the real GAME_OVER screen
        // once PLAYER_DEATH_DELAY_MS has elapsed. The high-score
        // comparison happens exactly here, not at the moment of the
        // fatal hit - score can no longer change by this point (every
        // scoring source for that final frame already ran before
        // game_state left GAME_PLAYING above, and nothing touches it
        // during GAME_PLAYER_DEATH), so there's no risk of comparing
        // against a value that's still about to go up. Strictly
        // greater-than: matching the existing record isn't good enough
        // to count as beating it.
        if (game_state == GAME_PLAYER_DEATH &&
                game_ticks() - player_death_started_at >= PLAYER_DEATH_DELAY_MS)
        {
            game_state = GAME_OVER;

            new_high_score = (score > high_score);

            if (new_high_score)
            {
                high_score = score;
                highscore_save(high_score);
                audio_play_new_high_score(&laser);

                // The Funeral March must not start underneath the
                // fanfare - wait for it to actually finish (checked
                // below, every frame) rather than starting it here.
                game_over_awaiting_fanfare = 1;
            }
            else
            {
                // No fanfare to wait for - the Funeral March is this
                // run's only Game Over audio, so it starts right away.
                // One-shot (loop=0): bar 2 already repeats bar 1
                // verbatim in the source, so looping this excerpt
                // would make the repetition too obvious for what's
                // meant to be a single Game Over cue (see Phase 6A/6B).
                // audio_music_start_dual() resets the shared playback
                // rate to 1.0 as part of this same locked call, so a
                // boss-fight death can never leave Mountain King's
                // 1.15x/1.30x rate active under the Funeral March.
                audio_music_start_dual(
                    &laser,
                    FUNERAL_MARCH_MELODY, FUNERAL_MARCH_MELODY_COUNT, 0,
                    FUNERAL_MARCH_ACCOMPANIMENT, FUNERAL_MARCH_ACCOMPANIMENT_COUNT, 0
                );
            }
        }

        // The fanfare finished naturally (not cancelled - ENTER's
        // handling below clears game_over_awaiting_fanfare itself, so
        // this can't also fire after that) - start the Funeral March
        // exactly once, the moment it's actually safe to, rather than
        // guessing at the fanfare's duration. Checked every frame
        // while GAME_OVER is waiting, but the flag itself is only ever
        // set once per run (immediately above) and cleared the first
        // time this fires, so audio_music_start_dual() below runs at
        // most once per Game Over screen.
        if (game_over_awaiting_fanfare &&
                audio_new_high_score_finished(&laser))
        {
            game_over_awaiting_fanfare = 0;

            audio_music_start_dual(
                &laser,
                FUNERAL_MARCH_MELODY, FUNERAL_MARCH_MELODY_COUNT, 0,
                FUNERAL_MARCH_ACCOMPANIMENT, FUNERAL_MARCH_ACCOMPANIMENT_COUNT, 0
            );
        }

        // Leave GAME_OVER back to the title screen - the arcade flow
        // is GAME_OVER -> GAME_TITLE -> (SPACE) -> a new game, not a
        // direct restart, so no other reset happens here.
        // game_start_new() only ever runs from the GAME_TITLE
        // transition below, the single place a run is allowed to
        // begin. screen_effects is the one exception: shake/flash are
        // applied unconditionally every frame - including the title
        // screen's own rendering - regardless of game_state, so this
        // guarantees a still-fading shake or flash can never bleed
        // into the title screen even if a future tuning change made
        // one outlast the death pause and Game Over screen combined.
        if (return_to_title_requested)
        {
            game_state = GAME_TITLE;
            screen_effects_init(&screen_effects);

            // Cancel every Game Over audio possibility at once (v0.8.0
            // Phase 6B) - whichever of these was actually true this
            // frame, ENTER must cut it cleanly: the fanfare still
            // playing, the Funeral March still playing, or the
            // Funeral March merely pending behind a fanfare that
            // hadn't finished yet. audio_music_stop() and
            // audio_stop_new_high_score() are two independent
            // subsystems (music voices vs. the fanfare's own fields),
            // so both are needed - stopping one never stops the other.
            audio_music_stop(&laser);
            audio_stop_new_high_score(&laser);
            game_over_awaiting_fanfare = 0;

            // Restart both title-theme voices together from the
            // beginning, sample-aligned, with the rate forced back to
            // 1.0 - a boss fight's tempo increase (see the Phase 2.5
            // playback-rate work) must never leak into the title
            // screen. audio_music_start_dual() clears both voices'
            // prior state (note index, phase, sample position) as part
            // of restarting them, so the four-beat accompaniment
            // silence plays again exactly as it did on first launch.
            audio_music_start_dual(
                &laser,
                BLUE_DANUBE_MELODY, BLUE_DANUBE_MELODY_COUNT, 1,
                BLUE_DANUBE_ACCOMPANIMENT, BLUE_DANUBE_ACCOMPANIMENT_COUNT, 1
            );
        }

        // Leave the title screen. Deliberately placed after the
        // GAME_PLAYING block above (rather than up in the event loop)
        // so this transition only takes effect starting next frame -
        // this frame already finished evaluating
        // "if (game_state == GAME_PLAYING)" against the old GAME_TITLE
        // value, so the fire-on-SPACE logic in that block cannot have
        // run yet this frame. suppress_fire_until_space_released then
        // covers every frame after that for as long as this same
        // press stays down.
        if (start_game_requested)
        {
            // Cut the title theme the instant gameplay begins - no
            // Blue Danube (melody or accompaniment) under gameplay,
            // and no fade-out in this phase. audio_music_stop() calls
            // music_stop_all(), which loops over every voice, so both
            // are silenced together. This only silences music; the SFX
            // mixing path in audio_callback() is untouched.
            audio_music_stop(&laser);

            game_start_new(
                &player,
                bullets,
                enemies,
                enemy_bullets,
                asteroids,
                powerups,
                &powerup_state,
                explosions,
                &wave,
                &boss,
                &last_shot_time,
                &last_scout_spawn,
                &last_bomber_spawn,
                &last_asteroid_spawn,
                &new_high_score,
                &screen_effects,
                score_popups,
                &laser,
                game_ticks()
            );

            game_state = GAME_PLAYING;
            suppress_fire_until_space_released = 1;
        }

        starfield_update(stars);

        // Keep any in-flight explosions animating, even after game over.
        explosions_update(explosions);

        // Same idea for score popups already in flight.
        popups_update(score_popups, game_ticks());

        // Set the drawing color to opaque black.
        // The four values represent Red, Green, Blue, and Alpha.
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        // Clear the previous frame using the current drawing color.
        // SDL_RenderClear() always clears the full render target,
        // ignoring whatever viewport is set below - so it's unaffected
        // by screen shake regardless of draw order.
        SDL_RenderClear(renderer);

        // Screen shake: SDL_RenderSetViewport() interprets its rect in
        // the same logical 160x120 coordinate space established by
        // SDL_RenderSetLogicalSize() at startup, so offsetting it here
        // shifts everything drawn for the rest of this frame by
        // exactly (shake_x, shake_y) logical pixels - no gameplay
        // struct's x/y is ever touched. Recomputed and set fresh every
        // frame (even when idle, where it's just (0, 0)), so there is
        // no stale-viewport state to accidentally carry into a frame
        // where nothing is shaking.
        int shake_x = 0;
        int shake_y = 0;

        screen_effects_get_shake_offset(
            &screen_effects,
            game_ticks(),
            &shake_x,
            &shake_y
        );

        SDL_Rect shake_viewport = { shake_x, shake_y, SCREEN_WIDTH, SCREEN_HEIGHT };
        SDL_RenderSetViewport(renderer, &shake_viewport);

        starfield_render(renderer, stars);

        // Title screen text - drawn over the moving starfield in
        // place of any gameplay entities. Every line is centered with
        // text_width(), the same "measure, then center" pattern
        // boss_render_health_bar() and wave_render_announcement()
        // already use, so nothing here hardcodes an x position for a
        // string of a specific length.
        if (game_state == GAME_TITLE)
        {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

            const char *title = "starfall";
            int title_scale = 2;
            int title_x = (SCREEN_WIDTH - text_width(title, title_scale)) / 2;
            text_draw(renderer, title, title_x, 16, title_scale);

            // High score label and value as two separate centered
            // lines - the value's width changes with its digit count
            // (0 vs. 99999 vs. a much bigger run), so it gets its own
            // text_width() call rather than assuming a fixed position
            // relative to the label above it.
            const char *high_score_label = "HIGH SCORE";
            int high_score_label_x =
                (SCREEN_WIDTH - text_width(high_score_label, 1)) / 2;
            text_draw(renderer, high_score_label, high_score_label_x, 36, 1);

            char high_score_text[16];
            snprintf(
                high_score_text,
                sizeof(high_score_text),
                "%d",
                high_score
            );
            int high_score_value_x =
                (SCREEN_WIDTH - text_width(high_score_text, 1)) / 2;
            text_draw(renderer, high_score_text, high_score_value_x, 46, 1);

            const char *prompt = "PRESS SPACE";
            int prompt_x = (SCREEN_WIDTH - text_width(prompt, 1)) / 2;
            text_draw(renderer, prompt, prompt_x, 62, 1);

            const char *controls_move = "ARROWS MOVE";
            int controls_move_x =
                (SCREEN_WIDTH - text_width(controls_move, 1)) / 2;
            text_draw(renderer, controls_move, controls_move_x, 92, 1);

            const char *controls_fire = "SPACE FIRE";
            int controls_fire_x =
                (SCREEN_WIDTH - text_width(controls_fire, 1)) / 2;
            text_draw(renderer, controls_fire, controls_fire_x, 102, 1);
        }

        // Everything below is gameplay presentation - the ship, HUD,
        // enemies, and every other entity pool. None of it should
        // appear on the title screen, which is just the starfield
        // (and, from Phase 2 on, the title text drawn further down).
        // GAME_OVER still draws through this block so the battlefield
        // stays visible (frozen) behind the "GAME OVER" text.
        if (game_state != GAME_TITLE)
        {

        // Build the HUD strings for this frame.
        char score_text[32];
        char lives_text[32];

        snprintf(
            score_text,
            sizeof(score_text),
            "SCORE %d",
            score
        );

        snprintf(
            lives_text,
            sizeof(lives_text),
            "LIVES %d",
            player.lives
        );

        // Draw HUD text in white.
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        text_draw(
            renderer,
            score_text,
            2,
            2,
            1
        );

        text_draw(
            renderer,
            lives_text,
            112,
            2,
            1
        );

        // Rapid Fire countdown indicator - only shown while the
        // effect is active, and disappears the instant it expires.
        // Sits in the dead space between the score and lives text
        // rather than growing the HUD strip.
        if (powerup_state.rapid_fire_active)
        {
            Uint8 rapid_r, rapid_g, rapid_b;
            powerup_color(POWERUP_RAPID_FIRE, &rapid_r, &rapid_g, &rapid_b);

            powerup_hud_bar_render(
                renderer,
                POWERUP_HUD_RAPID_X,
                POWERUP_HUD_Y,
                powerup_letter(POWERUP_RAPID_FIRE),
                rapid_r, rapid_g, rapid_b,
                powerup_time_remaining(powerup_state.rapid_fire_until, game_ticks()),
                RAPID_FIRE_DURATION,
                game_ticks()
            );
        }

        // Spread Shot countdown indicator - same layout as Rapid
        // Fire's, in its own reserved slot further right.
        if (powerup_state.spread_shot_active)
        {
            Uint8 spread_r, spread_g, spread_b;
            powerup_color(POWERUP_SPREAD_SHOT, &spread_r, &spread_g, &spread_b);

            powerup_hud_bar_render(
                renderer,
                POWERUP_HUD_SPREAD_X,
                POWERUP_HUD_Y,
                powerup_letter(POWERUP_SPREAD_SHOT),
                spread_r, spread_g, spread_b,
                powerup_time_remaining(powerup_state.spread_shot_until, game_ticks()),
                SPREAD_SHOT_DURATION,
                game_ticks()
            );
        }

        // Shield HUD indicator - just the letter, since Shield is a
        // charge rather than a timer, so there's no ratio to show a
        // bar for. The energy-field bubble around the ship (drawn
        // after player_render() below) is the primary "shield is
        // active" signal; this is a small supporting HUD cue.
        if (powerup_state.shield_active)
        {
            Uint8 shield_r, shield_g, shield_b;
            powerup_color(POWERUP_SHIELD, &shield_r, &shield_g, &shield_b);

            SDL_SetRenderDrawColor(renderer, shield_r, shield_g, shield_b, 255);
            text_draw_char(
                renderer,
                powerup_letter(POWERUP_SHIELD),
                POWERUP_HUD_SHIELD_X,
                POWERUP_HUD_Y,
                1
            );
        }

        // Draw a divider underneath the HUD.
        SDL_RenderDrawLine(
            renderer,
            0,
            HUD_HEIGHT + 0.5,
            SCREEN_WIDTH,
            HUD_HEIGHT + 0.5
        );

        bullets_render(renderer, bullets);
        enemies_render(renderer, enemies);
        asteroids_render(renderer, asteroids);
        powerups_render(renderer, powerups);
        enemy_bullets_render(renderer, enemy_bullets);
        boss_render(renderer, &boss);

        // Draw the player.
        player_render(renderer, &player);

        // Draw the Shield energy-field bubble around the ship while
        // the effect is active.
        if (powerup_state.shield_active)
        {
            powerup_render_shield(
                renderer,
                player.x,
                player.y,
                player.width,
                player.height
            );
        }

        // Draw any active explosions on top of everything else.
        explosions_render(renderer, explosions);

        // Floating "+value" score popups, drawn on top of explosions
        // so a popup is never hidden behind the burst it came from.
        popups_render(renderer, score_popups, game_ticks());

        // Boss health bar overlay - drawn on top of gameplay like the
        // wave announcement, not part of the permanently reserved HUD
        // strip. A no-op whenever no boss is present.
        boss_render_health_bar(renderer, &boss);

        // Defeat completion message - shows for WAVE_COMPLETE_DELAY_MS
        // after the destruction sequence finishes, then disappears
        // once main.c resets the boss back to inactive.
        boss_render_defeat_message(renderer, &boss);

        // Blinking "WARNING / DREADNOUGHT" announcement, shown only
        // during BOSS_STATE_WARNING, before the boss has actually
        // appeared.
        boss_render_warning(renderer, &boss);

        // Brief "EXTRA LIFE" readout - a no-op outside the short
        // window player_check_extra_life() just armed.
        player_render_extra_life_notification(renderer, &player);

        } // end "not GAME_TITLE" gameplay presentation block

        // Wave announcement overlay - drawn on top of gameplay, not
        // a separate frozen state. Gated to GAME_PLAYING/GAME_PAUSED
        // since wave_update() (which clears announcement_active)
        // doesn't run during GAME_OVER either - GAME_PAUSED is
        // included here (unlike most GAME_PLAYING-only gameplay logic)
        // so an announcement that was on screen when the player paused
        // stays visible instead of blinking out and back in around the
        // pause. Also suppressed during BOSS_STATE_WARNING - both this
        // and boss_render_warning() start at the same instant Wave 5
        // begins and run for the same ~1.8s, so showing both at once
        // would just overlap into unreadable text; the boss-specific
        // warning is the more dramatic and more specific of the two,
        // so it wins.
        if ((game_state == GAME_PLAYING || game_state == GAME_PAUSED) &&
                boss.state != BOSS_STATE_WARNING)
        {
            wave_render_announcement(renderer, &wave);
        }

        // PAUSED overlay - drawn on top of the frozen battlefield the
        // same way GAME_OVER's text is, so it's obvious the game has
        // stopped responding to gameplay input on purpose rather than
        // hung.
        if (game_state == GAME_PAUSED)
        {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

            const char *paused_title = "PAUSED";
            int paused_scale = 2;
            int paused_x =
                (SCREEN_WIDTH - text_width(paused_title, paused_scale)) / 2;
            text_draw(renderer, paused_title, paused_x, 50, paused_scale);
        }

        if (game_state == GAME_OVER)
        {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

            const char *game_over_title = "GAME OVER";
            int game_over_scale = 2;
            int game_over_x =
                (SCREEN_WIDTH - text_width(game_over_title, game_over_scale)) / 2;
            text_draw(renderer, game_over_title, game_over_x, 30, game_over_scale);

            // Final score and high score both need their own
            // text_width() call rather than a shared/hardcoded x -
            // "SCORE 40" and "SCORE 142999" don't take up the same
            // width, and the two lines can each be centered
            // independently of each other.
            char final_score_text[32];
            snprintf(
                final_score_text,
                sizeof(final_score_text),
                "SCORE %d",
                score
            );
            int final_score_x =
                (SCREEN_WIDTH - text_width(final_score_text, 1)) / 2;
            text_draw(renderer, final_score_text, final_score_x, 52, 1);

            // In the normal case this is "HIGH SCORE <n>", same as
            // before. On a new record it becomes "NEW HIGH SCORE!"
            // instead - score above already shows the value, which
            // now equals high_score, so repeating the number a
            // second time on this line would be redundant rather
            // than more informative.
            char high_score_line_text[32];

            if (new_high_score)
            {
                snprintf(
                    high_score_line_text,
                    sizeof(high_score_line_text),
                    "NEW HIGH SCORE!"
                );
            }
            else
            {
                snprintf(
                    high_score_line_text,
                    sizeof(high_score_line_text),
                    "HIGH SCORE %d",
                    high_score
                );
            }

            int high_score_line_x =
                (SCREEN_WIDTH - text_width(high_score_line_text, 1)) / 2;
            text_draw(renderer, high_score_line_text, high_score_line_x, 62, 1);

            const char *continue_prompt = "PRESS ENTER";
            int continue_prompt_x =
                (SCREEN_WIDTH - text_width(continue_prompt, 1)) / 2;
            text_draw(renderer, continue_prompt, continue_prompt_x, 82, 1);
        }

        // Full-screen flash overlay, if one is active - drawn last so
        // it tints everything else already on screen this frame. A
        // no-op the rest of the time.
        screen_effects_render_flash(renderer, &screen_effects, game_ticks());

        // Display the completed frame in the window.
        SDL_RenderPresent(renderer);
        // Calculate how long this frame took to process.
        Uint32 frame_time = SDL_GetTicks() - frame_start;

        // If the frame finished early, wait for the remaining time.
        if (frame_time < FRAME_TIME)
        {
            SDL_Delay(FRAME_TIME - frame_time);
        }

    } // end of game loop

    // -------------------------
    // Cleanup
    // -------------------------

    // Destroy resources in reverse order of their creation.
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    // Shut down all initialized SDL systems.
    SDL_Quit();

    // Returning 0 tells the operating system that the program
    // finished successfully.
    return 0;

} // end of main
