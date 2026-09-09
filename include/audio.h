
#ifndef AUDIO_H
#define AUDIO_H

#include <SDL.h>

#include "music.h"

// Sample rate every procedural sound effect AND the music sequencer
// (music.h/music.c) are generated at - defined here, once, so
// music.c's note-duration timing and every waveform in this file stay
// in sync by construction rather than by convention.
#define SAMPLE_RATE 44100

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

    // The music sequencer (see music.h). Embedded directly, not a
    // pointer, so the single LaserSound instance/userdata pointer
    // main.c already threads through every audio_play_*() call keeps
    // working completely unchanged - the SDL callback just also mixes
    // this in every sample.
    MusicState music;

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

// True once the new high score fanfare has played through its last
// note (or was never started/was already stopped) - reads the same
// note_index the audio callback itself advances, rather than a
// separately tracked duration/timer, so this can never drift out of
// sync with what's actually still audible. Lets a caller (main.c)
// sequence something to start exactly when the fanfare actually
// finishes, instead of guessing at its total duration.
int audio_new_high_score_finished(const LaserSound *laser);

// Stop the new high score fanfare immediately, before it finishes on
// its own - e.g. the player pressing ENTER partway through it. Forces
// the same idle state audio_init() and a normal completion both leave
// it in, so audio_new_high_score_finished() immediately reports true
// afterward.
void audio_stop_new_high_score(LaserSound *laser);

// Turn the Dreadnought warning klaxon on or off. Unlike every other
// audio_play_*() function above, this doesn't trigger a fixed-length
// sound - it's a switch. Pass 1 when BOSS_STATE_WARNING begins and 0
// the moment it ends (however it ends - the warning completing
// normally, a debug skip, or a mid-warning restart all need to call
// this with 0, or the alarm would otherwise loop forever).
void audio_set_boss_warning(LaserSound *laser, int active);


// Start playing a music track (see music.h's MusicNote) on the melody
// voice, from the beginning, looping if requested. Generic - this has
// no idea whether the caller means the title theme, the boss theme,
// or a test melody; that decision belongs entirely to main.c.
//
// Also resets playback_rate to 1.0 every time it's called. This is
// deliberate: it's the one place every new composition is expected to
// begin, so resetting here - rather than requiring every future call
// site to remember a separate "and also reset the rate" step -
// structurally prevents a boss fight's tempo increase from ever
// surviving into whatever plays next (e.g. Mountain King ending at
// 1.3x, then the Blue Danube starting sped up by mistake).
void audio_music_start(
    LaserSound *laser,
    const MusicNote *notes,
    int note_count,
    int loop
);

// Same as audio_music_start(), but for an arbitrary voice rather than
// always voice 0 - needed for a second simultaneous part (e.g. a
// future accompaniment voice) that should join a composition already
// in progress. Deliberately does NOT reset playback_rate the way
// audio_music_start() does: starting a second voice mid-composition
// must not discard whatever rate the first voice already established.
void audio_music_start_voice(
    LaserSound *laser,
    int voice_index,
    const MusicNote *notes,
    int note_count,
    int loop
);

// Start two voices in a single locked operation, both from event 0,
// with playback_rate reset to 1.0 first - guarantees the two voices
// are sample-aligned from the very first callback that reads them,
// unlike calling audio_music_start() followed by
// audio_music_start_voice() (two separate lock/unlock cycles, with a
// window between them where the callback could run and leave one
// voice's start offset from the other's by however many samples that
// callback produced). Intended for any two-voice composition that
// must start in lockstep - has no idea one of the two tracks is a
// title theme; that decision belongs to main.c, same as every other
// music_*() call here.
void audio_music_start_dual(
    LaserSound *laser,
    const MusicNote *notes0,
    int note_count0,
    int loop0,
    const MusicNote *notes1,
    int note_count1,
    int loop1
);

// Stop all music voices immediately.
void audio_music_stop(LaserSound *laser);

// Set the overall music volume (0.0-1.0).
void audio_music_set_volume(LaserSound *laser, float volume);

// Set how fast musical time advances for every voice - see
// music_set_playback_rate() in music.h for exact semantics and
// validation. Does not restart, reset position, or otherwise disturb
// whatever is currently playing.
void audio_music_set_playback_rate(LaserSound *laser, double playback_rate);


// Close the audio device.
void audio_shutdown(void);


#endif
