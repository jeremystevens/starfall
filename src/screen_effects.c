#include "screen_effects.h"

#include "game_config.h"

#include <stdlib.h>

// Reset to idle.
void screen_effects_init(ScreenEffects *effects)
{
    effects->shake_magnitude = 0;
    effects->shake_duration_ms = 0;
    effects->shake_until = 0;

    effects->flash_r = 0;
    effects->flash_g = 0;
    effects->flash_b = 0;
    effects->flash_alpha = 0;
    effects->flash_duration_ms = 0;
    effects->flash_until = 0;
}

// Start (or replace) a shake, unless a stronger one is already
// playing.
void screen_effects_shake(
    ScreenEffects *effects,
    int magnitude,
    Uint32 duration_ms,
    Uint32 current_time
)
{
    int already_shaking = current_time < effects->shake_until;

    if (already_shaking && magnitude < effects->shake_magnitude)
    {
        return;
    }

    effects->shake_magnitude = magnitude;
    effects->shake_duration_ms = duration_ms;
    effects->shake_until = current_time + duration_ms;
}

// Compute this frame's shake offset - a fresh random jitter each
// call, scaled down as the shake nears its end.
void screen_effects_get_shake_offset(
    const ScreenEffects *effects,
    Uint32 current_time,
    int *offset_x,
    int *offset_y
)
{
    if (current_time >= effects->shake_until)
    {
        *offset_x = 0;
        *offset_y = 0;
        return;
    }

    Uint32 remaining = effects->shake_until - current_time;

    float fraction =
        (effects->shake_duration_ms > 0)
        ? (float)remaining / (float)effects->shake_duration_ms
        : 0.0f;

    int current_magnitude = (int)((float)effects->shake_magnitude * fraction);

    if (current_magnitude <= 0)
    {
        *offset_x = 0;
        *offset_y = 0;
        return;
    }

    // +1 so rand() % range can land on both -current_magnitude and
    // +current_magnitude, not just up to one side.
    int range = current_magnitude * 2 + 1;

    *offset_x = (rand() % range) - current_magnitude;
    *offset_y = (rand() % range) - current_magnitude;
}

// Start (or replace) a full-screen flash.
void screen_effects_flash(
    ScreenEffects *effects,
    Uint8 r,
    Uint8 g,
    Uint8 b,
    Uint8 alpha,
    Uint32 duration_ms,
    Uint32 current_time
)
{
    effects->flash_r = r;
    effects->flash_g = g;
    effects->flash_b = b;
    effects->flash_alpha = alpha;
    effects->flash_duration_ms = duration_ms;
    effects->flash_until = current_time + duration_ms;
}

// Draw the current flash, if one is active, fading linearly from
// flash_alpha to 0 as it approaches flash_until.
void screen_effects_render_flash(
    SDL_Renderer *renderer,
    const ScreenEffects *effects,
    Uint32 current_time
)
{
    if (current_time >= effects->flash_until)
    {
        return;
    }

    Uint32 remaining = effects->flash_until - current_time;

    float fraction =
        (effects->flash_duration_ms > 0)
        ? (float)remaining / (float)effects->flash_duration_ms
        : 0.0f;

    Uint8 alpha = (Uint8)((float)effects->flash_alpha * fraction);

    // Restore whatever blend mode the renderer had before this call -
    // nothing else in the project ever changes it away from the
    // default, so this guarantees the renderer is left exactly as it
    // was found regardless of whether a flash happened to be active.
    SDL_BlendMode previous_blend_mode;
    SDL_GetRenderDrawBlendMode(renderer, &previous_blend_mode);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, effects->flash_r, effects->flash_g, effects->flash_b, alpha);

    SDL_Rect full_screen = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &full_screen);

    SDL_SetRenderDrawBlendMode(renderer, previous_blend_mode);
}
