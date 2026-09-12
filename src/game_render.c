#include "game_render.h"

#include <stdio.h>

#include "text.h"
#include "game_config.h"

// Draws text horizontally centered on the 160px-wide logical screen at
// the given y and scale - the "measure with text_width(), then center"
// pattern every piece of centered text below already used individually
// (v0.8.0 main.c refactor, Phase 6). Reduces eleven near-identical
// x-calculation/text_draw pairs to one call each without hiding any of
// their real differences - each call site still picks its own string,
// y, and scale.
static void draw_centered_text(
    SDL_Renderer *renderer,
    const char *text,
    int y,
    int scale
)
{
    int x = (SCREEN_WIDTH - text_width(text, scale)) / 2;
    text_draw(renderer, text, x, y, scale);
}

// Render one complete frame. Moved out of main.c as-is (v0.8.0 main.c
// refactor, Phase 2) - every coordinate, scale, string, centering
// calculation, and color below is unchanged from the original inline
// block; only the source of each value changed, from a local variable
// to a *ctx field.
void game_render_frame(SDL_Renderer *renderer, const RenderContext *ctx)
{
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
        ctx->screen_effects,
        ctx->now,
        &shake_x,
        &shake_y
    );

    SDL_Rect shake_viewport = { shake_x, shake_y, SCREEN_WIDTH, SCREEN_HEIGHT };
    SDL_RenderSetViewport(renderer, &shake_viewport);

    starfield_render(renderer, ctx->stars);

    // Title screen text - drawn over the moving starfield in
    // place of any gameplay entities. Every line is centered with
    // text_width(), the same "measure, then center" pattern
    // boss_render_health_bar() and wave_render_announcement()
    // already use, so nothing here hardcodes an x position for a
    // string of a specific length.
    if (ctx->game_state == GAME_TITLE)
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        draw_centered_text(renderer, "starfall", 16, 2);

        // High score label and value as two separate centered
        // lines - the value's width changes with its digit count
        // (0 vs. 99999 vs. a much bigger run), so it gets its own
        // text_width() call (inside draw_centered_text()) rather
        // than assuming a fixed position relative to the label
        // above it.
        draw_centered_text(renderer, "HIGH SCORE", 36, 1);

        char high_score_text[16];
        snprintf(
            high_score_text,
            sizeof(high_score_text),
            "%d",
            ctx->high_score
        );
        draw_centered_text(renderer, high_score_text, 46, 1);

        draw_centered_text(renderer, "PRESS SPACE", 62, 1);
        draw_centered_text(renderer, "ARROWS MOVE", 92, 1);
        draw_centered_text(renderer, "SPACE FIRE", 102, 1);
    }

    // Everything below is gameplay presentation - the ship, HUD,
    // enemies, and every other entity pool. None of it should
    // appear on the title screen, which is just the starfield
    // (and the title text drawn above). GAME_OVER still draws
    // through this block so the battlefield stays visible (frozen)
    // behind the "GAME OVER" text.
    if (ctx->game_state != GAME_TITLE)
    {
        // Build the HUD strings for this frame.
        char score_text[32];
        char lives_text[32];

        snprintf(
            score_text,
            sizeof(score_text),
            "SCORE %d",
            ctx->score
        );

        snprintf(
            lives_text,
            sizeof(lives_text),
            "LIVES %d",
            ctx->player->lives
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
        if (ctx->powerup_state->rapid_fire_active)
        {
            Uint8 rapid_r, rapid_g, rapid_b;
            powerup_color(POWERUP_RAPID_FIRE, &rapid_r, &rapid_g, &rapid_b);

            powerup_hud_bar_render(
                renderer,
                POWERUP_HUD_RAPID_X,
                POWERUP_HUD_Y,
                powerup_letter(POWERUP_RAPID_FIRE),
                rapid_r, rapid_g, rapid_b,
                powerup_time_remaining(ctx->powerup_state->rapid_fire_until, ctx->now),
                RAPID_FIRE_DURATION,
                ctx->now
            );
        }

        // Spread Shot countdown indicator - same layout as Rapid
        // Fire's, in its own reserved slot further right.
        if (ctx->powerup_state->spread_shot_active)
        {
            Uint8 spread_r, spread_g, spread_b;
            powerup_color(POWERUP_SPREAD_SHOT, &spread_r, &spread_g, &spread_b);

            powerup_hud_bar_render(
                renderer,
                POWERUP_HUD_SPREAD_X,
                POWERUP_HUD_Y,
                powerup_letter(POWERUP_SPREAD_SHOT),
                spread_r, spread_g, spread_b,
                powerup_time_remaining(ctx->powerup_state->spread_shot_until, ctx->now),
                SPREAD_SHOT_DURATION,
                ctx->now
            );
        }

        // Shield HUD indicator - just the letter, since Shield is a
        // charge rather than a timer, so there's no ratio to show a
        // bar for. The energy-field bubble around the ship (drawn
        // after player_render() below) is the primary "shield is
        // active" signal; this is a small supporting HUD cue.
        if (ctx->powerup_state->shield_active)
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

        bullets_render(renderer, ctx->bullets);
        enemies_render(renderer, ctx->enemies);
        asteroids_render(renderer, ctx->asteroids);
        powerups_render(renderer, ctx->powerups);
        enemy_bullets_render(renderer, ctx->enemy_bullets);
        boss_render(renderer, ctx->boss);

        // Draw the player.
        player_render(renderer, ctx->player);

        // Draw the Shield energy-field bubble around the ship while
        // the effect is active.
        if (ctx->powerup_state->shield_active)
        {
            powerup_render_shield(
                renderer,
                ctx->player->x,
                ctx->player->y,
                ctx->player->width,
                ctx->player->height
            );
        }

        // Draw any active explosions on top of everything else.
        explosions_render(renderer, ctx->explosions);

        // Floating "+value" score popups, drawn on top of explosions
        // so a popup is never hidden behind the burst it came from.
        popups_render(renderer, ctx->score_popups, ctx->now);

        // Boss health bar overlay - drawn on top of gameplay like the
        // wave announcement, not part of the permanently reserved HUD
        // strip. A no-op whenever no boss is present.
        boss_render_health_bar(renderer, ctx->boss);

        // Defeat completion message - shows for WAVE_COMPLETE_DELAY_MS
        // after the destruction sequence finishes, then disappears
        // once main.c resets the boss back to inactive.
        boss_render_defeat_message(renderer, ctx->boss);

        // Blinking "WARNING / DREADNOUGHT" announcement, shown only
        // during BOSS_STATE_WARNING, before the boss has actually
        // appeared.
        boss_render_warning(renderer, ctx->boss);

        // Brief "EXTRA LIFE" readout - a no-op outside the short
        // window player_check_extra_life() just armed.
        player_render_extra_life_notification(renderer, ctx->player);

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
    if ((ctx->game_state == GAME_PLAYING || ctx->game_state == GAME_PAUSED) &&
            ctx->boss->state != BOSS_STATE_WARNING)
    {
        wave_render_announcement(renderer, ctx->wave);
    }

    // PAUSED overlay - drawn on top of the frozen battlefield the
    // same way GAME_OVER's text is, so it's obvious the game has
    // stopped responding to gameplay input on purpose rather than
    // hung.
    if (ctx->game_state == GAME_PAUSED)
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        draw_centered_text(renderer, "PAUSED", 50, 2);
    }

    if (ctx->game_state == GAME_OVER)
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        draw_centered_text(renderer, "GAME OVER", 30, 2);

        // Final score and high score both need their own
        // text_width() call (inside draw_centered_text()) rather
        // than a shared/hardcoded x - "SCORE 40" and "SCORE 142999"
        // don't take up the same width, and the two lines can each
        // be centered independently of each other.
        char final_score_text[32];
        snprintf(
            final_score_text,
            sizeof(final_score_text),
            "SCORE %d",
            ctx->score
        );
        draw_centered_text(renderer, final_score_text, 52, 1);

        // In the normal case this is "HIGH SCORE <n>", same as
        // before. On a new record it becomes "NEW HIGH SCORE!"
        // instead - score above already shows the value, which
        // now equals high_score, so repeating the number a
        // second time on this line would be redundant rather
        // than more informative.
        char high_score_line_text[32];

        if (ctx->new_high_score)
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
                ctx->high_score
            );
        }

        draw_centered_text(renderer, high_score_line_text, 62, 1);
        draw_centered_text(renderer, "PRESS ENTER", 82, 1);
    }

    // Full-screen flash overlay, if one is active - drawn last so
    // it tints everything else already on screen this frame. A
    // no-op the rest of the time.
    screen_effects_render_flash(renderer, ctx->screen_effects, ctx->now);

    // Display the completed frame in the window.
    SDL_RenderPresent(renderer);
}
