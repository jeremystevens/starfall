#include "soundtrack.h"

// Title screen theme (v0.8.0 Phases 3B/4B) - the melody (voice 0,
// Phase 3B) plus its waltz accompaniment (voice 1, Phase 4B).
// Transcribed from the Mutopia Project's public-domain piano
// arrangement of Johann Strauss II's "The Blue Danube Waltz" (Op.
// 314), Music ID 519, arranged by N. Kouremenos, licensed CC BY-SA 4.0
// - the 1867 composition itself is long since public domain; this
// credits only the source arrangement this data was transcribed from.
// Bars 1-17 (including the pickup), verified note-by-note against
// both the LilyPond source and its MIDI export during Phases 3A/4A -
// see those phases' reports for the full transcription, the
// accompaniment's chord-reduction rule, and cross-checks. Do not
// hand-edit these values; regenerate from the verified source if a
// correction is ever needed.
static const MusicNote BLUE_DANUBE_MELODY[] =
{
    { 261.63, 17640 },  // C4, quarter (pickup)
    { 261.63, 17640 },  // C4, quarter
    { 329.63, 17640 },  // E4, quarter
    { 392.00, 17640 },  // G4, quarter
    { 392.00, 35280 },  // G4, half
    { 783.99, 17640 },  // G5, quarter
    { 783.99, 35280 },  // G5, half
    { 659.26, 17640 },  // E5, quarter
    { 659.26, 35280 },  // E5, half
    { 261.63, 17640 },  // C4, quarter
    { 261.63, 17640 },  // C4, quarter
    { 329.63, 17640 },  // E4, quarter
    { 392.00, 17640 },  // G4, quarter
    { 392.00, 35280 },  // G4, half
    { 783.99, 17640 },  // G5, quarter
    { 783.99, 35280 },  // G5, half
    { 698.46, 17640 },  // F5, quarter
    { 698.46, 35280 },  // F5, half
    { 246.94, 17640 },  // B3, quarter
    { 246.94, 17640 },  // B3, quarter
    { 293.66, 17640 },  // D4, quarter
    { 440.00, 17640 },  // A4, quarter
    { 440.00, 35280 },  // A4, half
    { 880.00, 17640 },  // A5, quarter
    { 880.00, 35280 },  // A5, half
    { 698.46, 17640 },  // F5, quarter
    { 698.46, 35280 },  // F5, half
    { 246.94, 17640 },  // B3, quarter
    { 246.94, 17640 },  // B3, quarter
    { 293.66, 17640 },  // D4, quarter
    { 440.00, 17640 },  // A4, quarter
    { 440.00, 35280 },  // A4, half
    { 880.00, 17640 },  // A5, quarter
    { 880.00, 35280 },  // A5, half
    { 659.26, 17640 },  // E5, quarter
    { 659.26, 35280 },  // E5, half
    { 261.63, 17640 },  // C4, quarter
};

#define BLUE_DANUBE_MELODY_COUNT \
    (sizeof(BLUE_DANUBE_MELODY) / sizeof(BLUE_DANUBE_MELODY[0]))

// Waltz accompaniment for voice 1 (Phase 4B) - a monophonic
// bass-on-beat-1 / chord-on-beats-2-and-3 reduction of the source's
// lower piano staff, source-verified during Phase 4A. Every
// beat-2/beat-3 chord in this excerpt is reduced to its top tone (G3),
// the one pitch common to both chord types the source actually uses
// here (<E3,G3> in the C-bass bars, <F3,G3> in the D-bass bars) - see
// Phase 4A's report for the full reduction rule and justification.
// The two leading rest events reproduce the source's own four beats
// of accompaniment silence under the melody's pickup and first bar;
// removing them would desync this voice from BLUE_DANUBE_MELODY.
// Deliberately the same 49 beats / 864,360 samples as voice 0 above -
// do not edit one without re-verifying the other still matches.
static const MusicNote BLUE_DANUBE_ACCOMPANIMENT[] =
{
    { 0.00, 17640 },    // bar1 pickup rest
    { 0.00, 52920 },    // bar2 full-bar rest
    { 130.81, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar3 C3 G3 G3
    { 130.81, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar4
    { 130.81, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar5
    { 130.81, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar6
    { 146.83, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar7 D3 G3 G3
    { 146.83, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar8
    { 146.83, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar9
    { 146.83, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar10
    { 146.83, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar11
    { 146.83, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar12
    { 146.83, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar13
    { 146.83, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar14
    { 130.81, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar15 C3 G3 G3
    { 130.81, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar16
    { 130.81, 17640 }, { 196.00, 17640 }, { 196.00, 17640 }, // bar17
};

#define BLUE_DANUBE_ACCOMPANIMENT_COUNT \
    (sizeof(BLUE_DANUBE_ACCOMPANIMENT) / sizeof(BLUE_DANUBE_ACCOMPANIMENT[0]))

// Dreadnought boss theme (v0.8.0 Phase 5B) - Edvard Grieg, "In the
// Hall of the Mountain King," Op. 46 No. 4 (Peer Gynt Suite I).
// Source: Mutopia Project Music ID 1888, Grieg's own public-domain
// piano reduction (both the 1874 composition and this piano source are
// public domain - no CC attribution requirement, unlike Blue Danube's
// arrangement). Bars 2-17 (16 bars, 4/4, base tempo 138 BPM), verified
// note-by-note against the LilyPond source and its MIDI export during
// Phase 5A, independently re-summed to confirm exact totals during
// Phase 5A.1 - see those phases' reports for the full transcription,
// the two documented octave-doubling reductions in the melody, and the
// reasoning behind the bar 2-17 loop boundary. Do not hand-edit these
// values or "correct" their ~0.126ms rounding difference from
// mathematically exact 138 BPM - that rounding is intentional (see
// Phase 5A.1); regenerate from the verified source instead if a real
// correction is ever needed.
static const MusicNote MOUNTAIN_KING_MELODY[] =
{
    // bar 2
    { 61.74, 9587 },  { 69.30, 9587 },  { 73.42, 9587 },  { 82.41, 9587 },
    { 92.50, 9587 },  { 73.42, 9587 },  { 92.50, 19174 },
    // bar 3
    { 87.31, 9587 },  { 69.30, 9587 },  { 87.31, 19174 },
    { 82.41, 9587 },  { 65.41, 9587 },  { 82.41, 19174 },
    // bar 4
    { 61.74, 9587 },  { 69.30, 9587 },  { 73.42, 9587 },  { 82.41, 9587 },
    { 92.50, 9587 },  { 73.42, 9587 },  { 92.50, 9587 },  { 123.47, 9587 },
    // bar 5
    { 110.00, 9587 }, { 92.50, 9587 },  { 73.42, 9587 },  { 92.50, 9587 }, { 110.00, 38348 },
    // bar 6
    { 123.47, 9587 }, { 138.59, 9587 }, { 146.83, 9587 }, { 164.81, 9587 },
    { 185.00, 9587 }, { 146.83, 9587 }, { 185.00, 19174 },
    // bar 7
    { 174.61, 9587 }, { 138.59, 9587 }, { 174.61, 19174 },
    { 164.81, 9587 }, { 130.81, 9587 }, { 164.81, 19174 },
    // bar 8
    { 123.47, 9587 }, { 138.59, 9587 }, { 146.83, 9587 }, { 164.81, 9587 },
    { 185.00, 9587 }, { 146.83, 9587 }, { 185.00, 9587 }, { 246.94, 9587 },
    // bar 9
    { 220.00, 9587 }, { 185.00, 9587 }, { 146.83, 9587 }, { 185.00, 9587 }, { 220.00, 38348 },
    // bar 10
    { 92.50, 9587 },  { 103.83, 9587 }, { 116.54, 9587 }, { 123.47, 9587 },
    { 138.59, 9587 }, { 116.54, 9587 }, { 138.59, 19174 },
    // bar 11
    { 146.83, 9587 }, { 116.54, 9587 }, { 146.83, 19174 },
    { 138.59, 9587 }, { 116.54, 9587 }, { 138.59, 19174 },
    // bar 12
    { 92.50, 9587 },  { 103.83, 9587 }, { 116.54, 9587 }, { 123.47, 9587 },
    { 138.59, 9587 }, { 116.54, 9587 }, { 138.59, 19174 },
    // bar 13
    { 146.83, 9587 }, { 116.54, 9587 }, { 146.83, 19174 }, { 138.59, 38348 },
    // bar 14
    { 185.00, 9587 }, { 207.65, 9587 }, { 233.08, 9587 }, { 246.94, 9587 },
    { 277.18, 9587 }, { 233.08, 9587 }, { 277.18, 19174 },
    // bar 15
    { 293.66, 9587 }, { 233.08, 9587 }, { 293.66, 19174 },
    { 277.18, 9587 }, { 233.08, 9587 }, { 277.18, 19174 },
    // bar 16
    { 185.00, 9587 }, { 207.65, 9587 }, { 233.08, 9587 }, { 246.94, 9587 },
    { 277.18, 9587 }, { 233.08, 9587 }, { 277.18, 19174 },
    // bar 17
    { 293.66, 9587 }, { 233.08, 9587 }, { 293.66, 19174 }, { 277.18, 38348 },
};

#define MOUNTAIN_KING_MELODY_COUNT \
    (sizeof(MOUNTAIN_KING_MELODY) / sizeof(MOUNTAIN_KING_MELODY[0]))

// Voice 1 for the Dreadnought theme - the source's own left-hand bass
// drone (root/fifth alternation), already monophonic in the source
// with no chord reduction needed - see Phase 5A's report. Same 64
// beats / 1,227,136 samples as MOUNTAIN_KING_MELODY above - do not
// edit one without re-verifying the other still matches (Phase 5A.1
// re-confirmed this exact total independently for both arrays).
static const MusicNote MOUNTAIN_KING_ACCOMPANIMENT[] =
{
    // bars 2-4 (B minor drone)
    { 30.87, 19174 }, { 46.25, 19174 }, { 30.87, 19174 }, { 46.25, 19174 },
    { 30.87, 19174 }, { 46.25, 19174 }, { 30.87, 19174 }, { 46.25, 19174 },
    { 30.87, 19174 }, { 46.25, 19174 }, { 30.87, 19174 }, { 46.25, 19174 },
    // bar 5 (cadence)
    { 36.71, 19174 }, { 55.00, 19174 }, { 36.71, 19174 }, { 55.00, 19174 },
    // bars 6-8 (repeat)
    { 30.87, 19174 }, { 46.25, 19174 }, { 30.87, 19174 }, { 46.25, 19174 },
    { 30.87, 19174 }, { 46.25, 19174 }, { 30.87, 19174 }, { 46.25, 19174 },
    { 30.87, 19174 }, { 46.25, 19174 }, { 30.87, 19174 }, { 46.25, 19174 },
    // bar 9 (cadence)
    { 36.71, 19174 }, { 55.00, 19174 }, { 36.71, 19174 }, { 55.00, 19174 },
    // bars 10-13 (transposed sequence)
    { 46.25, 19174 }, { 69.30, 19174 }, { 46.25, 19174 }, { 69.30, 19174 },
    { 36.71, 19174 }, { 58.27, 19174 }, { 46.25, 19174 }, { 69.30, 19174 },
    { 46.25, 19174 }, { 69.30, 19174 }, { 46.25, 19174 }, { 69.30, 19174 },
    { 36.71, 19174 }, { 58.27, 19174 }, { 46.25, 19174 }, { 69.30, 19174 },
    // bars 14-17 (repeat)
    { 46.25, 19174 }, { 69.30, 19174 }, { 46.25, 19174 }, { 69.30, 19174 },
    { 36.71, 19174 }, { 58.27, 19174 }, { 46.25, 19174 }, { 69.30, 19174 },
    { 46.25, 19174 }, { 69.30, 19174 }, { 46.25, 19174 }, { 69.30, 19174 },
    { 36.71, 19174 }, { 58.27, 19174 }, { 46.25, 19174 }, { 69.30, 19174 },
};

#define MOUNTAIN_KING_ACCOMPANIMENT_COUNT \
    (sizeof(MOUNTAIN_KING_ACCOMPANIMENT) / sizeof(MOUNTAIN_KING_ACCOMPANIMENT[0]))

// Game Over theme (v0.8.0 Phase 6B) - Frederic Chopin, "Marche
// funebre," Piano Sonata No. 2 in B-flat minor, Op. 35, Mvt. III.
// Source: Chopin Online (CFEO/OCVE, Universities of Cambridge/King's
// College London), facsimile of the 1840 Breitkopf & Hartel first
// edition - public domain. Bars 1-2 only (identical bars in the
// source; verified note-by-note during Phase 6A - see that phase's
// report for the full transcription, the bass-clef notation of the
// melody, and the chord-reduction rule for the accompaniment). The
// ~20-35s target was deliberately not reached: bar 3 onward could not
// be verified to this project's required standard, so the excerpt was
// intentionally shortened rather than guessed. BASE_BPM = 60 is an
// arrangement choice (Chopin's score gives only "Lento," no numeric
// tempo) - do not treat it as a source fact. Played once, not looped
// (see Phase 6B) - bar 2 already repeats bar 1 verbatim, so looping
// this excerpt would make the repetition too obvious for a one-time
// Game Over cue.
static const MusicNote FUNERAL_MARCH_MELODY[] =
{
    { 233.08, 44100 },  // Bb3, quarter      (bar 1)
    { 233.08, 33075 },  // Bb3, dotted-eighth
    { 207.65, 11025 },  // Ab3, sixteenth
    { 261.63, 88200 },  // C4,  half
    { 233.08, 44100 },  // Bb3, quarter      (bar 2)
    { 233.08, 33075 },  // Bb3, dotted-eighth
    { 207.65, 11025 },  // Ab3, sixteenth
    { 261.63, 88200 },  // C4,  half
};

#define FUNERAL_MARCH_MELODY_COUNT \
    (sizeof(FUNERAL_MARCH_MELODY) / sizeof(FUNERAL_MARCH_MELODY[0]))

// Accompaniment: the source's low open fifth (Bb1+F2, no third)
// reduced to its root (Bb1) - see Phase 6A's report for why the root
// was chosen over the fifth. Same 8 beats / 352,800 samples as
// FUNERAL_MARCH_MELODY above - do not edit one without re-verifying
// the other still matches.
static const MusicNote FUNERAL_MARCH_ACCOMPANIMENT[] =
{
    { 58.27, 44100 },  // Bb1, quarter (bar 1, attack 1)
    { 58.27, 44100 },  // Bb1, quarter (bar 1, attack 2)
    { 58.27, 44100 },  // Bb1, quarter (bar 1, attack 3)
    { 58.27, 44100 },  // Bb1, quarter (bar 1, attack 4)
    { 58.27, 44100 },  // Bb1, quarter (bar 2, attack 1)
    { 58.27, 44100 },  // Bb1, quarter (bar 2, attack 2)
    { 58.27, 44100 },  // Bb1, quarter (bar 2, attack 3)
    { 58.27, 44100 },  // Bb1, quarter (bar 2, attack 4)
};

#define FUNERAL_MARCH_ACCOMPANIMENT_COUNT \
    (sizeof(FUNERAL_MARCH_ACCOMPANIMENT) / sizeof(FUNERAL_MARCH_ACCOMPANIMENT[0]))

// Start the title theme - both voices, looping, sample-aligned, at
// 1.0x. audio_music_start_dual() resets playback_rate to 1.0 as part
// of the same locked call, so a boss fight's tempo increase can never
// leak into a subsequent title-theme start.
void soundtrack_play_title(LaserSound *audio)
{
    audio_music_start_dual(
        audio,
        BLUE_DANUBE_MELODY, BLUE_DANUBE_MELODY_COUNT, 1,
        BLUE_DANUBE_ACCOMPANIMENT, BLUE_DANUBE_ACCOMPANIMENT_COUNT, 1
    );
}

// Start the boss theme - both voices, looping, sample-aligned, at
// 1.0x. Intensity changes afterward go through
// soundtrack_set_boss_intensity() below, never through this function
// again, so a tier change is never mistaken for a fresh encounter.
void soundtrack_play_boss(LaserSound *audio)
{
    audio_music_start_dual(
        audio,
        MOUNTAIN_KING_MELODY, MOUNTAIN_KING_MELODY_COUNT, 1,
        MOUNTAIN_KING_ACCOMPANIMENT, MOUNTAIN_KING_ACCOMPANIMENT_COUNT, 1
    );
}

// Map the Dreadnought's intensity phase to the boss theme's playback
// rate and apply it - only the rate setter is called, so both voices'
// note_index/phase/samples_into_note carry on completely undisturbed,
// exactly where the performance already was. Any phase other than 2
// or 3 (including 1) resolves to the normal 1.00x rate.
void soundtrack_set_boss_intensity(LaserSound *audio, int phase)
{
    double rate = 1.00;

    if (phase == 2)
    {
        rate = 1.15;
    }
    else if (phase == 3)
    {
        rate = 1.30;
    }

    audio_music_set_playback_rate(audio, rate);
}

// Start the Game Over theme - both voices, not looping. Bar 2 already
// repeats bar 1 verbatim in the source, so looping this excerpt would
// make the repetition too obvious for what's meant to be a single
// Game Over cue (see Phase 6A/6B).
void soundtrack_play_game_over(LaserSound *audio)
{
    audio_music_start_dual(
        audio,
        FUNERAL_MARCH_MELODY, FUNERAL_MARCH_MELODY_COUNT, 0,
        FUNERAL_MARCH_ACCOMPANIMENT, FUNERAL_MARCH_ACCOMPANIMENT_COUNT, 0
    );
}

// Stop whichever soundtrack composition is currently playing.
void soundtrack_stop(LaserSound *audio)
{
    audio_music_stop(audio);
}
