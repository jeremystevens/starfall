#include "text.h"

#include <ctype.h>
#include <string.h>

#define FONT_WIDTH 5
#define FONT_HEIGHT 7

// One character's glyph, stored as FONT_HEIGHT strings of '#'/'.'.
typedef struct
{
    char character;
    const char *rows[FONT_HEIGHT];

} BitmapCharacter;

// Hand-drawn 5x7 bitmap glyphs for A-Z and 0-9.
static const BitmapCharacter font[] =
{
    // =========================
    // A - Z
    // =========================

    {
        'A',
        {
            ".###.",
            "#...#",
            "#...#",
            "#####",
            "#...#",
            "#...#",
            "#...#"
        }
    },

    {
        'B',
        {
            "####.",
            "#...#",
            "#...#",
            "####.",
            "#...#",
            "#...#",
            "####."
        }
    },

    {
        'C',
        {
            ".####",
            "#....",
            "#....",
            "#....",
            "#....",
            "#....",
            ".####"
        }
    },

    {
        'D',
        {
            "####.",
            "#...#",
            "#...#",
            "#...#",
            "#...#",
            "#...#",
            "####."
        }
    },

    {
        'E',
        {
            "#####",
            "#....",
            "#....",
            "####.",
            "#....",
            "#....",
            "#####"
        }
    },

    {
        'F',
        {
            "#####",
            "#....",
            "#....",
            "####.",
            "#....",
            "#....",
            "#...."
        }
    },

    {
        'G',
        {
            ".###.",
            "#....",
            "#....",
            "#.###",
            "#...#",
            "#...#",
            ".###."
        }
    },

    {
        'H',
        {
            "#...#",
            "#...#",
            "#...#",
            "#####",
            "#...#",
            "#...#",
            "#...#"
        }
    },

    {
        'I',
        {
            "#####",
            "..#..",
            "..#..",
            "..#..",
            "..#..",
            "..#..",
            "#####"
        }
    },

    {
        'J',
        {
            "..###",
            "...#.",
            "...#.",
            "...#.",
            "...#.",
            "#..#.",
            ".##.."
        }
    },

    {
        'K',
        {
            "#...#",
            "#..#.",
            "#.#..",
            "##...",
            "#.#..",
            "#..#.",
            "#...#"
        }
    },

    {
        'L',
        {
            "#....",
            "#....",
            "#....",
            "#....",
            "#....",
            "#....",
            "#####"
        }
    },

    {
        'M',
        {
            "#...#",
            "##.##",
            "#.#.#",
            "#.#.#",
            "#...#",
            "#...#",
            "#...#"
        }
    },

    {
        'N',
        {
            "#...#",
            "##..#",
            "##..#",
            "#.#.#",
            "#..##",
            "#..##",
            "#...#"
        }
    },

    {
        'O',
        {
            ".###.",
            "#...#",
            "#...#",
            "#...#",
            "#...#",
            "#...#",
            ".###."
        }
    },

    {
        'P',
        {
            "####.",
            "#...#",
            "#...#",
            "####.",
            "#....",
            "#....",
            "#...."
        }
    },

    {
        'Q',
        {
            ".###.",
            "#...#",
            "#...#",
            "#...#",
            "#.#.#",
            "#..#.",
            ".##.#"
        }
    },

    {
        'R',
        {
            "####.",
            "#...#",
            "#...#",
            "####.",
            "#.#..",
            "#..#.",
            "#...#"
        }
    },

    {
        'S',
        {
            ".####",
            "#....",
            "#....",
            ".###.",
            "....#",
            "....#",
            "####."
        }
    },

    {
        'T',
        {
            "#####",
            "..#..",
            "..#..",
            "..#..",
            "..#..",
            "..#..",
            "..#.."
        }
    },

    {
        'U',
        {
            "#...#",
            "#...#",
            "#...#",
            "#...#",
            "#...#",
            "#...#",
            ".###."
        }
    },

    {
        'V',
        {
            "#...#",
            "#...#",
            "#...#",
            "#...#",
            "#...#",
            ".#.#.",
            "..#.."
        }
    },

    {
        'W',
        {
            "#...#",
            "#...#",
            "#...#",
            "#...#",
            "#.#.#",
            "##.##",
            "#...#"
        }
    },

    {
        'X',
        {
            "#...#",
            "#...#",
            ".#.#.",
            "..#..",
            ".#.#.",
            "#...#",
            "#...#"
        }
    },

    {
        'Y',
        {
            "#...#",
            "#...#",
            ".#.#.",
            "..#..",
            "..#..",
            "..#..",
            "..#.."
        }
    },

    {
        'Z',
        {
            "#####",
            "....#",
            "...#.",
            "..#..",
            ".#...",
            "#....",
            "#####"
        }
    },


    // =========================
    // 0 - 9
    // =========================

    {
        '0',
        {
            ".###.",
            "#...#",
            "#..##",
            "#.#.#",
            "##..#",
            "#...#",
            ".###."
        }
    },

    {
        '1',
        {
            "..#..",
            ".##..",
            "..#..",
            "..#..",
            "..#..",
            "..#..",
            ".###."
        }
    },

    {
        '2',
        {
            ".###.",
            "#...#",
            "....#",
            "...#.",
            "..#..",
            ".#...",
            "#####"
        }
    },

    {
        '3',
        {
            "####.",
            "....#",
            "....#",
            ".###.",
            "....#",
            "....#",
            "####."
        }
    },

    {
        '4',
        {
            "...#.",
            "..##.",
            ".#.#.",
            "#..#.",
            "#####",
            "...#.",
            "...#."
        }
    },

    {
        '5',
        {
            "#####",
            "#....",
            "#....",
            "####.",
            "....#",
            "....#",
            "####."
        }
    },

    {
        '6',
        {
            ".###.",
            "#....",
            "#....",
            "####.",
            "#...#",
            "#...#",
            ".###."
        }
    },

    {
        '7',
        {
            "#####",
            "....#",
            "...#.",
            "..#..",
            ".#...",
            ".#...",
            ".#..."
        }
    },

    {
        '8',
        {
            ".###.",
            "#...#",
            "#...#",
            ".###.",
            "#...#",
            "#...#",
            ".###."
        }
    },

    {
        '9',
        {
            ".###.",
            "#...#",
            "#...#",
            ".####",
            "....#",
            "....#",
            ".###."
        }
    }
};

// Look up one glyph and draw it pixel-by-pixel, scaled up by "scale".
void text_draw_char(
    SDL_Renderer *renderer,
    char character,
    int x,
    int y,
    int scale
)
{
    // Convert lowercase letters to uppercase.
    character = (char)toupper((unsigned char)character);

    int font_count = sizeof(font) / sizeof(font[0]);

    // Search the font table for the requested character.
    for (int i = 0; i < font_count; i++)
    {
        if (font[i].character == character)
        {
            // Go through all 7 rows of the character.
            for (int row = 0; row < FONT_HEIGHT; row++)
            {
                // Go through all 5 columns.
                for (int col = 0; col < FONT_WIDTH; col++)
                {
                    if (font[i].rows[row][col] == '#')
                    {
                        SDL_Rect pixel =
                        {
                            x + (col * scale),
                            y + (row * scale),
                            scale,
                            scale
                        };

                        SDL_RenderFillRect(renderer, &pixel);
                    }
                }
            }

            return;
        }
    }
}

// Draw a full string, character by character, left to right.
void text_draw(
    SDL_Renderer *renderer,
    const char *text,
    int x,
    int y,
    int scale
)
{
    int cursor_x = x;

    // Draw each character in the string.
    for (int i = 0; text[i] != '\0'; i++)
    {
        // Spaces don't need to be rendered.
        // Just move the cursor forward.
        if (text[i] == ' ')
        {
            cursor_x += (FONT_WIDTH + 1) * scale;
            continue;
        }

        text_draw_char(
            renderer,
            text[i],
            cursor_x,
            y,
            scale
        );

        // Move to the position of the next character.
        cursor_x += (FONT_WIDTH + 1) * scale;
    }
}

// Pixel width a string would occupy if drawn with text_draw().
int text_width(const char *text, int scale)
{
    int length = (int)strlen(text);

    if (length == 0)
    {
        return 0;
    }

    // Every character (including spaces) advances the cursor by
    // FONT_WIDTH+1 columns, except the last one, which doesn't need
    // trailing space after it.
    return (length - 1) * (FONT_WIDTH + 1) * scale + FONT_WIDTH * scale;
}


