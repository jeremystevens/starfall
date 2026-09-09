#ifndef MUSIC_H
#define MUSIC_H

// A generic, reusable multi-voice music sequencer used by audio.c.
// This module has no idea what a "title screen" or "boss fight" is -
// it only knows how to step through whatever note data it's handed,
// one audio sample at a time, entirely independent of the game's
// frame rate. main.c never calls into this module directly; audio.c
// wraps every function below with SDL's audio-device lock and exposes
// the locked versions as audio_music_*() in audio.h - the same
// pattern every existing audio_play_*() function already uses.

// One note in a track: a frequency in Hz (0.0 means a rest - silence
// for the full duration) and how long it lasts, in samples rather
// than milliseconds, so playback speed depends only on the actual
// sample rate, never on anything else.
typedef struct
{
    double frequency;
    int duration_samples;

} MusicNote;

// Two voices for now - enough for a melody plus one accompaniment
// part (see the Blue Danube's planned waltz accompaniment), which is
// as far as any currently planned track goes. Raising this later, if
// a track ever needs a third voice, requires no other code changes -
// every function below already loops over every voice generically.
#define MUSIC_MAX_VOICES 2

// One voice's playback position through its own note array. Voices
// are completely independent of each other - different tracks, or
// different parts of the same track, never share state.
typedef struct
{
    const MusicNote *notes;
    int note_count;
    int note_index;

    // How far into the current note playback has progressed, in
    // samples - a double, not an int, so it can accumulate a
    // fractional playback_rate (see MusicState) every call without
    // truncating away the fractional part. At the default 1.0 rate
    // this behaves exactly like the plain integer counter it replaced.
    double samples_into_note;

    double phase;
    int loop;
    int active;

} MusicVoice;

// All currently-playing music. Owned by audio.c as a field on its
// existing LaserSound struct, so the SDL audio callback can mix it in
// alongside every procedural sound effect without a separate audio
// device or callback of its own.
// Valid range for playback_rate below - generous enough for any
// foreseeable use (the Dreadnought's three intensity tiers need
// nothing close to either extreme) while still rejecting anything
// that would make the sequencer behave nonsensically.
#define MUSIC_PLAYBACK_RATE_MIN 0.25
#define MUSIC_PLAYBACK_RATE_MAX 4.0

typedef struct
{
    MusicVoice voices[MUSIC_MAX_VOICES];

    // Independent of every individual SFX amplitude, so music can be
    // tuned to sit below sound effects as a whole without retuning any
    // of them.
    float volume;

    // How fast musical time advances relative to the notes' own
    // encoded timing: 1.0 = as written, >1.0 = faster, <1.0 = slower.
    // Shared by every voice (not per-voice) since all voices already
    // advance together from the same music_next_sample() call, and
    // melody/accompaniment must never drift out of sync with each
    // other when this changes.
    double playback_rate;

} MusicState;

// Reset to silent - no voices playing.
void music_init(MusicState *music);

// Start playing notes/note_count on the given voice, from the
// beginning. Not locked against the audio callback itself - see
// audio_music_start() in audio.h for the version main.c should
// actually call. The engine has no idea what track this is; it just
// plays whatever array it's handed, looping back to the first note
// if loop is nonzero once the last note finishes.
void music_start_voice(
    MusicState *music,
    int voice_index,
    const MusicNote *notes,
    int note_count,
    int loop
);

// Stop a single voice immediately.
void music_stop_voice(MusicState *music, int voice_index);

// Stop every voice immediately.
void music_stop_all(MusicState *music);

// Set the overall music volume (0.0-1.0), applied to every voice.
void music_set_volume(MusicState *music, float volume);

// Set how fast musical time advances (see MusicState.playback_rate).
// Does NOT reset note_index, samples_into_note, or phase on any voice
// - changing rate while a composition is playing continues it from
// exactly where it already was, just faster or slower from that point
// on. Out-of-range, non-positive, NaN, or infinite values fall back to
// 1.0 rather than being allowed to corrupt playback - see music.c for
// the exact validation.
void music_set_playback_rate(MusicState *music, double playback_rate);

// Advance every active voice by exactly one sample (scaled by the
// current playback_rate) and return their summed, volume-scaled
// contribution. Called once per output sample from audio.c's existing
// SDL audio callback - the only function on the audio thread's hot
// path, and it never allocates, blocks, or logs.
float music_next_sample(MusicState *music);

#endif
