#ifndef BOSS_H
#define BOSS_H

#include <SDL.h>

#include "enemy.h"
#include "enemy_bullet.h"
#include "explosion.h"

// Only one boss type exists right now, but this is what lets a future
// Dreadnought Mk II or an entirely different boss be added later
// without reshaping the Boss struct or teaching the Wave Director
// anything new - wave.c only ever needs to know "this wave has a
// boss", not which one.
typedef enum
{
    BOSS_DREADNOUGHT

} BossType;

// Tracks a boss encounter's lifecycle. Entrance and destruction both
// need to play out over several frames without combat logic
// interrupting them, so they're distinct states rather than boolean
// flags bolted onto "is active".
typedef enum
{
    BOSS_STATE_INACTIVE, // No boss encounter running.
    BOSS_STATE_WARNING,  // "WARNING / DREADNOUGHT" building tension before it appears.
    BOSS_STATE_ENTERING, // Sliding in from the right edge.
    BOSS_STATE_ACTIVE,   // In combat - can move, fire, and take damage.
    BOSS_STATE_DYING,    // Destruction sequence playing; can't fire or hurt the player.
    BOSS_STATE_DEFEATED  // Destruction sequence finished; Wave Director can advance.

} BossState;

// Starting stats, each its own #define so they're easy to retune
// after playtesting. Sized relative to the existing 1-damage-per-hit
// convention every player bullet already uses (Scout = 1 HP,
// Bomber = 3 HP) - 150 HP means roughly 22 seconds of sustained
// accurate fire at the normal fire rate, before Rapid Fire/Spread
// Shot/misses/dodging change that.
#define DREADNOUGHT_MAX_HEALTH 150
#define DREADNOUGHT_WIDTH 40
#define DREADNOUGHT_HEIGHT 32

// Where the boss comes to rest once its entrance finishes, and the
// firing cadence used before Phase 5 adds per-phase pacing.
#define DREADNOUGHT_COMBAT_X 110.0f
#define DREADNOUGHT_FIRE_DELAY 1200

// BARRAGE (Phase 2, <=66% HP): a bit faster on both counts than
// GUNSHIP. Later phases may push these further.
#define DREADNOUGHT_PHASE2_MOVE_SPEED 0.45f
#define DREADNOUGHT_PHASE2_FIRE_DELAY 800

// Vertical spread of the two angled bolts in a spread attack.
#define DREADNOUGHT_SPREAD_DY 1.0f

// CRITICAL (Phase 3, <=33% HP): faster still than BARRAGE.
#define DREADNOUGHT_PHASE3_MOVE_SPEED 0.65f
#define DREADNOUGHT_PHASE3_FIRE_DELAY 550

// How often CRITICAL calls in a support Scout, and how forgiving that
// Scout's own fire rate is - it's backup, not another full threat.
// enemies_spawn_scout() already enforces MIN_SCOUT_SPAWN_DELAY and
// respects the shared MAX_ENEMIES pool cap, so support Scouts can
// never bypass either safety net.
#define DREADNOUGHT_SUPPORT_SPAWN_DELAY 9000
#define DREADNOUGHT_SUPPORT_FIRE_DELAY 1200

// How long the boss flashes white after a phase transition, and how
// big the accompanying burst from the shared explosion pool is - both
// noticeably smaller than the eventual destruction sequence, since
// this is a transition, not a kill.
#define PHASE_TRANSITION_FLASH_MS 400
#define PHASE_TRANSITION_EXPLOSION_PARTICLES 20

// Ongoing cosmetic battle damage from Phase 2 onward - small
// spark/smoke bursts at a random spot on the hull, on their own
// independent timer rather than a per-frame random roll (matching how
// every other timed event in this project already works). CRITICAL
// fires these noticeably more often than BARRAGE does, so the ship
// visibly reads as more damaged in the final phase. Purely cosmetic -
// boss.c's phase system is still the only thing that actually changes
// health, speed, or fire rate.
#define DREADNOUGHT_DAMAGE_EFFECT_PARTICLES 4
#define DREADNOUGHT_PHASE2_DAMAGE_EFFECT_INTERVAL_MS 1500
#define DREADNOUGHT_PHASE3_DAMAGE_EFFECT_INTERVAL_MS 600

// How long BOSS_STATE_WARNING lasts before the boss actually spawns -
// long enough to build tension, short enough not to feel like a wait.
#define BOSS_WARNING_DURATION_MS 1800

// Score awarded once for defeating the Dreadnought - substantially
// more than Scout (10), Bomber (30), or asteroids (20/50), matching
// how much bigger the encounter is. Easy to retune after playtesting.
#define DREADNOUGHT_SCORE_VALUE 500

// The destruction sequence: a handful of small explosions staggered
// across the hull, then one large final explosion, all reusing the
// existing shared ExplosionParticle pool - no new particle system.
#define DREADNOUGHT_DEATH_EXPLOSION_COUNT 5
#define DREADNOUGHT_DEATH_EXPLOSION_INTERVAL_MS 200
#define DREADNOUGHT_DEATH_SMALL_EXPLOSION_PARTICLES 14
#define DREADNOUGHT_DEATH_FINAL_EXPLOSION_PARTICLES 60

// Bits returned by boss_update() so the caller knows what happened
// this frame - same idea as enemies_fire()'s ENEMY_FIRED_* bits. More
// than one can be set at once.
#define BOSS_JUST_DEFEATED 0x1         // Award DREADNOUGHT_SCORE_VALUE now.
#define BOSS_DEATH_SMALL_EXPLOSION 0x2 // Play the short destruction sound.
#define BOSS_DEATH_FINAL_EXPLOSION 0x4 // Play the long destruction sound.
#define BOSS_WARNING_ENDED 0x8         // Stop the warning alarm now - the boss just spawned.

// The boss health bar overlay (see boss_render_health_bar() in
// boss.c) sits at the top of the play area, so during a boss fight
// both the boss and the player need to stay below it instead of the
// normal HUD_HEIGHT - otherwise either one can fly up behind the bar,
// which looks wrong. Keep this in sync with the bar's own Y position
// in boss.c if that ever moves.
#define BOSS_FIGHT_TOP_BOUNDARY 28

// A single boss encounter. Only one is ever active at a time, so this
// is one plain instance rather than an object pool - no dynamic
// allocation needed anywhere in this module.
typedef struct
{
    BossType type;
    BossState state;

    float x;
    float y;

    // Current movement velocity. Horizontal only really matters
    // during the entrance; once in combat the boss holds its ground
    // horizontally and just drifts vertically.
    float dx;
    float dy;

    int width;
    int height;

    int health;
    int max_health;

    // 1, 2, or 3 - GUNSHIP/BARRAGE/CRITICAL. Driven by health
    // percentage rather than a timer once that logic is added.
    int phase;

    // The x position the boss settles at once ENTERING finishes.
    float combat_x;

    Uint32 last_shot_time;
    Uint32 fire_delay;

    // Counts every shot fired so far, used to alternate between
    // attack patterns once a phase introduces more than one (e.g.
    // BARRAGE alternating straight and spread attacks).
    int attack_index;

    // While current time is before this, boss_render() flashes the
    // ship white - the phase-transition feedback. 0 means "not
    // currently flashing".
    Uint32 phase_flash_until;

    // Same idea, but set by collision.c on every ordinary bullet hit
    // that doesn't itself trigger a phase transition - much shorter
    // than phase_flash_until so sustained Rapid Fire/Spread Shot
    // reads as a rapid flicker of hits landing, never a permanently
    // solid-white boss. boss_render() flashes white while either
    // timer is still active. 0 means "not currently flashing".
    Uint32 hit_flash_until;

    // Own independent cooldown for CRITICAL's support Scout calls,
    // separate from the boss's own weapon timer.
    Uint32 last_support_spawn;

    // Destruction sequence progress while BOSS_STATE_DYING - how many
    // of the staggered small explosions have played, and when the
    // next explosion (small, or the final big one once the count is
    // reached) should fire.
    int death_explosions_fired;
    Uint32 next_death_explosion_time;

    // When BOSS_STATE_DEFEATED was reached - lets the caller wait a
    // beat (see boss_ready_for_wave_advance()) before handing off to
    // the Wave Director, so the defeat has a moment to land instead
    // of cutting straight to the next wave.
    Uint32 defeated_at;

    // When BOSS_STATE_WARNING began - boss_update() compares this
    // against BOSS_WARNING_DURATION_MS to know when to actually spawn
    // the boss.
    Uint32 warning_started_at;

    // When the next Phase 2+ cosmetic damage spark/smoke burst is
    // allowed to fire. Unused (never checked) during Phase 1.
    Uint32 next_damage_effect_time;

} Boss;

// How long to show the defeat message before advancing to the next
// wave, once BOSS_STATE_DEFEATED is reached.
#define WAVE_COMPLETE_DELAY_MS 1500

// Reset the boss to BOSS_STATE_INACTIVE. Used at game start and on
// restart, matching every other pool/state struct in this project
// (enemies_init(), powerup_state_init(), etc.) - none of those need a
// timestamp just to go idle either.
void boss_init(Boss *boss);

// Begin the warning sequence that precedes a new Dreadnought
// encounter - BOSS_STATE_WARNING, not ENTERING yet. The caller decides
// when this should happen (main.c, once it sees the current wave is a
// boss wave and the boss is currently inactive) - boss.c doesn't know
// about waves. boss_update() advances WARNING into the actual
// boss_spawn() once BOSS_WARNING_DURATION_MS has elapsed.
void boss_begin_warning(Boss *boss, Uint32 current_time);

// Begin a new Dreadnought encounter, entering from just beyond the
// right edge of the screen. Normally called internally by
// boss_update() once BOSS_STATE_WARNING finishes, but exposed here too
// since nothing about it actually depends on having gone through a
// warning first.
void boss_spawn(Boss *boss, Uint32 current_time);

// Advance BOSS_STATE_WARNING once its timer elapses (spawning the boss
// and setting BOSS_WARNING_ENDED so the caller can stop the warning
// alarm), move the boss, handle its ENTERING -> ACTIVE transition,
// check for phase transitions (health percentage, not a timer), detect
// defeat (health <= 0, exactly once - the state change out of ACTIVE
// prevents it from re-triggering), and step through the destruction
// sequence while BOSS_STATE_DYING. Phase transitions and destruction
// explosions are both spawned from the shared explosion pool directly
// inside boss.c - the caller doesn't need to know why an explosion
// happened, only whether to play a sound (see the BOSS_* return bits).
// A no-op while BOSS_STATE_INACTIVE or BOSS_STATE_DEFEATED.
// Returns a combination of the BOSS_* bits above, or 0 if nothing
// notable happened this call.
int boss_update(
    Boss *boss,
    ExplosionParticle explosions[],
    Uint32 current_time
);

// Fire from the boss's nose cannon if its own cooldown has expired.
// Reuses the existing enemy bullet pool/system (the same straight
// bolt a Scout fires) rather than a boss-specific projectile type, so
// boss shots automatically go through the existing
// collisions_player_enemy_bullets() -> player_take_damage() path -
// Shield and invulnerability both work without any new code. A no-op
// outside BOSS_STATE_ACTIVE.
// Returns 1 if a shot was actually fired, 0 otherwise.
int boss_fire(
    Boss *boss,
    EnemyBullet bullets[],
    Uint32 current_time
);

// Occasionally bring in a Scout as backup once the boss reaches
// CRITICAL (phase 3). Reuses enemies_spawn_scout() entirely - the
// resulting Scout is a completely normal Enemy, going through the
// exact same collision, scoring, and power-up-drop paths as any other
// Scout, since there's no such thing as a "boss-flagged" enemy. A
// no-op outside phase 3 or BOSS_STATE_ACTIVE.
void boss_spawn_support(
    Boss *boss,
    Enemy enemies[],
    Uint32 current_time
);

// True once the boss has been BOSS_STATE_DEFEATED for at least
// WAVE_COMPLETE_DELAY_MS - the caller's cue to call
// wave_advance_after_boss() and then boss_init() to fully reset for
// a hypothetical future boss encounter. False the rest of the time,
// including before defeat, so it's safe to check every frame.
int boss_ready_for_wave_advance(const Boss *boss, Uint32 current_time);

// Draw the boss. A no-op while BOSS_STATE_INACTIVE or
// BOSS_STATE_WARNING (nothing has actually appeared on screen yet), so
// it's safe to call unconditionally every frame - the same convention
// as wave_render_announcement().
void boss_render(SDL_Renderer *renderer, const Boss *boss);

// Draw the "DREADNOUGHT" label and a shrinking health bar as an
// overlay near the top of the play area while the boss is present.
// Drawn as an overlay (like wave_render_announcement()) rather than
// growing the permanently-reserved HUD strip, since it only matters
// for the duration of a boss fight. A no-op while BOSS_STATE_INACTIVE
// or BOSS_STATE_WARNING - a health bar showing 100% before the boss
// has even appeared would be misleading.
void boss_render_health_bar(SDL_Renderer *renderer, const Boss *boss);

// Draw the blinking "WARNING" / "DREADNOUGHT" announcement while
// BOSS_STATE_WARNING is active. A no-op otherwise, so it's safe to
// call unconditionally every frame.
void boss_render_warning(SDL_Renderer *renderer, const Boss *boss);

// Draw the "DREADNOUGHT / DESTROYED" completion message while waiting
// out WAVE_COMPLETE_DELAY_MS after defeat. A no-op outside
// BOSS_STATE_DEFEATED, so it naturally disappears the moment the
// caller resets the boss back to inactive.
void boss_render_defeat_message(SDL_Renderer *renderer, const Boss *boss);

#endif
