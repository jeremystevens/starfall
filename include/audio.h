
#ifndef AUDIO_H
#define AUDIO_H

#include <SDL.h>

// Holds the live state of every procedurally generated sound effect.
// SDL's audio callback reads and advances these fields each time it
// needs more samples, so there's no audio file loading anywhere here.
typedef struct
{
    // Player laser.
    double phase;
    double frequency;
    int samples_remaining;

    // Enemy laser.
    double enemy_phase;
    double enemy_frequency;
    int enemy_samples_remaining;

    // Bomber's bomb release - a short, low thunk instead of a laser.
    double bomb_phase;
    double bomb_frequency;
    int bomb_samples_remaining;

    // Player ship destruction.
    double explosion_rumble_phase;
    int explosion_samples_remaining;
    int explosion_samples_total;

    // Scout destruction.
    double enemy_explosion_rumble_phase;
    int enemy_explosion_samples_remaining;
    int enemy_explosion_samples_total;

    // Power-up pickup.
    double pickup_phase;
    double pickup_frequency;
    int pickup_samples_remaining;

    // Extra life fanfare - a short ascending arpeggio rather than a
    // single tone, so note_index tracks which note is currently
    // playing (== EXTRA_LIFE_NOTE_COUNT means idle) alongside how
    // many samples are left in that note.
    double extra_life_phase;
    int extra_life_note_index;
    int extra_life_samples_remaining;

    // Dreadnought warning klaxon - unlike every other sound in this
    // struct, this has no fixed duration of its own. It plays for as
    // long as boss_warning_active stays set (see
    // audio_set_boss_warning()), sweeping LOW -> HIGH -> LOW on a
    // repeating cycle rather than counting down samples to silence.
    int boss_warning_active;
    double boss_warning_phase;
    int boss_warning_pulse_sample;

    // New high score fanfare - same "step through a note table"
    // approach as the extra life fanfare above, just a longer,
    // different melody (see NEW_HIGH_SCORE_NOTES in audio.c) so the
    // two are never confused by ear.
    double new_high_score_phase;
    int new_high_score_note_index;
    int new_high_score_samples_remaining;

} LaserSound;

// Open the audio device and set up the sound state.
// Returns 1 on success and 0 on failure.
int audio_init(LaserSound *laser);


// Start/restart the player's laser sound.
void audio_play_laser(LaserSound *laser);

// Start/restart the Scout's laser sound.
void audio_play_enemy_laser(LaserSound *laser);

// Start/restart the Bomber's bomb-drop sound.
void audio_play_bomb_drop(LaserSound *laser);

// Start/restart the player ship's destruction sound.
void audio_play_explosion(LaserSound *laser);

// Start/restart the Scout's destruction sound.
void audio_play_enemy_explosion(LaserSound *laser);

// Start/restart the power-up pickup sound.
void audio_play_pickup(LaserSound *laser);

// Start/restart the extra life fanfare.
void audio_play_extra_life(LaserSound *laser);

// Start/restart the new high score fanfare.
void audio_play_new_high_score(LaserSound *laser);

// Turn the Dreadnought warning klaxon on or off. Unlike every other
// audio_play_*() function above, this doesn't trigger a fixed-length
// sound - it's a switch. Pass 1 when BOSS_STATE_WARNING begins and 0
// the moment it ends (however it ends - the warning completing
// normally, a debug skip, or a mid-warning restart all need to call
// this with 0, or the alarm would otherwise loop forever).
void audio_set_boss_warning(LaserSound *laser, int active);


// Close the audio device.
void audio_shutdown(void);


#endif
