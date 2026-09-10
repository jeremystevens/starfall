#ifndef SOUNDTRACK_H
#define SOUNDTRACK_H

#include "audio.h"

// STARFALL's own procedural soundtrack - the three compositions
// (title, boss, and Game Over themes) and the intent-level API main.c
// uses to trigger them. main.c decides WHEN a soundtrack event should
// happen (e.g. "the boss just appeared"); this module owns WHAT plays
// and HOW it's started, using the existing audio_music_*() API in
// audio.h to actually drive the shared sequencer in music.h/music.c.
// No composition data (note arrays, counts) is exposed here - callers
// never need to know a track's name, note count, or how many voices
// it uses.

// Start the title theme (Blue Danube Waltz) from the beginning - both
// the melody and its waltz accompaniment, looping, sample-aligned,
// and at normal (1.0) playback rate.
void soundtrack_play_title(LaserSound *audio);

// Start the boss theme (In the Hall of the Mountain King) from the
// beginning - both the melody and its bass drone accompaniment,
// looping, sample-aligned, and at normal (1.0) playback rate.
void soundtrack_play_boss(LaserSound *audio);

// Adjust the boss theme's tempo to match the Dreadnought's current
// intensity phase (1/2/3, mapping to 1.00x/1.15x/1.30x) without
// restarting it - the melody and accompaniment continue from exactly
// where they already were, just faster or slower from that point on.
void soundtrack_set_boss_intensity(LaserSound *audio, int phase);

// Start the Game Over theme (Chopin's Funeral March) from the
// beginning - both voices, not looping. Plays through once and stops.
void soundtrack_play_game_over(LaserSound *audio);

// Stop whichever soundtrack composition is currently playing.
void soundtrack_stop(LaserSound *audio);

#endif
