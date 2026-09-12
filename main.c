// SDL port of my original Pyxel/Python space shooter.
#include <stdio.h>  // printf() for console output and error messages
#include <SDL.h>    // SDL2 functions, types, and constants

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
#include "game_config.h"
#include "highscore.h"
#include "screen_effects.h"
#include "popup.h"
#include "soundtrack.h"
#include "game_render.h"

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

// Main-owned bookkeeping for the current run and the game's state
// machine - consolidated here (v0.8.0 main.c refactor, Phase 3) so
// every piece of "what state is this run in, and since when" lives in
// one place instead of being scattered across file-scope statics and
// main()-local variables. Deliberately NOT a "God struct": entity
// pools (player, enemies, bullets, ...) and the per-subsystem spawn
// timers/last_shot_time declared in main() below stay OUTSIDE this
// struct - they're mutated directly by the systems they coordinate
// (bullet.c, enemy.c, asteroid.c), so they belong beside those systems
// rather than with the state machine that just decides WHEN those
// systems should run.
typedef struct
{
    // The state the game is currently in - see the GameState enum in
    // game_render.h (which has to branch on it too, to decide what a
    // given frame should show) for what each value means.
    GameState game_state;

    // How long the game has spent paused so far, in total, folded in
    // only once a pause ends (see the P key handling below) - NOT
    // updated while a pause is still in progress. Every gameplay timer
    // in this file is a Uint32 timestamp compared against "now" via
    // game_ticks() below rather than raw SDL_GetTicks() - spawn
    // delays, wave duration, invulnerability, boss timers, popups,
    // screen effects, all of it - so pausing correctly freezes every
    // one of them at once instead of only skipping the frame-by-frame
    // update loop while their underlying timestamps keep quietly
    // falling behind the wall clock. Without this, resuming from a
    // pause would make every "time since X" check see a sudden jump
    // equal to however long the game was paused, which reads as a
    // burst of enemies spawning all at once, a wave ending early, or
    // invulnerability/power-ups expiring instantly.
    Uint32 pause_offset_ms;

    // Whether a pause is currently in progress, and the frozen
    // game_ticks() value to keep returning for as long as it is. This
    // pair exists because pause_offset_ms above is only updated at the
    // MOMENT a pause ends - while the pause is still ongoing,
    // SDL_GetTicks() keeps climbing every real frame, so without
    // freezing the return value directly here, anything that calls
    // game_ticks() outside the GAME_PLAYING-gated update block
    // (screen_effects_get_shake_offset() and
    // screen_effects_render_flash(), both called every frame
    // regardless of state) would keep seeing time pass - a shake or
    // flash would silently keep animating behind the "PAUSED" text.
    int is_paused;
    Uint32 paused_at_tick;

    // When the current pause began (raw wall-clock time, not
    // game_ticks() - this is the one timestamp that must NOT be offset
    // by pause_offset_ms, since it's what that offset gets computed
    // from on unpause). Only meaningful while game_state == GAME_PAUSED.
    Uint32 pause_started_at;

    // When GAME_PLAYER_DEATH began - compared against
    // PLAYER_DEATH_DELAY_MS to know when to actually hand off to
    // GAME_OVER. Always freshly set at the moment death is detected,
    // so there's no stale-value risk from a previous run.
    Uint32 player_death_started_at;

    // Set whenever a game starts while SPACE is still physically held
    // down (the same press used to leave the title screen), and held
    // until that key is released. A human press easily spans several
    // frames, so skipping the fire logic on just the transition frame
    // (see start_game_requested below) isn't enough on its own - this
    // covers every frame the original press is still down, however
    // long that turns out to be, and asks nothing of how long a press
    // actually lasts.
    int suppress_fire_until_space_released;

    // Which Mountain King intensity tier the boss music is currently
    // playing at (1/2/3, matching boss.phase exactly - see the
    // BOSS_STATE_ACTIVE tier check below). Tracked separately from
    // boss.phase itself so the music's playback-rate setter is only
    // called on an actual tier CHANGE, not every frame - reset to 1
    // whenever Mountain King (re)starts, alongside boss.phase's own
    // reset to 1 in boss_init()/boss_spawn(), so a fresh encounter
    // never begins mid-tier from a previous fight.
    int boss_music_tier;

    // Set at most once per run, at the GAME_PLAYER_DEATH -> GAME_OVER
    // handoff - never re-evaluated on later frames while GAME_OVER is
    // just sitting on screen. Read by the GAME_OVER render block to
    // decide whether to show "NEW HIGH SCORE!" instead of the normal
    // "HIGH SCORE <n>" line.
    int new_high_score;

    // True while GAME_OVER is waiting for the new-high-score fanfare
    // to actually finish before starting the Funeral March (v0.8.0
    // Phase 6B) - set only at the GAME_PLAYER_DEATH -> GAME_OVER
    // handoff below when this run set a new record, and cleared either
    // once audio_new_high_score_finished() reports true (the normal
    // path) or by ENTER cancelling Game Over audio early. Never set
    // when there's no fanfare to wait for, so the Funeral March starts
    // immediately in that case instead.
    int game_over_awaiting_fanfare;

    // The current run's score. Was a plain file-scope global before
    // this phase; moved here since nothing outside main.c ever reads
    // or writes it directly - every other module either receives it
    // as a parameter (e.g. player_check_extra_life()) or returns
    // points earned for main.c to add (e.g. every collisions_*()
    // function), confirmed by an audit across the whole project during
    // this phase.
    int score;

} GameRuntime;

// "Now", per every gameplay timer in this file - see GameRuntime's
// pause_offset_ms/is_paused/paused_at_tick fields above for why this
// exists instead of calling SDL_GetTicks() directly. The one exception
// is the frame-rate cap at the bottom of the main loop, which measures
// real wall-clock frame duration and must not be shifted or frozen by
// this. Takes runtime explicitly (rather than reaching into a file-scope
// static, as the pre-Phase-3 version of this function did) so the
// dependency is visible at every call site instead of hidden.
static Uint32 game_ticks(const GameRuntime *runtime)
{
    if (runtime->is_paused)
    {
        return runtime->paused_at_tick;
    }

    return SDL_GetTicks() - runtime->pause_offset_ms;
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
// Only resets the two GameRuntime fields a fresh run actually needs to
// start clean (score, new_high_score) - every other runtime field
// (pause bookkeeping, boss music tier, the fanfare-wait flag) is either
// already idle between runs or gets set fresh by whatever transition
// leads into gameplay, so resetting them here too would be redundant,
// not safer.
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
    GameRuntime *runtime,
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
    runtime->new_high_score = 0;

    runtime->score = 0;
}

// Enter GAME_PAUSED from GAME_PLAYING. Only ever called while that's
// the current state (see toggle_pause() below) - freezes game_ticks()
// at exactly its last real reading and records the raw wall-clock
// moment the pause began, for runtime_end_pause() to measure against.
static void runtime_begin_pause(GameRuntime *runtime)
{
    // Capture the frozen game_ticks() value BEFORE flipping is_paused
    // on, since game_ticks() itself checks that flag - this has to
    // read the last real (unpaused) reading, not the frozen one it's
    // about to start returning.
    runtime->paused_at_tick = game_ticks(runtime);
    runtime->is_paused = 1;

    runtime->game_state = GAME_PAUSED;

    // Raw SDL_GetTicks(), not game_ticks() - see pause_started_at's
    // field comment on GameRuntime.
    runtime->pause_started_at = SDL_GetTicks();
}

// Leave GAME_PAUSED back to GAME_PLAYING. Only ever called while
// that's the current state (see toggle_pause() below).
static void runtime_end_pause(GameRuntime *runtime)
{
    // Fold however long this pause lasted into the running offset so
    // game_ticks() resumes from exactly paused_at_tick with no jump,
    // the instant is_paused clears below.
    runtime->pause_offset_ms += SDL_GetTicks() - runtime->pause_started_at;
    runtime->is_paused = 0;

    runtime->game_state = GAME_PLAYING;
}

// P toggles pause, but only while actually playing - pausing during
// the title screen, the death pause, or Game Over wouldn't mean
// anything (nothing is running to freeze), so it's deliberately a
// no-op there rather than a state jump to somewhere the rest of the
// game doesn't expect.
static void toggle_pause(GameRuntime *runtime)
{
    if (runtime->game_state == GAME_PLAYING)
    {
        runtime_begin_pause(runtime);
    }
    else if (runtime->game_state == GAME_PAUSED)
    {
        runtime_end_pause(runtime);
    }
}

// Polls every SDL event queued this frame and reacts only to the ones
// the state machine cares about: window close, and discrete
// (non-repeat) SPACE/ENTER/P key-downs. Deliberately does not perform
// any gameplay update beyond the pause toggle itself - starting a new
// game or leaving GAME_OVER are only requested here, via the two
// out-parameters, never actually carried out. That mirrors the
// original inline loop's own ordering: main()'s caller still applies
// those two requests later in the same frame, after gameplay
// processing has already run against the state as it was at the start
// of the frame - see the call site for why that ordering matters.
// Continuous keyboard state (SDL_GetKeyboardState(), used for firing
// and pause-time movement) is a completely separate concern and is
// never touched here.
static void process_input_events(
    GameRuntime *runtime,
    int *running,
    int *start_game_requested,
    int *return_to_title_requested
)
{
    SDL_Event event;

    // &event passes the address of our event variable so that
    // SDL_PollEvent() can write event information into it.
    while (SDL_PollEvent(&event))
    {
        // SDL_QUIT occurs when the user requests that the
        // application close, such as clicking the window's X button.
        if (event.type == SDL_QUIT)
        {
            *running = 0;
        }

        // event.key.repeat is nonzero for the auto-repeated key-down
        // events SDL sends while a key stays held - checking for it
        // being 0 means this only latches once per physical press.
        if (event.type == SDL_KEYDOWN && !event.key.repeat)
        {
            if (event.key.keysym.scancode == SDL_SCANCODE_SPACE &&
                    runtime->game_state == GAME_TITLE)
            {
                *start_game_requested = 1;
            }

            if (event.key.keysym.scancode == SDL_SCANCODE_RETURN &&
                    runtime->game_state == GAME_OVER)
            {
                *return_to_title_requested = 1;
            }

            if (event.key.keysym.scancode == SDL_SCANCODE_P)
            {
                toggle_pause(runtime);
            }
        }
    }
}

// GAME_OVER -> GAME_TITLE, on ENTER. Cancels every Game Over audio
// possibility at once - whichever of these was actually true this
// frame, ENTER must cut it cleanly: the fanfare still playing, the
// Funeral March still playing, or the Funeral March merely pending
// behind a fanfare that hadn't finished yet. soundtrack_stop() and
// audio_stop_new_high_score() are two independent subsystems (music
// voices vs. the fanfare's own fields), so both are needed - stopping
// one never stops the other. screen_effects is reset unconditionally
// too (not just on game_start_new()), so a still-fading shake or
// flash can never bleed into the title screen.
static void transition_game_over_to_title(
    GameRuntime *runtime,
    ScreenEffects *screen_effects,
    LaserSound *laser
)
{
    runtime->game_state = GAME_TITLE;
    screen_effects_init(screen_effects);

    soundtrack_stop(laser);
    audio_stop_new_high_score(laser);
    runtime->game_over_awaiting_fanfare = 0;

    // Restart the title theme from the beginning, sample-aligned, with
    // the rate forced back to 1.0 - a boss fight's tempo increase must
    // never leak into the title screen.
    soundtrack_play_title(laser);
}

// GAME_TITLE -> GAME_PLAYING, on SPACE. Cuts the title theme, resets
// every piece of gameplay state via game_start_new() (the one place
// both this transition and a hypothetical future restart path go
// through), then enters GAME_PLAYING with fire suppressed for as long
// as the SPACE press that started this run stays down. Takes the same
// parameters as game_start_new() (minus current_time, computed here
// via game_ticks()) since that's nearly this entire function's job.
static void transition_title_to_playing(
    GameRuntime *runtime,
    LaserSound *laser,
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
    ScreenEffects *screen_effects,
    ScorePopup score_popups[]
)
{
    // Cut the title theme the instant gameplay begins - no Blue Danube
    // (melody or accompaniment) under gameplay, and no fade-out in
    // this phase. soundtrack_stop() silences every music voice
    // together; the SFX mixing path in audio_callback() is untouched.
    soundtrack_stop(laser);

    game_start_new(
        player,
        bullets,
        enemies,
        enemy_bullets,
        asteroids,
        powerups,
        powerup_state,
        explosions,
        wave,
        boss,
        last_shot_time,
        last_scout_spawn,
        last_bomber_spawn,
        last_asteroid_spawn,
        runtime,
        screen_effects,
        score_popups,
        laser,
        game_ticks(runtime)
    );

    runtime->game_state = GAME_PLAYING;
    runtime->suppress_fire_until_space_released = 1;
}

// Player movement and death detection - the first thing every
// GAME_PLAYING frame does. Order matters: this runs BEFORE this
// frame's own collisions (see update_projectiles_and_collisions()
// below), so a hit that brings lives to exactly 0 is only detected
// here on the NEXT frame, not the same frame it happened - preserved
// exactly as the original inline code behaved. Also note: setting
// game_state to GAME_PLAYER_DEATH here does NOT skip the rest of this
// frame's GAME_PLAYING stages below - main()'s "if (game_state ==
// GAME_PLAYING)" branch was already entered for this frame, so firing,
// collisions, spawning, and everything else still runs once more even
// on the exact frame death is detected. That was already true of the
// original inline code and is unchanged here.
static void update_player_movement(
    GameRuntime *runtime,
    Player *player,
    const Uint8 *keyboard,
    const Boss *boss,
    EnemyBullet enemy_bullets[],
    LaserSound *laser,
    ScreenEffects *screen_effects
)
{
    // One snapshot for the whole function - safe because nothing in
    // this function's own call chain can toggle the pause state (that
    // only ever happens in process_input_events(), earlier in the
    // frame), so every game_ticks(runtime) call here would have
    // returned the same value anyway.
    Uint32 now = game_ticks(runtime);

    // Stay below the boss health bar overlay during a boss
    // fight instead of the normal (shorter) HUD strip, so the
    // player can't fly up behind it.
    int player_min_y =
        (boss->state != BOSS_STATE_INACTIVE)
        ? BOSS_FIGHT_TOP_BOUNDARY
        : HUD_HEIGHT;

    player_update(
        player,
        keyboard,
        now,
        player_min_y
    );

    // Once all lives are gone, the run moves into a short
    // dramatic pause (GAME_PLAYER_DEATH) rather than jumping
    // straight to GAME_OVER - see PLAYER_DEATH_DELAY_MS. The
    // high-score comparison itself doesn't happen until that
    // pause elapses (see the GAME_PLAYER_DEATH handling in
    // main()'s loop, after GAME_PLAYING), not here.
    if (player->lives <= 0)
    {
        runtime->game_state = GAME_PLAYER_DEATH;
        runtime->player_death_started_at = now;

        // Cut any boss music immediately as the run leaves
        // active gameplay (v0.8.0 Phase 5B) - if this death
        // happened mid-Dreadnought-fight, Mountain King must
        // not keep playing through the death pause, GAME_OVER,
        // and beyond. A no-op the rest of the time, since
        // nothing plays during normal (non-boss) gameplay.
        soundtrack_stop(laser);

        // Wipe out any Scout projectiles still in flight.
        enemy_bullets_init(enemy_bullets);

        // A brief, subtle flash on top of the player-hit shake
        // already triggered by whichever collision caused this
        // - translucent, so it doesn't hide the destruction
        // effect playing out underneath it.
        screen_effects_flash(
            screen_effects,
            PLAYER_DEATH_FLASH_R,
            PLAYER_DEATH_FLASH_G,
            PLAYER_DEATH_FLASH_B,
            PLAYER_DEATH_FLASH_ALPHA,
            PLAYER_DEATH_FLASH_DURATION_MS,
            runtime->player_death_started_at
        );
    }
}

// Continuous held-SPACE firing, plus releasing the fire-suppression
// flag once the SPACE press that started the run is let go. Reads
// continuous keyboard state (not an event), so this fires every frame
// SPACE is held, independent of process_input_events() above.
static void update_player_firing(
    GameRuntime *runtime,
    Player *player,
    const Uint8 *keyboard,
    Bullet bullets[],
    Uint32 *last_shot_time,
    const PowerUpState *powerup_state,
    LaserSound *laser
)
{
    // Once the key that started the game is finally released,
    // SPACE is free to mean "fire" again from here on.
    if (runtime->suppress_fire_until_space_released &&
            !keyboard[SDL_SCANCODE_SPACE])
    {
        runtime->suppress_fire_until_space_released = 0;
    }

    // Fire continuously while the Space bar is held down -
    // unless this is still the same press that just started
    // the game.
    if (keyboard[SDL_SCANCODE_SPACE] &&
            !runtime->suppress_fire_until_space_released)
    {
        Uint32 now = game_ticks(runtime);

        // Rapid Fire simply swaps in a shorter cooldown for as
        // long as it's active - bullets_fire() itself has no
        // idea power-ups exist.
        Uint32 fire_cooldown =
            powerup_state->rapid_fire_active
            ? RAPID_FIRE_COOLDOWN
            : FIRE_COOLDOWN;

        // Rapid Fire and Spread Shot are independent: the
        // cooldown above applies either way, while this just
        // decides one straight bullet vs. a three-way burst.
        int fired;

        if (powerup_state->spread_shot_active)
        {
            fired = bullets_fire_spread(
                        bullets,
                        player,
                        now,
                        last_shot_time,
                        fire_cooldown
                    );
        }
        else
        {
            fired = bullets_fire(
                        bullets,
                        player,
                        now,
                        last_shot_time,
                        fire_cooldown
                    );
        }

        if (fired)
        {
            audio_play_laser(laser);
        }
    }
}

// The standard response to the player taking a hit - a burst of the
// player-explosion SFX plus the standard-magnitude screen shake, used
// identically at all four player-damage sites in
// update_projectiles_and_collisions() below (v0.8.0 main.c refactor,
// Phase 6). Each call site still independently decides WHETHER the
// player was actually damaged this frame (its own collision return
// value/out-parameter, unchanged); this only consolidates what happens
// once that's already true.
static void respond_to_player_hit(
    ScreenEffects *screen_effects,
    LaserSound *laser,
    Uint32 now
)
{
    audio_play_explosion(laser);
    screen_effects_shake(
        screen_effects,
        PLAYER_HIT_SHAKE_MAGNITUDE,
        PLAYER_HIT_SHAKE_DURATION_MS,
        now
    );
}

// Advance every projectile pool, then run every collision pair for
// this frame. Bullet/enemy-bullet movement must happen before any
// collision check below reads their positions. Every collision here
// preserves its original score-award order and SFX/shake side effect
// exactly.
static void update_projectiles_and_collisions(
    GameRuntime *runtime,
    Player *player,
    Bullet bullets[],
    Enemy enemies[],
    EnemyBullet enemy_bullets[],
    Asteroid asteroids[],
    PowerUp powerups[],
    PowerUpState *powerup_state,
    Boss *boss,
    ExplosionParticle explosions[],
    ScorePopup score_popups[],
    ScreenEffects *screen_effects,
    LaserSound *laser
)
{
    // One snapshot for the whole function - safe because nothing here
    // can toggle the pause state (see update_player_movement()'s
    // identical comment above), so every one of the many game_ticks()
    // calls this function used to make individually would have
    // returned this exact same value anyway.
    Uint32 now = game_ticks(runtime);

    bullets_update(bullets);
    enemy_bullets_update(enemy_bullets);

    // Player bullets vs. enemies.
    int points_earned = collisions_bullets_enemies(
                             bullets,
                             enemies,
                             explosions,
                             powerups,
                             score_popups,
                             now
                         );
    runtime->score += points_earned;

    if (points_earned > 0)
    {
        audio_play_enemy_explosion(laser);
    }

    // Player ramming into enemies.
    int player_rammed = 0;
    int enemies_rammed = collisions_player_enemies(
                              player,
                              enemies,
                              explosions,
                              powerups,
                              powerup_state,
                              now,
                              &player_rammed
                          );

    if (enemies_rammed > 0)
    {
        audio_play_enemy_explosion(laser);
    }

    if (player_rammed)
    {
        respond_to_player_hit(screen_effects, laser, now);
    }

    // Player bullets vs. asteroids.
    int asteroid_points_earned = collisions_bullets_asteroids(
                                      bullets,
                                      asteroids,
                                      explosions,
                                      score_popups,
                                      now
                                  );
    runtime->score += asteroid_points_earned;

    if (asteroid_points_earned > 0)
    {
        audio_play_enemy_explosion(laser);
    }

    // Player bullets vs. the boss - works the same regardless
    // of whether it was a normal shot, Rapid Fire, or one of
    // Spread Shot's three pellets, since they're all just
    // Bullets from the same pool.
    collisions_bullets_boss(bullets, boss, explosions, now);

    // Player ramming into asteroids.
    int player_hit_asteroid = 0;
    int asteroids_rammed = collisions_player_asteroids(
                                player,
                                asteroids,
                                explosions,
                                powerup_state,
                                now,
                                &player_hit_asteroid
                            );

    if (asteroids_rammed > 0)
    {
        audio_play_enemy_explosion(laser);
    }

    if (player_hit_asteroid)
    {
        respond_to_player_hit(screen_effects, laser, now);
    }

    // Player ramming into the boss - damages the player only,
    // never the boss itself.
    if (collisions_player_boss(
                player,
                boss,
                explosions,
                powerup_state,
                now))
    {
        respond_to_player_hit(screen_effects, laser, now);
    }

    // Enemy bullets vs. player.
    if (collisions_player_enemy_bullets(
                player,
                enemy_bullets,
                explosions,
                powerup_state,
                now))
    {
        respond_to_player_hit(screen_effects, laser, now);
    }
}

// Threat spawning, firing, and movement for one frame: the Wave
// Director's difficulty lookup, the boss warning trigger, Scout/Bomber
// spawning (including the boss's CRITICAL-phase support Scouts),
// enemy and boss weapon fire, and every enemy/asteroid pool's own
// movement update. Kept as one function, in original order, rather
// than split further - every step here shares timers/pools with its
// neighbors, and the instructions for this extraction call for
// preserving that sharing intact. Returns the difficulty lookup since
// update_wave_and_debug() below still needs difficulty.boss_wave for
// the B debug key.
static WaveDifficulty update_threats(
    GameRuntime *runtime,
    const WaveState *wave,
    Boss *boss,
    Enemy enemies[],
    EnemyBullet enemy_bullets[],
    Asteroid asteroids[],
    Uint32 *last_scout_spawn,
    Uint32 *last_bomber_spawn,
    Uint32 *last_asteroid_spawn,
    LaserSound *laser
)
{
    // One snapshot for the whole function - see
    // update_player_movement()'s identical comment above for why this
    // is safe. Previously enemies_fire() and boss_fire() below each
    // called game_ticks() separately instead of reusing this value;
    // now they don't.
    Uint32 now = game_ticks(runtime);

    // The Wave Director decides which threats are active and
    // how aggressively they spawn; the enemy/asteroid modules
    // still do all the actual spawning.
    WaveDifficulty difficulty = wave_get_difficulty(wave);

    // Trigger the warning sequence the first time we see a
    // boss wave while the boss is still inactive.
    // boss_begin_warning() immediately moves it out of
    // BOSS_STATE_INACTIVE, so this naturally only fires once
    // per encounter. boss_update() advances WARNING into the
    // actual boss_spawn() once its timer elapses (see
    // BOSS_WARNING_ENDED in update_boss_and_extra_life() below).
    if (difficulty.boss_wave && boss->state == BOSS_STATE_INACTIVE)
    {
        boss_begin_warning(boss, now);
        audio_set_boss_warning(laser, 1);
    }

    if (difficulty.scouts_enabled)
    {
        enemies_spawn_scout(
            enemies,
            now,
            last_scout_spawn,
            difficulty.scout_spawn_delay,
            difficulty.scout_fire_delay
        );
    }

    // CRITICAL-phase support Scouts - a no-op outside phase 3,
    // and reuses the exact same Scout spawn function/pool as
    // the line above, so a support Scout is just a normal
    // enemy in every way once it exists.
    boss_spawn_support(boss, enemies, now);

    if (difficulty.bombers_enabled)
    {
        enemies_spawn_bomber(
            enemies,
            now,
            last_bomber_spawn,
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
                                 now
                             );

    if (enemy_fire_result & ENEMY_FIRED_BOLT)
    {
        audio_play_enemy_laser(laser);
    }

    if (enemy_fire_result & ENEMY_FIRED_BOMB)
    {
        audio_play_bomb_drop(laser);
    }

    // Boss weapon fire - reuses the same enemy bullet pool
    // and the Scout's laser sound, no new audio needed yet.
    if (boss_fire(boss, enemy_bullets, now))
    {
        audio_play_enemy_laser(laser);
    }

    enemies_update(enemies);

    // Spawn and move asteroids alongside the regular enemies.
    if (difficulty.asteroids_enabled)
    {
        asteroids_spawn(
            asteroids,
            now,
            last_asteroid_spawn,
            difficulty.asteroid_spawn_delay
        );
    }

    asteroids_update(asteroids);

    return difficulty;
}

// Power-up pool movement, player collection, and active-effect
// expiry - in that order, matching the original inline sequence.
static void update_powerups(
    GameRuntime *runtime,
    Player *player,
    PowerUp powerups[],
    PowerUpState *powerup_state,
    ExplosionParticle explosions[],
    LaserSound *laser
)
{
    // One snapshot for the whole function - see
    // update_player_movement()'s identical comment above for why this
    // is safe.
    Uint32 now = game_ticks(runtime);

    powerups_update(powerups);

    // Player collecting power-ups.
    if (collisions_player_powerups(
                player,
                powerups,
                powerup_state,
                explosions,
                now) > 0)
    {
        audio_play_pickup(laser);
    }

    // Expire any timed effects whose duration has elapsed.
    powerup_state_update(powerup_state, now);
}

// Steps the boss encounter, reacts to every event boss_update() can
// report this frame (in the exact order they were already handled),
// checks for a newly-earned extra life, and hands off to the Wave
// Director once the defeat message has shown long enough. Extra-life
// checking sits here (not its own stage) because that's exactly where
// it already ran in the original inline pipeline: immediately after
// every source of score gain this frame (enemy kills, asteroids, and
// - above, in this same function - a boss defeat) has had its chance
// to run.
static void update_boss_and_extra_life(
    GameRuntime *runtime,
    Player *player,
    Boss *boss,
    WaveState *wave,
    ExplosionParticle explosions[],
    ScorePopup score_popups[],
    ScreenEffects *screen_effects,
    LaserSound *laser
)
{
    // One snapshot for the whole function - see
    // update_player_movement()'s identical comment above for why this
    // is safe.
    Uint32 now = game_ticks(runtime);

    // Move the boss (entrance, then vertical bounce once in
    // combat), check phase transitions/defeat, and step
    // through the destruction sequence. A no-op while
    // inactive, so this is safe to call unconditionally -
    // only frozen because it's inside the GAME_PLAYING block,
    // same as every other system here.
    int boss_events = boss_update(boss, explosions, now);

    if (boss_events & BOSS_JUST_DEFEATED)
    {
        // Cut Mountain King the instant the boss dies - do not
        // wait for the 27.8s loop to finish, and no fade (none
        // exists in this architecture). soundtrack_stop() only
        // silences music; it doesn't touch the SFX played just
        // below for this same event.
        soundtrack_stop(laser);

        runtime->score += DREADNOUGHT_SCORE_VALUE;

        // Bigger, longer-lived popup than an ordinary kill -
        // the Dreadnought's payout should read as a much
        // bigger deal than a Scout's +10. Visualizes the exact
        // value just added above, never awards it separately.
        popups_spawn_emphasized(
            score_popups,
            boss->x + boss->width / 2.0f,
            boss->y + boss->height / 2.0f,
            DREADNOUGHT_SCORE_VALUE,
            now
        );
    }

    if (boss_events & BOSS_DEATH_SMALL_EXPLOSION)
    {
        audio_play_enemy_explosion(laser);
    }

    if (boss_events & BOSS_DEATH_FINAL_EXPLOSION)
    {
        audio_play_explosion(laser);

        // The strongest shake in the game so far - reserved
        // for the Dreadnought's own destruction so it reads
        // as clearly bigger than an ordinary player hit.
        screen_effects_shake(
            screen_effects,
            BOSS_DEFEATED_SHAKE_MAGNITUDE,
            BOSS_DEFEATED_SHAKE_DURATION_MS,
            now
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
        audio_set_boss_warning(laser, 0);

        soundtrack_play_boss(laser);

        runtime->boss_music_tier = 1;
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
    if (boss->state == BOSS_STATE_ACTIVE && boss->phase != runtime->boss_music_tier)
    {
        runtime->boss_music_tier = boss->phase;

        soundtrack_set_boss_intensity(laser, runtime->boss_music_tier);
    }

    // Award extra lives for reaching score thresholds - checked
    // once per frame, after every source of score gain above
    // (enemy kills, asteroids, boss defeat) has had a chance to
    // run this frame, so nothing needs its own separate check.
    if (player_check_extra_life(player, runtime->score, now) > 0)
    {
        audio_play_extra_life(laser);
    }

    // Once the defeat message has shown long enough, hand off
    // to the Wave Director and fully reset the boss - ready
    // for a hypothetical future encounter, and back to
    // BOSS_STATE_INACTIVE so its overlays stop drawing.
    if (boss_ready_for_wave_advance(boss, now))
    {
        wave_advance_after_boss(wave, now);
        boss_init(boss);
    }
}

// Wave progression, plus the two development shortcuts (B to skip a
// boss fight, 5 to jump to Wave 5) - grouped together as "remaining
// gameplay-only bookkeeping" for this frame. These shortcuts are
// intentional development tools (useful for testing later waves,
// bosses, music, and transitions without replaying the whole game)
// and are deliberately left in place, unmodified, by every phase of
// the v0.8.0 main.c refactor - a dedicated developer/debug system may
// replace them in a future version, but that's out of scope here.
static void update_wave_and_debug(
    GameRuntime *runtime,
    WaveState *wave,
    Boss *boss,
    const Uint8 *keyboard,
    WaveDifficulty difficulty,
    LaserSound *laser
)
{
    // One snapshot for the whole function - see
    // update_player_movement()'s identical comment above for why this
    // is safe.
    Uint32 now = game_ticks(runtime);

    // Advance wave progression. Only ticks while playing, so
    // waves stay frozen during GAME_OVER like everything else.
    wave_update(wave, now);

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
        audio_set_boss_warning(laser, 0);
        soundtrack_stop(laser);
        wave_advance_after_boss(wave, now);
        boss_init(boss);
    }

    // TEMPORARY DEBUG: press 5 to jump straight to Wave 5
    // instead of playing through 1-4 every time. Speeds up
    // testing the boss encounter. Remove before release.
    if (keyboard[SDL_SCANCODE_5])
    {
        wave_debug_jump(wave, 5, now);
    }
}

int main(void)
{
    // Create an array capable of storing all background stars.
    Star stars[MAX_STARS];
    starfield_init(stars);

    // Every piece of main-owned run/state-machine bookkeeping, in one
    // place - see GameRuntime's declaration above for what each field
    // means and why it lives here. Initialized explicitly, field by
    // field, rather than relying on zero-initialized stack memory -
    // boss_music_tier in particular starts at 1, not 0. Starts on the
    // title screen rather than dropping straight into Wave 1.
    GameRuntime runtime =
    {
        .game_state = GAME_TITLE,
        .pause_offset_ms = 0,
        .is_paused = 0,
        .paused_at_tick = 0,
        .pause_started_at = 0,
        .player_death_started_at = 0,
        .suppress_fire_until_space_released = 0,
        .boss_music_tier = 1,
        .new_high_score = 0,
        .game_over_awaiting_fanfare = 0,
        .score = 0
    };

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
    // is also the only time it gets written back out. Deliberately
    // kept outside GameRuntime: unlike every field there, it survives
    // across runs rather than being reset by game_start_new().
    int high_score = highscore_load();

    // Shared state for every procedurally generated sound effect.
    LaserSound laser;

    if (!audio_init(&laser))
    {
        printf("Audio initialization failed.\n");
    }

    // Start the title theme now, since runtime.game_state above already
    // begins on GAME_TITLE - this is that state's one-time entry, not
    // something the frame loop repeats. See soundtrack.h for what this
    // starts and why it's sample-aligned and reset to normal speed.
    soundtrack_play_title(&laser);

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
        audio_shutdown();
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
        audio_shutdown();
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

    // Per-subsystem coordination timestamps - deliberately kept
    // outside GameRuntime (see its declaration above): each one is
    // mutated directly, by pointer, by the fire/spawn function of the
    // system it times (bullets_fire()/bullets_fire_spread(),
    // enemies_spawn_scout(), enemies_spawn_bomber(), asteroids_spawn()),
    // so it belongs beside that system rather than with the state
    // machine.
    Uint32 last_shot_time = 0;

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
    wave_init(&wave, game_ticks(&runtime));

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

        // Poll every SDL event queued this frame and react to window
        // close and discrete SPACE/ENTER/P key-downs - see
        // process_input_events()'s own comment for exactly what this
        // does and does not do.
        process_input_events(
            &runtime,
            &running,
            &start_game_requested,
            &return_to_title_requested
        );

        // Get the current state of the keyboard.
        // This lets us detect keys that are being held down.
        const Uint8 *keyboard = SDL_GetKeyboardState(NULL);

        if (runtime.game_state == GAME_PLAYING)
        {
            update_player_movement(
                &runtime,
                &player,
                keyboard,
                &boss,
                enemy_bullets,
                &laser,
                &screen_effects
            );

            update_player_firing(
                &runtime,
                &player,
                keyboard,
                bullets,
                &last_shot_time,
                &powerup_state,
                &laser
            );

            update_projectiles_and_collisions(
                &runtime,
                &player,
                bullets,
                enemies,
                enemy_bullets,
                asteroids,
                powerups,
                &powerup_state,
                &boss,
                explosions,
                score_popups,
                &screen_effects,
                &laser
            );

            WaveDifficulty difficulty = update_threats(
                &runtime,
                &wave,
                &boss,
                enemies,
                enemy_bullets,
                asteroids,
                &last_scout_spawn,
                &last_bomber_spawn,
                &last_asteroid_spawn,
                &laser
            );

            update_powerups(
                &runtime,
                &player,
                powerups,
                &powerup_state,
                explosions,
                &laser
            );

            update_boss_and_extra_life(
                &runtime,
                &player,
                &boss,
                &wave,
                explosions,
                score_popups,
                &screen_effects,
                &laser
            );

            update_wave_and_debug(
                &runtime,
                &wave,
                &boss,
                keyboard,
                difficulty,
                &laser
            );

        } // end GAME_PLAYING

        // One snapshot for the rest of this frame's per-frame
        // bookkeeping and rendering below - safe for the same reason
        // as every stage function's own "now" (see
        // update_player_movement()'s comment): nothing between here
        // and the end of the frame can toggle the pause state.
        Uint32 now = game_ticks(&runtime);

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
        if (runtime.game_state == GAME_PLAYER_DEATH &&
                now - runtime.player_death_started_at >= PLAYER_DEATH_DELAY_MS)
        {
            runtime.game_state = GAME_OVER;

            runtime.new_high_score = (runtime.score > high_score);

            if (runtime.new_high_score)
            {
                high_score = runtime.score;
                highscore_save(high_score);
                audio_play_new_high_score(&laser);

                // The Funeral March must not start underneath the
                // fanfare - wait for it to actually finish (checked
                // below, every frame) rather than starting it here.
                runtime.game_over_awaiting_fanfare = 1;
            }
            else
            {
                // No fanfare to wait for - the Funeral March is this
                // run's only Game Over audio, so it starts right away.
                // soundtrack_play_game_over() resets the shared
                // playback rate to 1.0 as part of its locked call, so a
                // boss-fight death can never leave Mountain King's
                // 1.15x/1.30x rate active under the Funeral March.
                soundtrack_play_game_over(&laser);
            }
        }

        // The fanfare finished naturally (not cancelled - ENTER's
        // handling below clears game_over_awaiting_fanfare itself, so
        // this can't also fire after that) - start the Funeral March
        // exactly once, the moment it's actually safe to, rather than
        // guessing at the fanfare's duration. Checked every frame
        // while GAME_OVER is waiting, but the flag itself is only ever
        // set once per run (immediately above) and cleared the first
        // time this fires, so soundtrack_play_game_over() below runs
        // at most once per Game Over screen.
        if (runtime.game_over_awaiting_fanfare &&
                audio_new_high_score_finished(&laser))
        {
            runtime.game_over_awaiting_fanfare = 0;

            soundtrack_play_game_over(&laser);
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
            transition_game_over_to_title(&runtime, &screen_effects, &laser);
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
            transition_title_to_playing(
                &runtime,
                &laser,
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
                &screen_effects,
                score_popups
            );
        }

        starfield_update(stars);

        // Keep any in-flight explosions animating, even after game over.
        explosions_update(explosions);

        // Same idea for score popups already in flight.
        popups_update(score_popups, now);

        RenderContext render_ctx =
        {
            .game_state = runtime.game_state,
            .stars = stars,
            .player = &player,
            .bullets = bullets,
            .enemies = enemies,
            .asteroids = asteroids,
            .powerups = powerups,
            .enemy_bullets = enemy_bullets,
            .boss = &boss,
            .explosions = explosions,
            .score_popups = score_popups,
            .powerup_state = &powerup_state,
            .wave = &wave,
            .screen_effects = &screen_effects,
            .score = runtime.score,
            .high_score = high_score,
            .new_high_score = runtime.new_high_score,
            .now = now
        };

        game_render_frame(renderer, &render_ctx);

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
    audio_shutdown();

    // Shut down all initialized SDL systems.
    SDL_Quit();

    // Returning 0 tells the operating system that the program
    // finished successfully.
    return 0;

} // end of main
