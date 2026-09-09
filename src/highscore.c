#include "highscore.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

// A small human-readable save file - just the high score as plain
// text (e.g. "12450"), nothing more. Stored relative to the working
// directory the game is launched from; that's fine for a first
// implementation, but means the file lives wherever the executable
// happens to be run from rather than a proper per-user save location
// - worth revisiting if this game is ever packaged for distribution.
#define HIGHSCORE_FILE "highscore.dat"

// Load the persisted high score, tolerating every way the file could
// be missing or wrong without treating any of them as a real error.
int highscore_load(void)
{
    FILE *file = fopen(HIGHSCORE_FILE, "r");

    if (file == NULL)
    {
        // Most likely the first time the game has ever been run here
        // - not an error, just nothing saved yet.
        return 0;
    }

    // A high score easily fits in a handful of digits; this is
    // generous enough to still safely read (and reject) a much longer
    // line of garbage without overflowing the buffer.
    char line[64];

    if (fgets(line, sizeof(line), file) == NULL)
    {
        // Empty file.
        fclose(file);
        return 0;
    }

    fclose(file);

    // strtol() writes back how far it actually got into *endptr. If
    // that's still pointing at the very start of the buffer, it found
    // no digits at all (e.g. the file contained
    // "grandpa_gg_was_here") - anything parsed after that point, like
    // a trailing newline, doesn't need to be checked separately.
    char *endptr;
    long value = strtol(line, &endptr, 10);

    if (endptr == line)
    {
        return 0;
    }

    // Reject a negative score outright, and clamp an absurdly large
    // one (a hand-edited or corrupted file) down to something score,
    // an int everywhere else in the game, can hold safely.
    if (value < 0)
    {
        return 0;
    }

    if (value > INT_MAX)
    {
        value = INT_MAX;
    }

    return (int)value;
}

// Save a new high score, overwriting the file entirely - this only
// ever gets called with the new full value, never an increment.
int highscore_save(int score)
{
    FILE *file = fopen(HIGHSCORE_FILE, "w");

    if (file == NULL)
    {
        fprintf(stderr, "Warning: could not save high score.\n");
        return 0;
    }

    fprintf(file, "%d\n", score);

    fclose(file);

    return 1;
}
