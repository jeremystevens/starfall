#ifndef HIGHSCORE_H
#define HIGHSCORE_H

// Persistent high-score storage. Just one value for now - the best
// score ever achieved - stored in a small human-readable file next to
// the executable (see HIGHSCORE_FILE in highscore.c). No player
// names, no top-ten list, no binary format; a single integer doesn't
// need any of that, and main.c never needs to know the file even
// exists.

// Load the persisted high score from disk. A missing file (first
// run), an empty file, text that doesn't parse as a number, or a
// negative value are all treated the same way: not an error, just "no
// valid high score on record yet" - returns 0 in every one of those
// cases rather than failing or crashing.
int highscore_load(void);

// Persist a new high score to disk, overwriting whatever was there.
// Returns 1 on success, 0 if the file couldn't be written (e.g. a
// read-only working directory) - the caller should keep using the
// in-memory value for the rest of the session either way, since a
// failed save isn't a reason to stop the game.
int highscore_save(int score);

#endif
