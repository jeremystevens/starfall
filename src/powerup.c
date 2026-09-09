#include "powerup.h"

#include <stdio.h>

#include "game_config.h"
#include "text.h"

// Reset all power-up effects to inactive.
void powerup_state_init(PowerUpState *state)
{
    state->rapid_fire_active = 0;
    state->rapid_fire_until = 0;

    state->spread_shot_active = 0;
    state->spread_shot_until = 0;

    state->shield_active = 0;
}

// Activate (or refresh) the effect for a collected power-up type.
void powerup_state_collect(
    PowerUpState *state,
    PowerUpType type,
    Uint32 current_time
)
{
    switch (type)
    {
    case POWERUP_RAPID_FIRE:
        state->rapid_fire_active = 1;
        state->rapid_fire_until = current_time + RAPID_FIRE_DURATION;
        break;

    case POWERUP_SPREAD_SHOT:
        state->spread_shot_active = 1;
        state->spread_shot_until = current_time + SPREAD_SHOT_DURATION;
        break;

    case POWERUP_SHIELD:
        // Charge-based, not timed - collecting again while already
        // active just leaves the player with the one charge.
        state->shield_active = 1;
        break;
    }

    printf("Power-up collected: %s\n", powerup_type_name(type));
}

// Expire any timed effects whose duration has elapsed. Shield isn't
// timed, so it isn't touched here - it's removed elsewhere when it
// absorbs a hit.
void powerup_state_update(
    PowerUpState *state,
    Uint32 current_time
)
{
    if (state->rapid_fire_active && current_time >= state->rapid_fire_until)
    {
        state->rapid_fire_active = 0;
    }

    if (state->spread_shot_active && current_time >= state->spread_shot_until)
    {
        state->spread_shot_active = 0;
    }
}

// Short HUD-friendly label for a power-up type.
const char *powerup_type_name(PowerUpType type)
{
    switch (type)
    {
    case POWERUP_RAPID_FIRE:
        return "RAPID";

    case POWERUP_SPREAD_SHOT:
        return "SPREAD";

    case POWERUP_SHIELD:
    default:
        return "SHIELD";
    }
}

// How much of a timed effect's duration remains, in milliseconds.
Uint32 powerup_time_remaining(Uint32 until, Uint32 current_time)
{
    if (until <= current_time)
    {
        return 0;
    }

    return until - current_time;
}

// Layout for a compact HUD indicator: a single letter, a small gap,
// then a bordered bar - small enough that two of them fit in the dead
// space between the score and lives text without growing the HUD.
#define POWERUP_BAR_OFFSET 6
#define POWERUP_BAR_WIDTH 14
#define POWERUP_BAR_HEIGHT 5

// Below this fraction of remaining time, the indicator blinks as a
// simple "about to expire" warning.
#define POWERUP_LOW_TIME_RATIO 0.25f

// How fast the low-time warning blinks.
#define POWERUP_BLINK_INTERVAL 200

// Draws one compact HUD indicator - see powerup.h for the full
// contract.
void powerup_hud_bar_render(
    SDL_Renderer *renderer,
    int x,
    int y,
    char letter,
    Uint8 r,
    Uint8 g,
    Uint8 b,
    Uint32 remaining_time,
    Uint32 total_duration,
    Uint32 current_time
)
{
    float ratio = (float)remaining_time / (float)total_duration;

    if (ratio < 0.0f)
    {
        ratio = 0.0f;
    }

    if (ratio > 1.0f)
    {
        ratio = 1.0f;
    }

    // Blink the whole indicator when time is running low, rather than
    // drawing a barely-visible sliver of a bar every frame.
    if (ratio <= POWERUP_LOW_TIME_RATIO &&
            (current_time / POWERUP_BLINK_INTERVAL) % 2 == 0)
    {
        return;
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    text_draw_char(renderer, letter, x, y, 1);

    // Bar is a couple pixels shorter than the letter's 7px glyph
    // height and vertically centered against it.
    SDL_Rect border =
    {
        x + POWERUP_BAR_OFFSET,
        y + 1,
        POWERUP_BAR_WIDTH,
        POWERUP_BAR_HEIGHT
    };

    SDL_RenderDrawRect(renderer, &border);

    int filled_width = (int)((float)POWERUP_BAR_WIDTH * ratio);

    if (filled_width > 0)
    {
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);

        SDL_Rect fill =
        {
            x + POWERUP_BAR_OFFSET,
            y + 1,
            filled_width,
            POWERUP_BAR_HEIGHT
        };

        SDL_RenderFillRect(renderer, &fill);
    }
}

// How far the shield bubble extends past the player's bounding box,
// and how much its corners get clipped to read as a rounded bubble
// rather than a plain rectangle.
#define SHIELD_MARGIN 2
#define SHIELD_CORNER_CUT 3

// Draws a simple octagonal energy-field outline around the player -
// a small SDL primitive, not detailed artwork, but obvious at a
// glance that Shield is active.
void powerup_render_shield(
    SDL_Renderer *renderer,
    float player_x,
    float player_y,
    int player_width,
    int player_height
)
{
    Uint8 r, g, b;
    powerup_color(POWERUP_SHIELD, &r, &g, &b);
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);

    int x0 = (int)player_x - SHIELD_MARGIN;
    int y0 = (int)player_y - SHIELD_MARGIN;
    int w = player_width + (SHIELD_MARGIN * 2);
    int h = player_height + (SHIELD_MARGIN * 2);
    int c = SHIELD_CORNER_CUT;

    // Top, right, bottom, left edges with each corner cut by a short
    // diagonal, forming a rounded "bubble" outline.
    SDL_RenderDrawLine(renderer, x0 + c, y0, x0 + w - c, y0);
    SDL_RenderDrawLine(renderer, x0 + w - c, y0, x0 + w, y0 + c);
    SDL_RenderDrawLine(renderer, x0 + w, y0 + c, x0 + w, y0 + h - c);
    SDL_RenderDrawLine(renderer, x0 + w, y0 + h - c, x0 + w - c, y0 + h);
    SDL_RenderDrawLine(renderer, x0 + w - c, y0 + h, x0 + c, y0 + h);
    SDL_RenderDrawLine(renderer, x0 + c, y0 + h, x0, y0 + h - c);
    SDL_RenderDrawLine(renderer, x0, y0 + h - c, x0, y0 + c);
    SDL_RenderDrawLine(renderer, x0, y0 + c, x0 + c, y0);
}

// Every pickup is the same small size regardless of type.
#define POWERUP_WIDTH 7
#define POWERUP_HEIGHT 7

// Pickups drift left slowly - noticeably slower than enemies or
// bullets, so the player has time to react and go collect one.
#define POWERUP_SPEED 0.5f


// Initialize the power-up pool. All pickups start inactive.
void powerups_init(PowerUp powerups[])
{
    for (int i = 0; i < MAX_POWERUPS; i++)
    {
        powerups[i].active = 0;
    }
}


// Spawn a pickup of the given type at (x, y) using a free pool slot.
void powerups_spawn(
    PowerUp powerups[],
    float x,
    float y,
    PowerUpType type
)
{
    for (int i = 0; i < MAX_POWERUPS; i++)
    {
        if (powerups[i].active)
        {
            continue;
        }

        powerups[i].type = type;

        powerups[i].width = POWERUP_WIDTH;
        powerups[i].height = POWERUP_HEIGHT;

        powerups[i].x = x;
        powerups[i].y = y;

        // Keep pickups out of the HUD strip, same rule enemies follow.
        if (powerups[i].y < HUD_HEIGHT)
        {
            powerups[i].y = HUD_HEIGHT;
        }

        powerups[i].dx = -POWERUP_SPEED;

        powerups[i].active = 1;

        return;
    }

    // Pool was full - the drop is silently skipped.
}


// Move active pickups and recycle any that drift off the left edge.
void powerups_update(PowerUp powerups[])
{
    for (int i = 0; i < MAX_POWERUPS; i++)
    {
        if (!powerups[i].active)
        {
            continue;
        }

        powerups[i].x += powerups[i].dx;

        if (powerups[i].x + powerups[i].width < 0)
        {
            powerups[i].active = 0;
        }
    }
}


// Look up the color used to represent a power-up type. Centralized
// here so the pickup icon and its HUD countdown bar can't drift out
// of sync with each other.
void powerup_color(
    PowerUpType type,
    Uint8 *r,
    Uint8 *g,
    Uint8 *b
)
{
    switch (type)
    {
    case POWERUP_RAPID_FIRE:
        *r = 255;
        *g = 180;
        *b = 40;
        break;

    case POWERUP_SPREAD_SHOT:
        *r = 80;
        *g = 200;
        *b = 255;
        break;

    case POWERUP_SHIELD:
    default:
        *r = 120;
        *g = 255;
        *b = 140;
        break;
    }
}

// Look up the single letter drawn on a power-up's pickup icon.
char powerup_letter(PowerUpType type)
{
    switch (type)
    {
    case POWERUP_RAPID_FIRE:
        return 'R';

    case POWERUP_SPREAD_SHOT:
        return '3';

    case POWERUP_SHIELD:
    default:
        return 'S';
    }
}

// Draw every active pickup as a small colored capsule with a letter
// identifying its type, reusing the existing 5x7 bitmap font instead
// of hand-drawn icon art.
void powerups_render(
    SDL_Renderer *renderer,
    const PowerUp powerups[]
)
{
    for (int i = 0; i < MAX_POWERUPS; i++)
    {
        if (!powerups[i].active)
        {
            continue;
        }

        int x = (int)powerups[i].x;
        int y = (int)powerups[i].y;

        Uint8 r, g, b;

        powerup_color(powerups[i].type, &r, &g, &b);
        char letter = powerup_letter(powerups[i].type);

        SDL_Rect background =
        {
            x,
            y,
            powerups[i].width,
            powerups[i].height
        };

        // Colored capsule background so each pickup reads as a
        // distinct object even before the letter is legible.
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderFillRect(renderer, &background);

        // Dark outline to help the icon stand out from the starfield.
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderDrawRect(renderer, &background);

        // Letter identifying the power-up type, centered in the icon
        // (the 5-wide glyph gets 1px of padding on each side of the
        // 7-wide capsule).
        text_draw_char(renderer, letter, x + 1, y, 1);
    }
}
