#include "popup.h"

#include "text.h"

#include <stdio.h>

// Default popup: brief and small, for an ordinary kill.
#define SCORE_POPUP_DURATION_MS 700
#define SCORE_POPUP_SCALE 1

// Emphasized popup: noticeably longer and bigger, reserved for a
// reward significant enough that it should read as a bigger deal than
// an ordinary kill (see popups_spawn_emphasized()).
#define SCORE_POPUP_EMPHASIZED_DURATION_MS 1400
#define SCORE_POPUP_EMPHASIZED_SCALE 2

// How many logical pixels a popup rises per frame. A per-frame
// constant rather than a time-based rate, matching how every other
// moving entity in this project (asteroids, bullets, enemies) already
// advances position by a fixed amount each update() call.
#define SCORE_POPUP_RISE_SPEED 0.25f

// Initialize the popup pool. All popups start inactive.
void popups_init(ScorePopup popups[])
{
    for (int i = 0; i < MAX_SCORE_POPUPS; i++)
    {
        popups[i].active = 0;
    }
}

// Shared by both spawn functions below - fills in the first free slot
// it finds, or does nothing at all if the pool is full.
static void popup_spawn_in_free_slot(
    ScorePopup popups[],
    float x,
    float y,
    int value,
    Uint32 duration_ms,
    int scale,
    Uint32 current_time
)
{
    for (int i = 0; i < MAX_SCORE_POPUPS; i++)
    {
        if (popups[i].active)
        {
            continue;
        }

        popups[i].x = x;
        popups[i].y = y;
        popups[i].value = value;
        popups[i].spawn_time = current_time;
        popups[i].duration_ms = duration_ms;
        popups[i].scale = scale;
        popups[i].active = 1;

        return;
    }

    // Pool full - silently skip. A missed visual popup must never be
    // treated as an error, and it must never affect the score the
    // caller already awarded elsewhere.
}

// Spawn a normal "+value" popup.
void popups_spawn(
    ScorePopup popups[],
    float x,
    float y,
    int value,
    Uint32 current_time
)
{
    popup_spawn_in_free_slot(
        popups,
        x,
        y,
        value,
        SCORE_POPUP_DURATION_MS,
        SCORE_POPUP_SCALE,
        current_time
    );
}

// Spawn a bigger, longer-lived "+value" popup.
void popups_spawn_emphasized(
    ScorePopup popups[],
    float x,
    float y,
    int value,
    Uint32 current_time
)
{
    popup_spawn_in_free_slot(
        popups,
        x,
        y,
        value,
        SCORE_POPUP_EMPHASIZED_DURATION_MS,
        SCORE_POPUP_EMPHASIZED_SCALE,
        current_time
    );
}

// Drift every active popup upward and retire any whose time is up.
void popups_update(ScorePopup popups[], Uint32 current_time)
{
    for (int i = 0; i < MAX_SCORE_POPUPS; i++)
    {
        if (!popups[i].active)
        {
            continue;
        }

        popups[i].y -= SCORE_POPUP_RISE_SPEED;

        if (current_time - popups[i].spawn_time >= popups[i].duration_ms)
        {
            popups[i].active = 0;
        }
    }
}

// Draw every active popup as "+value", centered on its x position (so
// it reads as hovering over where the kill happened rather than
// trailing off to one side), fading from a bright flash through gold
// to a dim ember as it ages - the same "three fixed color tiers by
// remaining-life fraction" trick explosion.c already uses, rather
// than true alpha blending the existing renderer doesn't otherwise use.
void popups_render(
    SDL_Renderer *renderer,
    const ScorePopup popups[],
    Uint32 current_time
)
{
    for (int i = 0; i < MAX_SCORE_POPUPS; i++)
    {
        if (!popups[i].active)
        {
            continue;
        }

        float elapsed = (float)(current_time - popups[i].spawn_time);
        float fraction = 1.0f - (elapsed / (float)popups[i].duration_ms);

        if (fraction < 0.0f)
        {
            fraction = 0.0f;
        }

        Uint8 r, g, b;

        if (fraction > 0.66f)
        {
            r = 255;
            g = 255;
            b = 180;
        }
        else if (fraction > 0.33f)
        {
            r = 255;
            g = 215;
            b = 80;
        }
        else
        {
            r = 120;
            g = 100;
            b = 40;
        }

        SDL_SetRenderDrawColor(renderer, r, g, b, 255);

        char text[16];
        snprintf(text, sizeof(text), "+%d", popups[i].value);

        int x = (int)popups[i].x - text_width(text, popups[i].scale) / 2;
        int y = (int)popups[i].y;

        text_draw(renderer, text, x, y, popups[i].scale);
    }
}
