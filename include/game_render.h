#ifndef GAME_RENDER_H
#define GAME_RENDER_H

#include <SDL.h>

#include "player.h"
#include "starfield.h"
#include "bullet.h"
#include "enemy.h"
#include "enemy_bullet.h"
#include "asteroid.h"
#include "powerup.h"
#include "boss.h"
#include "explosion.h"
#include "popup.h"
#include "wave.h"
#include "screen_effects.h"

// The states the game can be in. Declared here, not in main.c, since
// this module has to branch on it to decide what a given frame should
// show - main.c still owns every state TRANSITION; the two files just
// need to agree on what the states themselves are.
typedef enum
{
    GAME_TITLE,        // Idle on the title screen - no gameplay systems run.
    GAME_PLAYING,
    GAME_PAUSED,       // GAME_PLAYING frozen mid-run - see main.c's P key handling.
    GAME_PLAYER_DEATH, // Final life just lost - a short dramatic pause before GAME_OVER.
    GAME_OVER

} GameState;

// Everything one frame's rendering needs to read. Plain pointers into
// main.c's own owned state (never a second copy of it), and never
// written to by game_render.c - this module only draws. It must never
// award score, change lives, advance waves, start audio, spawn
// objects, or otherwise mutate gameplay state.
typedef struct
{
    GameState game_state;

    const Star *stars;

    const Player *player;
    const Bullet *bullets;
    const Enemy *enemies;
    const Asteroid *asteroids;
    const PowerUp *powerups;
    const EnemyBullet *enemy_bullets;
    const Boss *boss;
    const ExplosionParticle *explosions;
    const ScorePopup *score_popups;
    const PowerUpState *powerup_state;
    const WaveState *wave;
    const ScreenEffects *screen_effects;

    int score;
    int high_score;
    int new_high_score;

    // A single "now" snapshot for the whole frame, taken once by
    // main.c (via its pause-aware game_ticks()) immediately before
    // rendering. Every render call that needs a timestamp (HUD
    // countdown bars, popups, shake, flash) reads this same value -
    // exactly equivalent to the original inline code calling
    // game_ticks() separately at each of those points, since nothing
    // in a render pass can change the pause state mid-frame.
    Uint32 now;

} RenderContext;

// Render one complete frame: clear, the screen-shake viewport, the
// starfield, either the title screen or the full gameplay presentation
// depending on game_state, the wave-announcement/PAUSED/GAME OVER
// overlays, the screen flash, and finally present. Draw order matches
// the original inline main.c block exactly - the critical invariant
// this function preserves is that the final frame looks identical.
void game_render_frame(SDL_Renderer *renderer, const RenderContext *ctx);

#endif
