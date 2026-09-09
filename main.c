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


int score = 0;

// The states the game can be in.
typedef enum
{
    GAME_TITLE,        // Idle on the title screen - no gameplay systems run.
    GAME_PLAYING,
    GAME_PLAYER_DEATH, // Final life just lost - a short dramatic pause before GAME_OVER.
    GAME_OVER

} GameState;

// Resets every piece of gameplay state to a fresh run. This is the
// one place both "start a new game from the title screen" and
// "restart after GAME_OVER" go through, so the two paths can never
// quietly drift apart the way two separate copies of the same reset
// list eventually do.
//
// current_time is passed in (rather than each system calling
// SDL_GetTicks() itself) so every timer this touches - Wave 1's
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

    // Create the main game window.
    // SDL_CreateWindow() returns a pointer to the new window,
    // or NULL if the window could not be created.
    SDL_Window *window = SDL_CreateWindow(
                             "Space Shooter",
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

    // Set at most once per run, at the GAME_PLAYER_DEATH -> GAME_OVER
    // handoff - never re-evaluated on later frames while GAME_OVER is
    // just sitting on screen. Read by the GAME_OVER render block to
    // decide whether to show "NEW HIGH SCORE!" instead of the normal
    // "HIGH SCORE <n>" line.
    int new_high_score = 0;

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
    wave_init(&wave, SDL_GetTicks());

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
                SDL_GetTicks(),
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
                player_death_started_at = SDL_GetTicks();

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
                Uint32 current_time = SDL_GetTicks();

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
                                     SDL_GetTicks()
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
                                      SDL_GetTicks(),
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
                    SDL_GetTicks()
                );
            }

            // Player bullets vs. asteroids.
            int asteroid_points_earned = collisions_bullets_asteroids(
                                              bullets,
                                              asteroids,
                                              explosions,
                                              score_popups,
                                              SDL_GetTicks()
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
            collisions_bullets_boss(bullets, &boss, explosions, SDL_GetTicks());

            // Player ramming into asteroids.
            int player_hit_asteroid = 0;
            int asteroids_rammed = collisions_player_asteroids(
                                        &player,
                                        asteroids,
                                        explosions,
                                        &powerup_state,
                                        SDL_GetTicks(),
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
                    SDL_GetTicks()
                );
            }

            // Player ramming into the boss - damages the player only,
            // never the boss itself.
            if (collisions_player_boss(
                        &player,
                        &boss,
                        explosions,
                        &powerup_state,
                        SDL_GetTicks()))
            {
                audio_play_explosion(&laser);
                screen_effects_shake(
                    &screen_effects,
                    PLAYER_HIT_SHAKE_MAGNITUDE,
                    PLAYER_HIT_SHAKE_DURATION_MS,
                    SDL_GetTicks()
                );
            }

            // Enemy bullets vs. player.
            if (collisions_player_enemy_bullets(
                        &player,
                        enemy_bullets,
                        explosions,
                        &powerup_state,
                        SDL_GetTicks()))
            {
                audio_play_explosion(&laser);
                screen_effects_shake(
                    &screen_effects,
                    PLAYER_HIT_SHAKE_MAGNITUDE,
                    PLAYER_HIT_SHAKE_DURATION_MS,
                    SDL_GetTicks()
                );
            }

            Uint32 current_time = SDL_GetTicks();

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
                                         SDL_GetTicks()
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
            if (boss_fire(&boss, enemy_bullets, SDL_GetTicks()))
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
                        SDL_GetTicks()) > 0)
            {
                audio_play_pickup(&laser);
            }

            // Expire any timed effects whose duration has elapsed.
            powerup_state_update(&powerup_state, SDL_GetTicks());

            // Move the boss (entrance, then vertical bounce once in
            // combat), check phase transitions/defeat, and step
            // through the destruction sequence. A no-op while
            // inactive, so this is safe to call unconditionally -
            // only frozen because it's inside the GAME_PLAYING block,
            // same as every other system here.
            int boss_events = boss_update(&boss, explosions, SDL_GetTicks());

            if (boss_events & BOSS_JUST_DEFEATED)
            {
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
                    SDL_GetTicks()
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
                    SDL_GetTicks()
                );
            }

            // The warning sequence just finished and the boss actually
            // spawned - stop the klaxon now that it's served its
            // purpose.
            if (boss_events & BOSS_WARNING_ENDED)
            {
                audio_set_boss_warning(&laser, 0);
            }

            // Award extra lives for reaching score thresholds - checked
            // once per frame, after every source of score gain above
            // (enemy kills, asteroids, boss defeat) has had a chance to
            // run this frame, so nothing needs its own separate check.
            if (player_check_extra_life(&player, score, SDL_GetTicks()) > 0)
            {
                audio_play_extra_life(&laser);
            }

            // Once the defeat message has shown long enough, hand off
            // to the Wave Director and fully reset the boss - ready
            // for a hypothetical future encounter, and back to
            // BOSS_STATE_INACTIVE so its overlays stop drawing.
            if (boss_ready_for_wave_advance(&boss, SDL_GetTicks()))
            {
                wave_advance_after_boss(&wave, SDL_GetTicks());
                boss_init(&boss);
            }

            // Advance wave progression. Only ticks while playing, so
            // waves stay frozen during GAME_OVER like everything else.
            wave_update(&wave, SDL_GetTicks());

            // TEMPORARY DEBUG: press B during a boss wave to skip
            // straight past the fight instead of playing it out - the
            // real defeat path (Phase 9/10) handles this normally now,
            // this is just a fast-forward for testing. Resets the
            // boss too, so it's safe to press at any point mid-fight -
            // including mid-warning, which is why the klaxon is
            // explicitly stopped here too rather than assuming
            // BOSS_WARNING_ENDED already handled it - without leaving
            // a stale ENTERING/ACTIVE boss (or a stuck alarm) behind on
            // the next (non-boss) wave. Remove before release.
            if (difficulty.boss_wave && keyboard[SDL_SCANCODE_B])
            {
                audio_set_boss_warning(&laser, 0);
                wave_advance_after_boss(&wave, SDL_GetTicks());
                boss_init(&boss);
            }

            // TEMPORARY DEBUG: press 5 to jump straight to Wave 5
            // instead of playing through 1-4 every time. Speeds up
            // testing the boss encounter. Remove before release.
            if (keyboard[SDL_SCANCODE_5])
            {
                wave_debug_jump(&wave, 5, SDL_GetTicks());
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
                SDL_GetTicks() - player_death_started_at >= PLAYER_DEATH_DELAY_MS)
        {
            game_state = GAME_OVER;

            new_high_score = (score > high_score);

            if (new_high_score)
            {
                high_score = score;
                highscore_save(high_score);
                audio_play_new_high_score(&laser);
            }
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
                SDL_GetTicks()
            );

            game_state = GAME_PLAYING;
            suppress_fire_until_space_released = 1;
        }

        starfield_update(stars);

        // Keep any in-flight explosions animating, even after game over.
        explosions_update(explosions);

        // Same idea for score popups already in flight.
        popups_update(score_popups, SDL_GetTicks());

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
            SDL_GetTicks(),
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

            const char *title = "SPACE SHOOTER";
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
                powerup_time_remaining(powerup_state.rapid_fire_until, SDL_GetTicks()),
                RAPID_FIRE_DURATION,
                SDL_GetTicks()
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
                powerup_time_remaining(powerup_state.spread_shot_until, SDL_GetTicks()),
                SPREAD_SHOT_DURATION,
                SDL_GetTicks()
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
        popups_render(renderer, score_popups, SDL_GetTicks());

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
        // a separate frozen state. Gated to GAME_PLAYING since
        // wave_update() (which clears announcement_active) doesn't
        // run during GAME_OVER either. Also suppressed during
        // BOSS_STATE_WARNING - both this and boss_render_warning()
        // start at the same instant Wave 5 begins and run for the
        // same ~1.8s, so showing both at once would just overlap into
        // unreadable text; the boss-specific warning is the more
        // dramatic and more specific of the two, so it wins.
        if (game_state == GAME_PLAYING && boss.state != BOSS_STATE_WARNING)
        {
            wave_render_announcement(renderer, &wave);
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
        screen_effects_render_flash(renderer, &screen_effects, SDL_GetTicks());

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
