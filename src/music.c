#include "music.h"

#include "audio.h" // SAMPLE_RATE

#include <math.h>
#include <stddef.h>

// Reset to silent.
void music_init(MusicState *music)
{
    for (int v = 0; v < MUSIC_MAX_VOICES; v++)
    {
        music->voices[v].notes = NULL;
        music->voices[v].note_count = 0;
        music->voices[v].note_index = 0;
        music->voices[v].samples_into_note = 0.0;
        music->voices[v].phase = 0.0;
        music->voices[v].loop = 0;
        music->voices[v].active = 0;
    }

    // Conservative default so music mixes below sound effects the
    // moment any track starts playing, without needing a separate
    // tuning pass later - individual SFX amplitudes run 0.15-0.25 per
    // voice, and several can already be layered at once.
    music->volume = 0.15f;

    // Original encoded timing - see MusicState.playback_rate.
    music->playback_rate = 1.0;
}

void music_start_voice(
    MusicState *music,
    int voice_index,
    const MusicNote *notes,
    int note_count,
    int loop
)
{
    if (voice_index < 0 || voice_index >= MUSIC_MAX_VOICES)
    {
        return;
    }

    MusicVoice *voice = &music->voices[voice_index];

    voice->notes = notes;
    voice->note_count = note_count;
    voice->note_index = 0;
    voice->samples_into_note = 0.0;
    voice->phase = 0.0;
    voice->loop = loop;
    voice->active = (note_count > 0);
}

void music_stop_voice(MusicState *music, int voice_index)
{
    if (voice_index < 0 || voice_index >= MUSIC_MAX_VOICES)
    {
        return;
    }

    music->voices[voice_index].active = 0;
}

void music_stop_all(MusicState *music)
{
    for (int v = 0; v < MUSIC_MAX_VOICES; v++)
    {
        music->voices[v].active = 0;
    }
}

void music_set_volume(MusicState *music, float volume)
{
    music->volume = volume;
}

// Set playback rate, rejecting anything that could corrupt playback.
// A NaN, zero, negative, or infinite rate would either freeze
// samples_into_note forever (zero/negative can never reach a note's
// duration) or make every comparison against it meaningless (NaN
// comparisons are always false, infinity would satisfy the boundary
// check on its very first sample) - either reads as a stuck or
// instantly-skipped note to the player, so both fall back to the
// normal 1.0 rate rather than being allowed through. A finite but
// out-of-range value is clamped instead of rejected outright, since
// it's still a meaningful (if extreme) request.
void music_set_playback_rate(MusicState *music, double playback_rate)
{
    if (!isfinite(playback_rate) || playback_rate <= 0.0)
    {
        playback_rate = 1.0;
    }
    else if (playback_rate < MUSIC_PLAYBACK_RATE_MIN)
    {
        playback_rate = MUSIC_PLAYBACK_RATE_MIN;
    }
    else if (playback_rate > MUSIC_PLAYBACK_RATE_MAX)
    {
        playback_rate = MUSIC_PLAYBACK_RATE_MAX;
    }

    music->playback_rate = playback_rate;
}

// Advance one voice by a single sample at the given playback_rate,
// returning its raw (unscaled) waveform contribution - a plain square
// wave, the same synthesis style as every procedural SFX in audio.c,
// so music and sound effects share one consistent retro timbre.
static float advance_voice(MusicVoice *voice, double playback_rate)
{
    if (!voice->active || voice->notes == NULL || voice->note_count <= 0)
    {
        return 0.0f;
    }

    const MusicNote *note = &voice->notes[voice->note_index];

    float sample = 0.0f;

    if (note->frequency > 0.0)
    {
        sample = (sin(voice->phase) > 0.0) ? 1.0f : -1.0f;

        voice->phase += (2.0 * M_PI * note->frequency) / SAMPLE_RATE;
    }
    // A rest (frequency == 0) contributes silence but still advances
    // timing below, so it occupies its full duration like any note.

    // Advance by playback_rate rather than a flat 1 - at the default
    // 1.0 this is identical to the plain counter it replaced; at any
    // other rate, musical time moves proportionally faster or slower
    // while real audio time (one call per output sample) is unchanged.
    voice->samples_into_note += playback_rate;

    // A while loop, not a single if - subtracting each note's own
    // duration (rather than resetting to 0) carries any fractional or
    // multi-note overflow forward instead of discarding it, so a high
    // playback_rate landing partway through a short note can correctly
    // cascade through as many note boundaries as that one sample's
    // worth of musical time actually covers, with no drift accumulating
    // over a long loop and no note silently skipped.
    while (voice->active && voice->samples_into_note >= (double)note->duration_samples)
    {
        voice->samples_into_note -= (double)note->duration_samples;
        voice->note_index++;

        if (voice->note_index >= voice->note_count)
        {
            if (voice->loop)
            {
                voice->note_index = 0;
            }
            else
            {
                voice->active = 0;
                voice->samples_into_note = 0.0;
                break;
            }
        }

        note = &voice->notes[voice->note_index];

        // A fresh note always starts its own phase from zero rather
        // than carrying over the previous note's, matching how every
        // existing multi-note fanfare in audio.c already resets phase
        // at each note boundary - avoids a discontinuous waveform
        // jump between two different frequencies.
        voice->phase = 0.0;
    }

    return sample;
}

float music_next_sample(MusicState *music)
{
    float total = 0.0f;

    for (int v = 0; v < MUSIC_MAX_VOICES; v++)
    {
        // The same shared rate for every voice, applied identically -
        // this is what keeps melody and accompaniment synchronized
        // through a rate change instead of drifting apart.
        total += advance_voice(&music->voices[v], music->playback_rate);
    }

    return total * music->volume;
}
