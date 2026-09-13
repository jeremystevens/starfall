#include "dev_tools.h"

#include <stdio.h>

#include "text.h"
#include "game_config.h"

// One selectable line within a group's item list. action is
// DEV_ACTION_NONE for the synthetic "BACK" entry every group ends
// with - dev_tools_handle_key() special-cases that action value to
// mean "return to the group list" rather than reporting it to main.c,
// so BACK never needs its own field/flag.
typedef struct
{
    const char *label;
    DevAction action;

} DevMenuItem;

typedef struct
{
    const char *label;
    const DevMenuItem *items;
    int item_count;

} DevMenuGroup;

// The v0.9.0 toolkit's first-version menu, exactly matching the
// spec's five groups. Every real leaf item's label stays short enough
// (with its "> " selection prefix) to fit the panel's fixed width at
// 1x text scale - see DEV_PANEL_WIDTH below.
static const DevMenuItem NAV_ITEMS[] =
{
    { "PREVIOUS WAVE", DEV_ACTION_WAVE_PREVIOUS },
    { "NEXT WAVE",     DEV_ACTION_WAVE_NEXT },
    { "JUMP WAVE 5",   DEV_ACTION_WAVE_JUMP_5 },
    { "NEXT BOSS",     DEV_ACTION_WAVE_NEXT_BOSS },
    { "BACK",          DEV_ACTION_NONE }
};

static const DevMenuItem PLAYER_ITEMS[] =
{
    { "ADD LIFE",      DEV_ACTION_PLAYER_ADD_LIFE },
    { "INVULNERABLE",  DEV_ACTION_PLAYER_TOGGLE_INVULNERABLE },
    { "BACK",          DEV_ACTION_NONE }
};

static const DevMenuItem POWERUP_ITEMS[] =
{
    { "SPAWN RAPID",   DEV_ACTION_POWERUP_SPAWN_RAPID },
    { "SPAWN SPREAD",  DEV_ACTION_POWERUP_SPAWN_SPREAD },
    { "SPAWN SHIELD",  DEV_ACTION_POWERUP_SPAWN_SHIELD },
    { "BACK",          DEV_ACTION_NONE }
};

static const DevMenuItem BOSS_ITEMS[] =
{
    { "SKIP BOSS",     DEV_ACTION_BOSS_SKIP },
    { "BACK",          DEV_ACTION_NONE }
};

static const DevMenuItem DIAG_ITEMS[] =
{
    { "HITBOXES",      DEV_ACTION_DIAG_TOGGLE_HITBOXES },
    { "STATS",         DEV_ACTION_DIAG_TOGGLE_STATS },
    { "BACK",          DEV_ACTION_NONE }
};

#define ITEM_COUNT(arr) (int)(sizeof(arr) / sizeof(arr[0]))

static const DevMenuGroup DEV_MENU_GROUPS[] =
{
    { "NAVIGATION",   NAV_ITEMS,     ITEM_COUNT(NAV_ITEMS) },
    { "PLAYER",       PLAYER_ITEMS,  ITEM_COUNT(PLAYER_ITEMS) },
    { "POWER-UPS",    POWERUP_ITEMS, ITEM_COUNT(POWERUP_ITEMS) },
    { "BOSS",         BOSS_ITEMS,    ITEM_COUNT(BOSS_ITEMS) },
    { "DIAGNOSTICS",  DIAG_ITEMS,    ITEM_COUNT(DIAG_ITEMS) }
};

#define DEV_MENU_GROUP_COUNT ITEM_COUNT(DEV_MENU_GROUPS)

// Panel layout - a fixed box comfortably inside the 160x120 logical
// screen, matching the retro scale (1x, same as the HUD text) rather
// than introducing a new font size. Sized for the longest of the two
// levels the panel ever shows at once (the group list, 5 lines, or
// NAVIGATION's item list, also 5 lines including BACK) - not for the
// full flat set of every action across every group.
#define DEV_PANEL_X 20
#define DEV_PANEL_Y 20
#define DEV_PANEL_WIDTH 120
#define DEV_PANEL_HEIGHT 82
#define DEV_PANEL_TEXT_SCALE 1

// Which real action (never BACK, never DEV_ACTION_NONE)
// dev_tools_handle_key() most recently reported, so dev_tools_render()
// can echo it back in the panel as permanent "did that actually
// register" feedback - originally added in Phase 3 as a testability
// aid before any action had a real gameplay effect to observe
// instead, kept afterward since confirming the panel registered a
// press is still useful once it does have one. File-local rather than
// a DevTools field: there is only ever one panel instance for the
// life of the program, and nothing outside this file has any reason
// to read this.
static const char *last_activated_label = NULL;

void dev_tools_init(DevTools *dev)
{
    dev->open = 0;
    dev->in_group = 0;
    dev->current_group = 0;
    dev->selected_item = 0;
    dev->show_hitboxes = 0;
    dev->show_stats = 0;
}

// The one place that decides whether a grave-key press opens or
// closes the panel - see this function's declaration in dev_tools.h
// for why main.c calls this instead of inspecting/mutating dev->open
// itself. Opening is gated on is_game_playing exactly the way P's own
// pause toggle is gated on GAME_PLAYING in main.c; closing has no such
// gate; a press that's neither (already open is handled by the close
// branch first, so this only remains true while closed and not in
// GAME_PLAYING) does nothing.
DevToggleResult dev_tools_handle_toggle(DevTools *dev, int is_game_playing)
{
    if (dev->open)
    {
        dev->open = 0;
        return DEV_TOGGLE_CLOSE;
    }

    if (is_game_playing)
    {
        dev->open = 1;
        return DEV_TOGGLE_OPEN;
    }

    return DEV_TOGGLE_NONE;
}

// How many lines the currently-displayed level has - the group list
// while dev->in_group is false, or the entered group's own item list
// (including its trailing BACK) otherwise.
static int dev_tools_current_item_count(const DevTools *dev)
{
    if (dev->in_group)
    {
        return DEV_MENU_GROUPS[dev->current_group].item_count;
    }

    return DEV_MENU_GROUP_COUNT;
}

DevAction dev_tools_handle_key(DevTools *dev, SDL_Scancode scancode)
{
    int item_count = dev_tools_current_item_count(dev);

    if (scancode == SDL_SCANCODE_UP)
    {
        dev->selected_item--;

        if (dev->selected_item < 0)
        {
            dev->selected_item = item_count - 1;
        }
    }
    else if (scancode == SDL_SCANCODE_DOWN)
    {
        dev->selected_item++;

        if (dev->selected_item >= item_count)
        {
            dev->selected_item = 0;
        }
    }
    else if (scancode == SDL_SCANCODE_RETURN)
    {
        if (!dev->in_group)
        {
            // Descend into the highlighted group's own item list,
            // landing on its first item.
            dev->current_group = dev->selected_item;
            dev->in_group = 1;
            dev->selected_item = 0;
        }
        else
        {
            const DevMenuItem *item =
                &DEV_MENU_GROUPS[dev->current_group].items[dev->selected_item];

            if (item->action == DEV_ACTION_NONE)
            {
                // BACK - return to the group list, re-highlighting the
                // group just left rather than resetting to the top.
                dev->in_group = 0;
                dev->selected_item = dev->current_group;
            }
            else
            {
                // A real action - report it to main.c. Deliberately
                // does not move the cursor or leave the group, so the
                // same action (e.g. Add Life) can be activated again
                // with another distinct Enter press without having to
                // re-enter the group each time.
                last_activated_label = item->label;

                return item->action;
            }
        }
    }

    return DEV_ACTION_NONE;
}

// v0.9.0 Phase 8 - the legacy B/5 hotkeys, reporting the exact same
// DevAction values the panel's own BOSS_SKIP/WAVE_JUMP_5 items report
// so main.c's single dispatch switch is the only place either effect
// is actually implemented. Mirrors the original inline checks exactly
// (same SDL_SCANCODE_B/SDL_SCANCODE_5 held-key polling, same
// boss_wave_active gate on B, no gate at all on 5) rather than
// switching to discrete key-down handling - this is a relocation, not
// a behavior change.
DevAction dev_tools_handle_legacy_shortcuts(const Uint8 *keyboard, int boss_wave_active)
{
    if (boss_wave_active && keyboard[SDL_SCANCODE_B])
    {
        return DEV_ACTION_BOSS_SKIP;
    }

    if (keyboard[SDL_SCANCODE_5])
    {
        return DEV_ACTION_WAVE_JUMP_5;
    }

    return DEV_ACTION_NONE;
}

// Draws one menu line. The current selection gets two independent,
// redundant treatments - neither one alone is "the" indicator:
//   1. A "> " cursor prefix (two plain spaces otherwise, so the label
//      itself never shifts horizontally depending on which line is
//      highlighted).
//   2. Bright green text, instead of every unselected line's white -
//      distinct from the panel's own yellow title/border/hint too.
// A color-only difference can be too subtle to notice reliably on a
// real display at 160x120 with a 5x7 font, and a glyph-only cursor
// (easy to skim past in a short list) is weaker alone than the two
// combined - see the fix report for why this line originally rendered
// with no visible indicator at all.
static void dev_tools_render_menu_line(
    SDL_Renderer *renderer,
    const char *label,
    int y,
    int is_selected
)
{
    char line[32];
    snprintf(line, sizeof(line), "%s%s", is_selected ? "> " : "  ", label);

    if (is_selected)
    {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    }

    text_draw(renderer, line, DEV_PANEL_X + 6, y, DEV_PANEL_TEXT_SCALE);
}

void dev_tools_render(SDL_Renderer *renderer, const DevTools *dev)
{
    if (!dev->open)
    {
        return;
    }

    // Solid panel background so the frozen battlefield behind it
    // doesn't make the menu text hard to read.
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    SDL_Rect panel_bg =
    {
        DEV_PANEL_X,
        DEV_PANEL_Y,
        DEV_PANEL_WIDTH,
        DEV_PANEL_HEIGHT
    };
    SDL_RenderFillRect(renderer, &panel_bg);

    // Border, so the panel clearly reads as an overlay rather than
    // part of the playfield underneath it.
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    SDL_RenderDrawRect(renderer, &panel_bg);

    // Title - "DEV TOOLS" at the group list, or the entered group's
    // own name once inside it, so the panel always shows where the
    // player currently is. Yellow, distinct from every other
    // white/colored text elsewhere in the game, in the same "identify
    // itself clearly as a development tool" spirit the spec asks for.
    text_draw(
        renderer,
        dev->in_group ? DEV_MENU_GROUPS[dev->current_group].label : "DEV TOOLS",
        DEV_PANEL_X + 6,
        DEV_PANEL_Y + 4,
        DEV_PANEL_TEXT_SCALE
    );

    // Draw color from here is set per-line inside
    // dev_tools_render_menu_line() (white or the selected line's
    // green) - not fixed once up front the way it is everywhere else
    // in this function, since which line is selected changes every
    // frame.
    int line_y = DEV_PANEL_Y + 16;

    if (!dev->in_group)
    {
        for (int i = 0; i < DEV_MENU_GROUP_COUNT; i++)
        {
            dev_tools_render_menu_line(
                renderer,
                DEV_MENU_GROUPS[i].label,
                line_y,
                i == dev->selected_item
            );

            line_y += 8;
        }
    }
    else
    {
        const DevMenuGroup *group = &DEV_MENU_GROUPS[dev->current_group];

        for (int i = 0; i < group->item_count; i++)
        {
            dev_tools_render_menu_line(
                renderer,
                group->items[i].label,
                line_y,
                i == dev->selected_item
            );

            line_y += 8;
        }
    }

    // Explicitly back to white here - the loop above leaves the draw
    // color at whichever line was drawn last, green if that happened
    // to be the selected one, and both lines below must always be
    // plain white regardless.
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // Permanent "last action" feedback line (see last_activated_label's
    // own comment) - shown only once something has actually been
    // activated, so the group/item lists above don't shift to make
    // room for it before there's anything to report.
    if (last_activated_label != NULL)
    {
        char activated_line[32];
        snprintf(activated_line, sizeof(activated_line), "LAST %s", last_activated_label);
        text_draw(
            renderer,
            activated_line,
            DEV_PANEL_X + 6,
            DEV_PANEL_Y + DEV_PANEL_HEIGHT - 20,
            DEV_PANEL_TEXT_SCALE
        );
    }

    // "GRAVE CLOSE", not a literal backtick character - the 5x7 font
    // only covers A-Z and 0-9 (see text.c), no punctuation glyphs.
    text_draw(
        renderer,
        "GRAVE CLOSE",
        DEV_PANEL_X + 6,
        DEV_PANEL_Y + DEV_PANEL_HEIGHT - 12,
        DEV_PANEL_TEXT_SCALE
    );
}
