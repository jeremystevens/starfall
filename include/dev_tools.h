#ifndef DEV_TOOLS_H
#define DEV_TOOLS_H

#include <SDL.h>

// The developer/debug overlay (v0.9.0). This header is the PRIMARY
// place in the project that checks STARFALL_DEV_TOOLS: every function
// below is called unconditionally from main.c/game_render.c, which
// never test the macro themselves, typechecking identically in both
// builds via the dual dev/stub branches further down. That alone was
// the whole story through Phase 6.
//
// Phase 8's release compile-out audit went further: some
// developer-only functionality lives on structs (Player) or in files
// (main.c, game_render.c) this header can't reach into without either
// breaking "dev_tools.c never touches gameplay structs directly" or
// reintroducing the exact circular header dependency this design
// already avoids elsewhere (game_render.h includes this header for
// the DevTools type; the reverse would create a cycle). For those
// specific cases - Player.dev_invulnerable and its check in
// player_take_damage(), and main.c's/game_render.c's own dev-action
// dispatch and diagnostic-overlay code, each called out in its own
// comment where it appears - a small number of additional, clearly
// marked #ifdef STARFALL_DEV_TOOLS regions exist directly in those
// files instead. This header remains the concentration point for
// everything that CAN live here (all the shared enums and functions
// below); those few exceptions exist only where "everything checks
// the macro nowhere else" would otherwise force choosing between
// leaving genuinely removable state/code compiled into release, or a
// disproportionate redesign just to avoid a handful of well-commented
// ifdefs.
//
// This is deliberately kept to the smallest surface each phase
// actually needs, rather than pre-declaring a large API up front -
// Phase 2 only needed open/closed state and a render call; Phase 3
// adds real navigation and the action-dispatch boundary below. Each
// addition, and its stub counterpart, grows here incrementally as it
// actually gets wired into main.c, so there's never unused
// scaffolding sitting in either build.
//
// dev_tools.c owns panel UI state only (open/closed, menu selection)
// and never touches gameplay structs directly - it only ever reports
// WHICH action the player picked (see DevAction below) and WHETHER a
// grave-key press should open/close the panel (see DevToggleResult
// and dev_tools_handle_toggle() below). main.c decides whether/when
// that's legal and performs it - including the actual
// runtime_begin_pause()/runtime_end_pause() calls - against
// GameRuntime/Player/Boss/WaveState/etc. itself. That split is what
// keeps normal gameplay modules free of any dev_tools dependency, and
// is also what makes it architecturally impossible (not just
// unlikely) for a release build to pause gameplay through grave: the
// open/close decision itself lives here, not in main.c.

// Every action the developer panel can ever request. Deliberately a
// real, explicit identifier - never a raw menu index - so main.c's
// dispatch switch (see its own comment) stays meaningful and the
// compiler can warn (-Wswitch) if a future action is added here but
// never handled there. Declared unconditionally, in both builds: it's
// pure type/name declaration with no code behind it, so
// dev_tools_handle_key()/dev_tools_handle_toggle()/
// dev_tools_handle_legacy_shortcuts() all typecheck identically
// whether they're the real dev-branch logic or the release stubs that
// can only ever return/pass DEV_ACTION_NONE, and dev_action_requested
// itself can be a plain variable in both builds. main.c's own
// dispatch switch that actually acts on a non-NONE value is a
// separate matter - see its own comment for why that switch (unlike
// this enum) is wrapped in its own #ifdef STARFALL_DEV_TOOLS in
// main.c (v0.9.0 Phase 8): a couple of its cases reference fields
// (Player.dev_invulnerable, DevTools.show_hitboxes/show_stats) that a
// release build no longer carries at all.
typedef enum
{
    DEV_ACTION_NONE, // Nothing to do this frame - the normal case.

    DEV_ACTION_WAVE_PREVIOUS,
    DEV_ACTION_WAVE_NEXT,
    DEV_ACTION_WAVE_JUMP_5,
    DEV_ACTION_WAVE_NEXT_BOSS,

    DEV_ACTION_PLAYER_ADD_LIFE,
    DEV_ACTION_PLAYER_TOGGLE_INVULNERABLE,

    DEV_ACTION_POWERUP_SPAWN_RAPID,
    DEV_ACTION_POWERUP_SPAWN_SPREAD,
    DEV_ACTION_POWERUP_SPAWN_SHIELD,

    DEV_ACTION_BOSS_SKIP,

    DEV_ACTION_DIAG_TOGGLE_HITBOXES,
    DEV_ACTION_DIAG_TOGGLE_STATS

} DevAction;

// What the grave/backtick key's press this frame should do to gameplay
// pause state, decided entirely by dev_tools_handle_toggle() below -
// never by main.c inspecting DevTools itself. Declared unconditionally
// for the same reason DevAction is: main.c's grave handling must
// typecheck identically in both builds, calling
// runtime_begin_pause()/runtime_end_pause() only in response to
// DEV_TOGGLE_OPEN/DEV_TOGGLE_CLOSE - which the release stub below can
// simply never produce, making it architecturally impossible (not just
// unlikely) for grave to reach either pause function when the toolkit
// is absent.
typedef enum
{
    DEV_TOGGLE_NONE,  // Not a legitimate open/close request - do nothing.
    DEV_TOGGLE_OPEN,  // Begin freezing gameplay - the panel just opened.
    DEV_TOGGLE_CLOSE  // End the freeze - the panel just closed.

} DevToggleResult;

#ifdef STARFALL_DEV_TOOLS

// Panel UI state: open/closed, whether the group list or a group's
// item list is currently showing, which group is highlighted/entered,
// and the cursor position at whichever level is currently displayed.
// Deliberately just presentation/selection state; no gameplay data
// lives here - see DevAction above for how a real action gets
// reported instead of dev_tools.c reaching into gameplay itself.
typedef struct
{
    int open;
    int in_group;      // 0 = browsing the top-level group list, 1 = browsing items inside current_group.
    int current_group;  // Which group is highlighted (top level) or entered (item level).
    int selected_item;  // Cursor position within whichever list is currently showing.

    // Persistent gameplay-visible diagnostic overlays, toggled by the
    // DIAG group's actions (see DEV_ACTION_DIAG_TOGGLE_HITBOXES/
    // _STATS above). Unlike the fields above, these stay in effect
    // while the panel is closed and gameplay is running - main.c flips
    // them directly in its dispatch switch, and game_render.c reads
    // them unconditionally every frame (not just while open) to decide
    // whether to draw the hitbox outlines / stats overlay.
    int show_hitboxes;
    int show_stats;

} DevTools;

// Reset to closed, top-level group list, nothing entered.
void dev_tools_init(DevTools *dev);

// Decide what one grave-key press should do, given whether the game
// is currently in GAME_PLAYING (the only state the panel is allowed
// to open from - passed in rather than making this header depend on
// GameState/game_render.h). This function - not main.c - owns the
// open/closed decision: it opens (and returns DEV_TOGGLE_OPEN) only
// when the panel is closed and is_game_playing is true, closes (and
// returns DEV_TOGGLE_CLOSE) whenever it's already open regardless of
// state, and otherwise leaves dev untouched and returns
// DEV_TOGGLE_NONE. main.c calls runtime_begin_pause()/
// runtime_end_pause() only in response to what this returns - it
// never reads or writes dev->open directly to make that decision
// itself.
DevToggleResult dev_tools_handle_toggle(DevTools *dev, int is_game_playing);

// Draw the developer panel if open - a no-op otherwise, so it's safe
// to call unconditionally every frame from game_render_frame().
void dev_tools_render(SDL_Renderer *renderer, const DevTools *dev);

// Feed one discrete (non-repeat) key-down scancode to the panel while
// it's open - Up/Down move the cursor, Enter either descends into a
// group, returns to the group list from its synthetic "BACK" item, or
// activates a real leaf action. Returns DEV_ACTION_NONE for every key
// that isn't "activate a real action" (movement, entering/leaving a
// group, or any key the panel doesn't use) - main.c only needs to act
// when this returns something else. The caller (main.c) is
// responsible for not calling this for keys the panel itself
// shouldn't see (SPACE/firing, etc.) and for the grave key that
// opens/closes the panel in the first place - this function only ever
// runs while the panel is already open.
DevAction dev_tools_handle_key(DevTools *dev, SDL_Scancode scancode);

// Translate the legacy B (boss skip) and 5 (jump to Wave 5) debug
// hotkeys into the same DevAction values the panel's own BOSS_SKIP/
// WAVE_JUMP_5 items report (v0.9.0 Phase 8 migration) - so there is
// exactly one implementation of each effect, reached either way, not
// two. Checked against held-key state every frame, matching the
// "fires every frame the key is held" polling behavior these two
// shortcuts always had before this migration - not the discrete
// key-down handling dev_tools_handle_key() above uses. Takes
// boss_wave_active rather than a WaveState/Boss pointer so this
// function needs no gameplay-struct knowledge at all - the caller
// gets it with its own single wave_get_difficulty() call (v0.9.1
// Phase 3), the same way DEV_ACTION_BOSS_SKIP's handling does.
// Independent of dev->open - B/5 are standalone hotkeys, not panel
// navigation, exactly as they were before this migration.
DevAction dev_tools_handle_legacy_shortcuts(const Uint8 *keyboard, int boss_wave_active);

#else

// Toolkit-free release build: DevTools carries only "open", the one
// field main.c's own shared (unconditional) code reads directly, to
// guard SPACE/ENTER/P from firing while a panel that can't exist here
// is somehow still open (defensive - see process_input_events()) and
// to decide whether Up/Down/Enter should route to
// dev_tools_handle_key() below. Every other field the dev build's
// DevTools carries (menu position, diagnostic toggles) is release-only
// state with no reader left anywhere in this build (v0.9.0 Phase 8
// release compile-out audit): main.c's dev-action dispatch and
// game_render.c's diagnostic overlays - the only code that ever read
// them - are themselves excluded from this build (see their own
// #ifdef STARFALL_DEV_TOOLS regions in main.c/game_render.c), so
// keeping those fields here too would just be unused state with
// nothing left to set or read them. Every function below is a static
// inline no-op that leaves "open" alone (dev_tools_init() sets it
// once, to 0, and nothing else in a release build ever touches it
// again); none of dev_tools.c is compiled or linked into this build.
// In particular, dev_tools_handle_toggle() always returns
// DEV_TOGGLE_NONE - main.c's grave handling therefore can never call
// runtime_begin_pause()/runtime_end_pause() here, regardless of
// dev->open's value, because that decision no longer lives in main.c
// at all.
typedef struct
{
    int open;

} DevTools;

static inline void dev_tools_init(DevTools *dev)
{
    dev->open = 0;
}

static inline void dev_tools_render(SDL_Renderer *renderer, const DevTools *dev)
{
    (void)renderer;
    (void)dev;
}

static inline DevAction dev_tools_handle_key(DevTools *dev, SDL_Scancode scancode)
{
    (void)dev;
    (void)scancode;

    return DEV_ACTION_NONE;
}

static inline DevToggleResult dev_tools_handle_toggle(DevTools *dev, int is_game_playing)
{
    (void)dev;
    (void)is_game_playing;

    return DEV_TOGGLE_NONE;
}

// B and 5 have no developer effect in a toolkit-free release build -
// regardless of keyboard state or boss_wave_active, this always
// reports nothing to do, so main.c's dispatch (itself excluded from
// this build) could never have anything to act on even if it existed.
static inline DevAction dev_tools_handle_legacy_shortcuts(const Uint8 *keyboard, int boss_wave_active)
{
    (void)keyboard;
    (void)boss_wave_active;

    return DEV_ACTION_NONE;
}

#endif

#endif
