#include <stdio.h>

#include "player.h"
#include "text.h"
#include "game_config.h"

// How long the player stays invulnerable after taking a hit.
#define INVULNERABILITY_TIME 1500

// The first extra life comes quickly - roughly partway through Wave 2
// for an average run - since a Scout is only worth 10 points and even
// the Dreadnought boss is 500. Every extra life after that costs more
// (a flat interval rather than a growing one, for simplicity), so
// lives taper off relative to the score rather than piling up as
// Wave 6+'s difficulty scaling makes points easier to earn.
#define EXTRA_LIFE_FIRST_THRESHOLD 1000
#define EXTRA_LIFE_INTERVAL 2000

// How long the "EXTRA LIFE" notification stays on screen once awarded -
// brief, since it's a quick "nice, another chance" beat rather than a
// major announcement like a wave transition or the boss warning.
#define EXTRA_LIFE_NOTIFICATION_DURATION_MS 1200

// Reset the player to its starting position, lives, and state.
void player_init(Player *player)
{
    player->x = 20.0f;
    player->y = 60.0f;
    player->width = 16;
    player->height = 10;
    player->speed = 2.0f;
    player->lives = 3;
    player->invulnerable = 0;
    player->invulnerable_until = 0;
    player->next_extra_life_score = EXTRA_LIFE_FIRST_THRESHOLD;
    player->extra_life_notification_until = 0;
    player->extra_life_notification_count = 0;
}
// Move the player based on held keys and clamp it inside the play area.
void player_update(
    Player *player,
    const Uint8 *keyboard,
    Uint32 current_time,
    int min_y
)
{

    // Move the player while the arrow keys are held down.
    if (keyboard[SDL_SCANCODE_UP])
    {
        player->y -= player->speed;
    }

    if (keyboard[SDL_SCANCODE_DOWN])
    {
        player->y += player->speed;
    }

    if (keyboard[SDL_SCANCODE_LEFT])
    {
        player->x -= player->speed;
    }

    if (keyboard[SDL_SCANCODE_RIGHT])
    {
        player->x += player->speed;
    }

    // Keep the player inside the 160x120 game area.
    if (player->x < 0)
    {
        player->x = 0;
    }

    // Keep the player below min_y - normally the HUD strip, but the
    // caller can raise this further during a boss fight.
    if (player->y < min_y)
    {
        player->y = (float)min_y;
    }

    if (player->x + player->width > 160)
    {
        player->x = 160 - player->width;
    }

    if (player->y + player->height > 120)
    {
        player->y = 120 - player->height;
    }

    // End invulnerability once the timer expires.
    if (player->invulnerable &&
            current_time >= player->invulnerable_until)
    {
        player->invulnerable = 0;
    }
}

// Apply a hit to the player and start the invulnerability window.
int player_take_damage(
    Player *player,
    PowerUpState *powerup_state,
    Uint32 current_time
)
{
    // Ignore damage while the player is invulnerable.
    if (player->invulnerable || player->lives <= 0)
    {
        return 0;
    }

    // An active Shield absorbs exactly one hit instead of costing a
    // life. It doesn't start invulnerability - the protection was the
    // shield itself, not a grace period.
    if (powerup_state->shield_active)
    {
        powerup_state->shield_active = 0;

        printf("Shield absorbed a hit!\n");

        return 0;
    }

    // Remove one life.
    player->lives--;

    if (player->lives < 0)
    {
        player->lives = 0;
    }

    printf("Player hit! Lives remaining: %d\n", player->lives);

    // Temporarily protect the player from additional damage.
    player->invulnerable = 1;
    player->invulnerable_until =
        current_time + INVULNERABILITY_TIME;

    return 1;
}

// Award an extra life for every score threshold crossed since the
// last call.
int player_check_extra_life(Player *player, int score, Uint32 current_time)
{
    int awarded = 0;

    while (score >= player->next_extra_life_score)
    {
        player->lives++;
        player->next_extra_life_score += EXTRA_LIFE_INTERVAL;
        awarded++;

        printf(
            "Extra life! Lives: %d (next at %d points)\n",
            player->lives,
            player->next_extra_life_score
        );
    }

    // Arms the on-screen notification - a side effect of awarding at
    // least one life, not a separate way of awarding one. If this
    // frame's score jump crossed more than one threshold at once (the
    // while loop above), count reflects that so the notification can
    // read "EXTRA LIFE X2" instead of showing two overlapping messages.
    if (awarded > 0)
    {
        player->extra_life_notification_until =
            current_time + EXTRA_LIFE_NOTIFICATION_DURATION_MS;
        player->extra_life_notification_count = awarded;
    }

    return awarded;
}

// Draw the player's ship.
void player_render(SDL_Renderer *renderer, const Player *player)
{
    // Blink the ship while temporarily invulnerable.
    if (player->invulnerable)
    {
        Uint32 current_time = SDL_GetTicks();

        // Alternate visibility every 100 milliseconds.
        if ((current_time / 100) % 2 == 0)
        {
            return;
        }
    }

    int x = (int)player->x;
    int y = (int)player->y;

    // Draw the gray body of the player's ship.
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);

    SDL_RenderDrawLine(renderer, x + 5, y,     x + 8,  y);
    SDL_RenderDrawLine(renderer, x + 4, y + 1, x + 10, y + 1);
    SDL_RenderDrawLine(renderer, x + 2, y + 2, x + 12, y + 2);
    SDL_RenderDrawLine(renderer, x + 1, y + 3, x + 14, y + 3);

    SDL_RenderDrawLine(renderer, x,     y + 4, x + 15, y + 4);
    SDL_RenderDrawLine(renderer, x,     y + 5, x + 15, y + 5);

    SDL_RenderDrawLine(renderer, x + 1, y + 6, x + 14, y + 6);
    SDL_RenderDrawLine(renderer, x + 2, y + 7, x + 12, y + 7);
    SDL_RenderDrawLine(renderer, x + 4, y + 8, x + 10, y + 8);
    SDL_RenderDrawLine(renderer, x + 5, y + 9, x + 8,  y + 9);

    // Draw the orange engine flame.
    SDL_SetRenderDrawColor(renderer, 255, 100, 0, 255);

    SDL_RenderDrawLine(renderer, x - 3, y + 4, x - 1, y + 4);
    SDL_RenderDrawLine(renderer, x - 3, y + 5, x - 1, y + 5);

    // Draw the light-blue cockpit.
    SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);

    SDL_Rect cockpit =
    {
        x + 9,
        y + 3,
        3,
        4
    };

    SDL_RenderFillRect(renderer, &cockpit);
}

// Y position for the "EXTRA LIFE" notification - low enough to clear
// the boss health bar overlay (which ends around y=25 when a boss
// fight is in progress), high enough to stay clear of the wave
// announcement/boss warning text starting at y=40, so it never
// overlaps either even if an extra life happens to land at the same
// moment as one of them.
#define EXTRA_LIFE_NOTIFICATION_Y 30

// Draws the brief "EXTRA LIFE" readout while one is active.
void player_render_extra_life_notification(
    SDL_Renderer *renderer,
    const Player *player
)
{
    if (SDL_GetTicks() >= player->extra_life_notification_until)
    {
        return;
    }

    char text[24];

    if (player->extra_life_notification_count > 1)
    {
        snprintf(
            text,
            sizeof(text),
            "EXTRA LIFE X%d",
            player->extra_life_notification_count
        );
    }
    else
    {
        snprintf(text, sizeof(text), "EXTRA LIFE");
    }

    // Warm gold, matching the celebratory feel of the extra-life
    // fanfare - distinct from the plain white used by the HUD and
    // every other announcement overlay.
    SDL_SetRenderDrawColor(renderer, 255, 215, 80, 255);

    int scale = 1;
    int x = (SCREEN_WIDTH - text_width(text, scale)) / 2;
    text_draw(renderer, text, x, EXTRA_LIFE_NOTIFICATION_Y, scale);
}
