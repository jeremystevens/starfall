#include "boss.h"

#include "game_config.h"
#include "text.h"

#include <stdlib.h>

// How fast the Dreadnought slides in during its entrance, and how
// fast it drifts vertically once in combat. Later phases may speed
// this up as health drops.
#define DREADNOUGHT_ENTRANCE_SPEED 0.6f
#define DREADNOUGHT_MOVE_SPEED 0.3f

// Matches the speed enemy_bullets_fire() already uses for a straight
// bolt, so the boss's angled spread shots feel consistent with its
// own straight ones instead of looking like a different weapon.
#define BOSS_BOLT_SPEED 2.0f

// Reset the boss to an idle, inactive state.
void boss_init(Boss *boss)
{
    boss->type = BOSS_DREADNOUGHT;
    boss->state = BOSS_STATE_INACTIVE;

    boss->x = 0.0f;
    boss->y = 0.0f;

    boss->dx = 0.0f;
    boss->dy = 0.0f;

    boss->width = DREADNOUGHT_WIDTH;
    boss->height = DREADNOUGHT_HEIGHT;

    boss->health = DREADNOUGHT_MAX_HEALTH;
    boss->max_health = DREADNOUGHT_MAX_HEALTH;

    boss->phase = 1;

    boss->combat_x = DREADNOUGHT_COMBAT_X;

    boss->last_shot_time = 0;
    boss->fire_delay = DREADNOUGHT_FIRE_DELAY;

    boss->attack_index = 0;
    boss->phase_flash_until = 0;
    boss->hit_flash_until = 0;
    boss->last_support_spawn = 0;

    boss->death_explosions_fired = 0;
    boss->next_death_explosion_time = 0;
    boss->defeated_at = 0;
    boss->warning_started_at = 0;
    boss->next_damage_effect_time = 0;
}

// Begin the warning sequence that precedes a new encounter. Nothing
// about the boss itself (position, health, phase) is set up yet -
// boss_update() does that by calling boss_spawn() once the warning's
// timer elapses, exactly the same setup as if it had been called
// directly.
void boss_begin_warning(Boss *boss, Uint32 current_time)
{
    boss->type = BOSS_DREADNOUGHT;
    boss->state = BOSS_STATE_WARNING;
    boss->warning_started_at = current_time;
}

// Begin a new Dreadnought encounter, entering from just beyond the
// right edge of the screen.
void boss_spawn(Boss *boss, Uint32 current_time)
{
    boss->type = BOSS_DREADNOUGHT;
    boss->state = BOSS_STATE_ENTERING;

    boss->width = DREADNOUGHT_WIDTH;
    boss->height = DREADNOUGHT_HEIGHT;

    boss->x = (float)SCREEN_WIDTH;

    // Start vertically centered in the playable area - below the
    // health bar overlay, not just the normal HUD strip.
    boss->y = (float)BOSS_FIGHT_TOP_BOUNDARY +
              (float)(SCREEN_HEIGHT - BOSS_FIGHT_TOP_BOUNDARY - boss->height) / 2.0f;

    boss->dx = -DREADNOUGHT_ENTRANCE_SPEED;
    boss->dy = 0.0f; // Vertical drift only starts once combat begins.

    boss->health = DREADNOUGHT_MAX_HEALTH;
    boss->max_health = DREADNOUGHT_MAX_HEALTH;

    boss->phase = 1;

    boss->combat_x = DREADNOUGHT_COMBAT_X;

    boss->last_shot_time = current_time;
    boss->fire_delay = DREADNOUGHT_FIRE_DELAY;

    boss->attack_index = 0;
    boss->phase_flash_until = 0;
    boss->hit_flash_until = 0;
    boss->last_support_spawn = current_time;

    boss->death_explosions_fired = 0;
    boss->next_death_explosion_time = 0;
    boss->defeated_at = 0;
    boss->next_damage_effect_time = 0;
}

// Shared by every phase transition: sets the new phase and pacing,
// starts the white flash, and spawns a burst from the shared
// explosion pool. Keeps GUNSHIP->BARRAGE and BARRAGE->CRITICAL from
// needing to duplicate this every time a new phase is added.
static void trigger_phase_transition(
    Boss *boss,
    ExplosionParticle explosions[],
    Uint32 current_time,
    int new_phase,
    float move_speed,
    Uint32 fire_delay
)
{
    boss->phase = new_phase;

    // Preserve current bounce direction, just move faster.
    boss->dy = (boss->dy > 0.0f) ? move_speed : -move_speed;

    boss->fire_delay = fire_delay;

    boss->phase_flash_until = current_time + PHASE_TRANSITION_FLASH_MS;

    explosions_spawn(
        explosions,
        boss->x + (boss->width / 2.0f),
        boss->y + (boss->height / 2.0f),
        255, 200, 80,
        PHASE_TRANSITION_EXPLOSION_PARTICLES
    );
}

// Move the boss, handle ENTERING -> ACTIVE, check for phase
// transitions and defeat, and step through the destruction sequence.
// Phases and defeat are both driven by health percentage/value rather
// than a timer, checked fresh every frame rather than only at the
// moment of a hit, so neither can ever be missed regardless of
// exactly how damage was applied.
int boss_update(
    Boss *boss,
    ExplosionParticle explosions[],
    Uint32 current_time
)
{
    int result = 0;

    switch (boss->state)
    {
    case BOSS_STATE_WARNING:
        if (current_time - boss->warning_started_at >= BOSS_WARNING_DURATION_MS)
        {
            // Reuses the exact same setup boss_spawn() would do if
            // called directly - the warning was purely a delay, not a
            // different way of entering.
            boss_spawn(boss, current_time);

            result |= BOSS_WARNING_ENDED;
        }

        break;

    case BOSS_STATE_ENTERING:
        boss->x += boss->dx;

        if (boss->x <= boss->combat_x)
        {
            boss->x = boss->combat_x;
            boss->dx = 0.0f;

            // Combat begins - start the vertical bounce.
            boss->dy = DREADNOUGHT_MOVE_SPEED;

            boss->state = BOSS_STATE_ACTIVE;
            boss->last_shot_time = current_time;
        }
        break;

    case BOSS_STATE_ACTIVE:
        boss->y += boss->dy;

        // Bounce off the health bar overlay and the bottom of the
        // play area - the same "clamp and flip" pattern asteroids
        // already use to stay out of the HUD.
        if (boss->y < BOSS_FIGHT_TOP_BOUNDARY)
        {
            boss->y = (float)BOSS_FIGHT_TOP_BOUNDARY;
            boss->dy = -boss->dy;
        }
        else if (boss->y + boss->height > SCREEN_HEIGHT)
        {
            boss->y = (float)(SCREEN_HEIGHT - boss->height);
            boss->dy = -boss->dy;
        }

        // Defeat takes priority over phase transitions - a lethal
        // volley (e.g. several Spread Shot pellets landing at once)
        // could cross a phase threshold and zero health in the same
        // frame, and defeat should win that race. The state change
        // out of BOSS_STATE_ACTIVE below guarantees this block - and
        // therefore BOSS_JUST_DEFEATED - can only ever fire once.
        if (boss->health <= 0)
        {
            boss->state = BOSS_STATE_DYING;

            boss->death_explosions_fired = 0;
            boss->next_death_explosion_time = current_time;

            result |= BOSS_JUST_DEFEATED;
        }
        // GUNSHIP -> BARRAGE at 66% health, BARRAGE -> CRITICAL at
        // 33%. Both one-way transitions - phase never regresses even
        // if a future power-up or mechanic could restore boss health,
        // since none currently exist to do so.
        else if (boss->phase == 1 &&
                 boss->health <= (boss->max_health * 66) / 100)
        {
            trigger_phase_transition(
                boss, explosions, current_time, 2,
                DREADNOUGHT_PHASE2_MOVE_SPEED,
                DREADNOUGHT_PHASE2_FIRE_DELAY
            );
        }
        else if (boss->phase == 2 &&
                 boss->health <= (boss->max_health * 33) / 100)
        {
            trigger_phase_transition(
                boss, explosions, current_time, 3,
                DREADNOUGHT_PHASE3_MOVE_SPEED,
                DREADNOUGHT_PHASE3_FIRE_DELAY
            );
        }

        // Ongoing cosmetic battle damage from Phase 2 onward - a small
        // spark/smoke burst at a random spot on the hull, checked
        // against its own independent timer rather than rolling random
        // chance every frame. Not an else-if off the phase-transition
        // chain above: this should still fire on the very same frame a
        // transition happens, not skip a beat waiting for the next one.
        // CRITICAL's shorter interval and smokier gray (vs. BARRAGE's
        // orange sparks) make the final phase read as visibly more
        // damaged, with boss.c's phase system remaining the only thing
        // that actually changes health, speed, or fire rate.
        if (boss->phase >= 2 && current_time >= boss->next_damage_effect_time)
        {
            float spot_x = boss->x + (float)(rand() % boss->width);
            float spot_y = boss->y + (float)(rand() % boss->height);

            if (boss->phase >= 3)
            {
                explosions_spawn(
                    explosions,
                    spot_x, spot_y,
                    160, 160, 160,
                    DREADNOUGHT_DAMAGE_EFFECT_PARTICLES
                );

                boss->next_damage_effect_time =
                    current_time + DREADNOUGHT_PHASE3_DAMAGE_EFFECT_INTERVAL_MS;
            }
            else
            {
                explosions_spawn(
                    explosions,
                    spot_x, spot_y,
                    255, 190, 100,
                    DREADNOUGHT_DAMAGE_EFFECT_PARTICLES
                );

                boss->next_damage_effect_time =
                    current_time + DREADNOUGHT_PHASE2_DAMAGE_EFFECT_INTERVAL_MS;
            }
        }

        break;

    case BOSS_STATE_DYING:
        if (current_time >= boss->next_death_explosion_time)
        {
            if (boss->death_explosions_fired < DREADNOUGHT_DEATH_EXPLOSION_COUNT)
            {
                // A random spot within the hull for this beat, so the
                // explosions read as scattered across the ship rather
                // than all landing in the same place.
                float spot_x = boss->x + (float)(rand() % boss->width);
                float spot_y = boss->y + (float)(rand() % boss->height);

                explosions_spawn(
                    explosions,
                    spot_x,
                    spot_y,
                    255, 150, 60,
                    DREADNOUGHT_DEATH_SMALL_EXPLOSION_PARTICLES
                );

                boss->death_explosions_fired++;
                boss->next_death_explosion_time =
                    current_time + DREADNOUGHT_DEATH_EXPLOSION_INTERVAL_MS;

                result |= BOSS_DEATH_SMALL_EXPLOSION;
            }
            else
            {
                // The staggered beats are done - one large finale at
                // the center, then the sequence is over.
                explosions_spawn(
                    explosions,
                    boss->x + (boss->width / 2.0f),
                    boss->y + (boss->height / 2.0f),
                    255, 220, 150,
                    DREADNOUGHT_DEATH_FINAL_EXPLOSION_PARTICLES
                );

                boss->state = BOSS_STATE_DEFEATED;
                boss->defeated_at = current_time;

                result |= BOSS_DEATH_FINAL_EXPLOSION;
            }
        }

        break;

    case BOSS_STATE_INACTIVE:
    case BOSS_STATE_DEFEATED:
    default:
        break;
    }

    return result;
}

// True once the boss has been defeated for long enough to advance.
int boss_ready_for_wave_advance(const Boss *boss, Uint32 current_time)
{
    return boss->state == BOSS_STATE_DEFEATED &&
           (current_time - boss->defeated_at) >= WAVE_COMPLETE_DELAY_MS;
}

// Straight shot from the nose cannon - GUNSHIP's only attack, and
// still BARRAGE's "straight" half of its alternation.
static int fire_straight(Boss *boss, EnemyBullet bullets[])
{
    float bullet_y = boss->y + (boss->height / 2.0f);

    return enemy_bullets_fire(bullets, boss->x, bullet_y);
}

// Three-bolt spread from three different gun positions: nose
// (straight), upper gun port (angled up), lower gun port (angled
// down) - BARRAGE's other half, introduced at Phase 2.
static int fire_spread(Boss *boss, EnemyBullet bullets[])
{
    float nose_y = boss->y + (boss->height / 2.0f);
    float upper_gun_y = boss->y + 4.0f;
    float lower_gun_y = boss->y + (float)boss->height - 4.0f;
    float gun_x = boss->x + 8.0f;

    int fired = enemy_bullets_fire(bullets, boss->x, nose_y);

    fired |= enemy_bullets_fire_angled(
                 bullets, gun_x, upper_gun_y,
                 -BOSS_BOLT_SPEED, -DREADNOUGHT_SPREAD_DY
             );

    fired |= enemy_bullets_fire_angled(
                 bullets, gun_x, lower_gun_y,
                 -BOSS_BOLT_SPEED, DREADNOUGHT_SPREAD_DY
             );

    return fired;
}

// Fire if the cooldown allows. GUNSHIP (phase 1) always fires the
// simple straight shot; BARRAGE (phase 2) alternates evenly between
// that and the three-way spread; CRITICAL (phase 3) mixes in spread
// twice as often as straight, per attack_index mod 3, so the attack
// stays varied without ever combining into one overwhelming volley.
int boss_fire(
    Boss *boss,
    EnemyBullet bullets[],
    Uint32 current_time
)
{
    if (boss->state != BOSS_STATE_ACTIVE)
    {
        return 0;
    }

    if (current_time - boss->last_shot_time < boss->fire_delay)
    {
        return 0;
    }

    int use_spread;

    if (boss->phase >= 3)
    {
        use_spread = (boss->attack_index % 3 != 0);
    }
    else if (boss->phase == 2)
    {
        use_spread = (boss->attack_index % 2 == 1);
    }
    else
    {
        use_spread = 0;
    }

    int fired;

    if (use_spread)
    {
        fired = fire_spread(boss, bullets);
    }
    else
    {
        fired = fire_straight(boss, bullets);
    }

    if (fired)
    {
        boss->last_shot_time = current_time;
        boss->attack_index++;
    }

    return fired;
}

// Occasionally bring in a Scout as backup during CRITICAL.
void boss_spawn_support(
    Boss *boss,
    Enemy enemies[],
    Uint32 current_time
)
{
    if (boss->state != BOSS_STATE_ACTIVE || boss->phase < 3)
    {
        return;
    }

    enemies_spawn_scout(
        enemies,
        current_time,
        &boss->last_support_spawn,
        DREADNOUGHT_SUPPORT_SPAWN_DELAY,
        DREADNOUGHT_SUPPORT_FIRE_DELAY
    );
}

// Sets the draw color to flash-white while a phase transition flash
// is active, or the given color otherwise - avoids repeating the same
// if/else at every single shape in boss_render() below.
static void set_boss_draw_color(
    SDL_Renderer *renderer,
    int flashing,
    Uint8 r,
    Uint8 g,
    Uint8 b
)
{
    if (flashing)
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    }
}

// Draw the Dreadnought: a large armored warship built from filled
// blocks rather than the hand-drawn per-row outlines Scout/Bomber
// use - practical at this much bigger size while staying in the same
// retro, no-sprite style. Flashes white briefly after a phase
// transition (player_render() uses the same "call SDL_GetTicks()
// directly for a render-only blink" approach for invulnerability).
// Also flashes, much more briefly, on every ordinary bullet hit - see
// hit_flash_until in boss.h.
void boss_render(SDL_Renderer *renderer, const Boss *boss)
{
    // Nothing to draw before it's actually spawned (still just
    // WARNING), and nothing left to draw once inactive or once the
    // destruction sequence has actually finished - an undamaged-looking
    // ship sitting there after exploding reads as a bug, not a victory.
    if (boss->state == BOSS_STATE_INACTIVE ||
            boss->state == BOSS_STATE_WARNING ||
            boss->state == BOSS_STATE_DEFEATED)
    {
        return;
    }

    int x = (int)boss->x;
    int y = (int)boss->y;

    Uint32 current_time = SDL_GetTicks();

    int flashing =
        (current_time < boss->phase_flash_until) ||
        (current_time < boss->hit_flash_until);

    // Central armored hull spine.
    set_boss_draw_color(renderer, flashing, 90, 95, 110);
    SDL_Rect hull = { x + 14, y + 2, 12, 27 };
    SDL_RenderFillRect(renderer, &hull);

    // Upper and lower wings, swept back from the nose.
    SDL_Rect upper_wing = { x + 4, y, 14, 8 };
    SDL_RenderFillRect(renderer, &upper_wing);

    SDL_Rect lower_wing = { x + 4, y + 24, 14, 8 };
    SDL_RenderFillRect(renderer, &lower_wing);

    // Rear engine block - slightly lighter armor plating.
    set_boss_draw_color(renderer, flashing, 110, 115, 130);
    SDL_Rect engine_block = { x + 26, y + 10, 13, 12 };
    SDL_RenderFillRect(renderer, &engine_block);

    // Central nose cannon, protruding toward the player.
    set_boss_draw_color(renderer, flashing, 255, 120, 50);
    SDL_Rect nose_cannon = { x, y + 13, 14, 6 };
    SDL_RenderFillRect(renderer, &nose_cannon);

    // Side gun ports on each wing.
    SDL_Rect upper_gun = { x + 6, y + 2, 5, 4 };
    SDL_RenderFillRect(renderer, &upper_gun);

    SDL_Rect lower_gun = { x + 6, y + 26, 5, 4 };
    SDL_RenderFillRect(renderer, &lower_gun);

    // Glowing engine exhaust at the rear - from Phase 2 onward this
    // periodically cuts out entirely to read as battle damage, a pure
    // render-time flicker driven by the existing phase rather than any
    // new state. CRITICAL flickers faster and darker than BARRAGE, so
    // the engine visibly looks worse the more damaged the ship is.
    int engine_flickering = 0;

    if (boss->phase == 2)
    {
        engine_flickering = (current_time / 300) % 4 == 0;
    }
    else if (boss->phase >= 3)
    {
        engine_flickering = (current_time / 150) % 2 == 0;
    }

    if (!engine_flickering)
    {
        set_boss_draw_color(renderer, flashing, 255, 160, 60);
        SDL_Rect engine_glow = { x + 36, y + 13, 4, 6 };
        SDL_RenderFillRect(renderer, &engine_glow);
    }

    // Pale cockpit/bridge highlight.
    set_boss_draw_color(renderer, flashing, 200, 255, 220);
    SDL_Rect cockpit = { x + 17, y + 6, 4, 3 };
    SDL_RenderFillRect(renderer, &cockpit);

    // Static scorch marks - a couple more appear at Phase 3 than at
    // Phase 2, so the hull itself looks progressively more battered
    // independent of the periodic spark bursts boss_update() spawns.
    // Drawn last so they show up over the hull/wings/engine block
    // rather than being painted over by them.
    if (boss->phase >= 2)
    {
        SDL_SetRenderDrawColor(renderer, 40, 40, 45, 255);
        SDL_RenderDrawPoint(renderer, x + 18, y + 10);
        SDL_RenderDrawPoint(renderer, x + 30, y + 18);
    }

    if (boss->phase >= 3)
    {
        SDL_SetRenderDrawColor(renderer, 40, 40, 45, 255);
        SDL_RenderDrawPoint(renderer, x + 22, y + 20);
        SDL_RenderDrawPoint(renderer, x + 9, y + 5);
    }
}

// Layout for the boss health bar overlay - sits just below the HUD
// divider, not part of the permanently reserved HUD strip since it
// only ever appears during a boss fight.
#define BOSS_HEALTH_BAR_LABEL_Y 12
#define BOSS_HEALTH_BAR_Y 20
#define BOSS_HEALTH_BAR_X 20
#define BOSS_HEALTH_BAR_WIDTH 120
#define BOSS_HEALTH_BAR_HEIGHT 5

// Draws the "DREADNOUGHT" label and a shrinking health bar beneath it.
void boss_render_health_bar(SDL_Renderer *renderer, const Boss *boss)
{
    if (boss->state == BOSS_STATE_INACTIVE ||
            boss->state == BOSS_STATE_WARNING)
    {
        return;
    }

    const char *label = "DREADNOUGHT";

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    int label_x = (SCREEN_WIDTH - text_width(label, 1)) / 2;
    text_draw(renderer, label, label_x, BOSS_HEALTH_BAR_LABEL_Y, 1);

    // Ratio of remaining health, clamped so rounding or an
    // over-damage frame can never push the bar past empty or full.
    float ratio = (float)boss->health / (float)boss->max_health;

    if (ratio < 0.0f)
    {
        ratio = 0.0f;
    }

    if (ratio > 1.0f)
    {
        ratio = 1.0f;
    }

    SDL_Rect border =
    {
        BOSS_HEALTH_BAR_X,
        BOSS_HEALTH_BAR_Y,
        BOSS_HEALTH_BAR_WIDTH,
        BOSS_HEALTH_BAR_HEIGHT
    };

    SDL_RenderDrawRect(renderer, &border);

    int filled_width = (int)((float)BOSS_HEALTH_BAR_WIDTH * ratio);

    if (filled_width > 0)
    {
        SDL_SetRenderDrawColor(renderer, 220, 60, 60, 255);

        SDL_Rect fill =
        {
            BOSS_HEALTH_BAR_X,
            BOSS_HEALTH_BAR_Y,
            filled_width,
            BOSS_HEALTH_BAR_HEIGHT
        };

        SDL_RenderFillRect(renderer, &fill);
    }
}

// Y positions for the two-line warning announcement, and how fast
// "WARNING" blinks - fast enough to read as urgent/alarm-like rather
// than the slower, calmer blink player_render() uses for invulnerability.
#define WARNING_LINE1_Y 45
#define WARNING_LINE2_Y 63
#define WARNING_BLINK_INTERVAL_MS 200

// Draws the blinking "WARNING" / "DREADNOUGHT" announcement.
void boss_render_warning(SDL_Renderer *renderer, const Boss *boss)
{
    if (boss->state != BOSS_STATE_WARNING)
    {
        return;
    }

    // "WARNING" blinks in red - urgent and visually distinct from
    // every other piece of white HUD/announcement text in the game.
    // "DREADNOUGHT" stays solid underneath it so the player always has
    // something readable to look at, even mid-blink.
    Uint32 current_time = SDL_GetTicks();
    int warning_visible =
        (current_time / WARNING_BLINK_INTERVAL_MS) % 2 == 0;

    if (warning_visible)
    {
        SDL_SetRenderDrawColor(renderer, 220, 40, 40, 255);

        const char *line1 = "WARNING";
        int line1_scale = 2;
        int line1_x = (SCREEN_WIDTH - text_width(line1, line1_scale)) / 2;
        text_draw(renderer, line1, line1_x, WARNING_LINE1_Y, line1_scale);
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    const char *line2 = "DREADNOUGHT";
    int line2_scale = 1;
    int line2_x = (SCREEN_WIDTH - text_width(line2, line2_scale)) / 2;
    text_draw(renderer, line2, line2_x, WARNING_LINE2_Y, line2_scale);
}

// Y positions for the two-line defeat message, matching the spacing
// wave_render_announcement() already uses for its own two lines.
#define DEFEAT_MESSAGE_LINE1_Y 45
#define DEFEAT_MESSAGE_LINE2_Y 63

// Draws the "DREADNOUGHT / DESTROYED" completion message.
void boss_render_defeat_message(SDL_Renderer *renderer, const Boss *boss)
{
    if (boss->state != BOSS_STATE_DEFEATED)
    {
        return;
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    const char *line1 = "DREADNOUGHT";
    int line1_scale = 2;
    int line1_x = (SCREEN_WIDTH - text_width(line1, line1_scale)) / 2;
    text_draw(renderer, line1, line1_x, DEFEAT_MESSAGE_LINE1_Y, line1_scale);

    const char *line2 = "DESTROYED";
    int line2_scale = 1;
    int line2_x = (SCREEN_WIDTH - text_width(line2, line2_scale)) / 2;
    text_draw(renderer, line2, line2_x, DEFEAT_MESSAGE_LINE2_Y, line2_scale);
}
