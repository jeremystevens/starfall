#ifndef POPUP_H
#define POPUP_H

#include <SDL.h>

#define MAX_SCORE_POPUPS 16

// A single floating "+N" score readout, spawned at the exact moment
// and position points are awarded elsewhere, so the player sees what
// a kill was worth without having to do the math themselves. This
// module only ever visualizes a value it's handed - it never computes
// or awards score itself (see popups_spawn()'s comment).
typedef struct
{
    float x;
    float y;

    int value;

    Uint32 spawn_time;
    Uint32 duration_ms;
    int scale;

    int active;

} ScorePopup;

// Initialize the popup pool. All popups start inactive.
void popups_init(ScorePopup popups[]);

// Spawn a floating "+value" popup at (x, y) using the default
// duration/scale. Purely visual - the caller must have already
// awarded this score itself; this call never changes score, only
// echoes a value the caller already added. If the pool is full, the
// popup is silently skipped - a missed visual must never affect
// scoring, so this deliberately has no failure return to check.
void popups_spawn(
    ScorePopup popups[],
    float x,
    float y,
    int value,
    Uint32 current_time
);

// Same as popups_spawn(), but bigger and longer-lived - for a reward
// significant enough that it should read as more important than an
// ordinary kill (e.g. a boss defeat), without needing a second pool
// or rendering path.
void popups_spawn_emphasized(
    ScorePopup popups[],
    float x,
    float y,
    int value,
    Uint32 current_time
);

// Drift every active popup upward and deactivate any whose
// duration_ms has elapsed.
void popups_update(ScorePopup popups[], Uint32 current_time);

// Draw every active popup as "+value", fading bright -> dim as it
// ages (see the fixed color tiers in popup.c) - the same three-tier
// fraction-based approach explosion.c already uses for its own
// fade-out, rather than true alpha blending.
void popups_render(
    SDL_Renderer *renderer,
    const ScorePopup popups[],
    Uint32 current_time
);

#endif
