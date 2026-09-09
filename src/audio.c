#include "audio.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// Player laser: a descending square-wave "pew".
// (SAMPLE_RATE now lives in audio.h, shared with music.c.)
#define LASER_FREQUENCY 900.0
#define LASER_DURATION 4000

// Scout laser: lower and shorter than the player's.
#define ENEMY_LASER_FREQUENCY 450.0
#define ENEMY_LASER_DURATION 2500

// Bomber's bomb drop: a short, low thunk rather than a laser zap -
// it should sound like something heavy being released, not fired.
#define BOMB_DROP_FREQUENCY 220.0
#define BOMB_DROP_DURATION 1800

// Player ship destruction: a long, low noise burst.
#define EXPLOSION_RUMBLE_FREQUENCY 70.0
#define EXPLOSION_DURATION 16000

// Scout destruction: a short, higher-pitched noise burst.
#define ENEMY_EXPLOSION_RUMBLE_FREQUENCY 150.0
#define ENEMY_EXPLOSION_DURATION 7000

// Power-up pickup: a short, bright tone that slides UP in pitch -
// the opposite direction from every laser in the game, so a pickup
// never gets mistaken for a weapon or an impact.
#define PICKUP_FREQUENCY 500.0
#define PICKUP_MAX_FREQUENCY 1200.0
#define PICKUP_DURATION 3000

// Extra life fanfare: a short 4-note ascending major arpeggio (C5,
// E5, G5, C6) rather than a single sliding tone - a classic "happy
// 1-up jingle" reads as several distinct notes, not a sweep, and it
// needs to stand apart from the pickup sound's own upward slide.
#define EXTRA_LIFE_NOTE_COUNT 4
static const double EXTRA_LIFE_NOTES[EXTRA_LIFE_NOTE_COUNT] =
{
    523.25, // C5
    659.25, // E5
    784.00, // G5
    1046.50 // C6
};
#define EXTRA_LIFE_NOTE_DURATION 2200

// New high score fanfare: a full ascending C major scale (C5 through
// C6, 8 notes rather than the extra life arpeggio's 4) with the final
// note held far longer than the rest - a longer, more triumphant
// shape than the extra life jingle, and built from different notes
// (every scale degree, not just 1-3-5-8) so the two are impossible to
// mix up by ear even played back to back.
#define NEW_HIGH_SCORE_NOTE_COUNT 8
static const double NEW_HIGH_SCORE_NOTES[NEW_HIGH_SCORE_NOTE_COUNT] =
{
    523.25, // C5
    587.33, // D5
    659.25, // E5
    698.46, // F5
    783.99, // G5
    880.00, // A5
    987.77, // B5
    1046.50 // C6 - held (see NEW_HIGH_SCORE_FINAL_NOTE_DURATION)
};
#define NEW_HIGH_SCORE_NOTE_DURATION 1400
#define NEW_HIGH_SCORE_FINAL_NOTE_DURATION 6000

// Dreadnought warning klaxon: a deep, continuous LOW -> HIGH -> LOW
// sweep, repeating for as long as audio_set_boss_warning() leaves it
// switched on - a ship's-alarm feel, and pitched well below every
// other tone in the game (the deepest of those, the bomb-drop thunk,
// bottoms out at 60Hz; this never goes above 240Hz) so it reads as
// distinctly deeper/heavier than anything else that plays.
#define BOSS_WARNING_LOW_FREQ 110.0
#define BOSS_WARNING_HIGH_FREQ 240.0
#define BOSS_WARNING_PULSE_DURATION_MS 500
#define BOSS_WARNING_PULSE_SAMPLES ((SAMPLE_RATE * BOSS_WARNING_PULSE_DURATION_MS) / 1000)

// The SDL audio device we open for playback.
static SDL_AudioDeviceID audio_device = 0;


// SDL calls this whenever it needs more audio samples. All of our
// sound effects are generated here and mixed into a single buffer.
static void audio_callback(void *userdata, Uint8 *stream, int len)
{
    float *buffer = (float *)stream;

    int sample_count = len / sizeof(float);

    LaserSound *sound = (LaserSound *)userdata;


    for (int i = 0; i < sample_count; i++)
    {
        float sample = 0.0f;

        if (sound->samples_remaining > 0)
        {
            // Square wave for a crunchy retro laser sound.
            sample +=
            (sin(sound->phase) > 0.0) ? 0.2f : -0.2f;

            // Slide the frequency down for the descending "pew".
            sound->frequency -= 2.0;

            if (sound->frequency < 100.0)
            {
                sound->frequency = 100.0;
            }


            sound->phase +=
                (2.0 * M_PI * sound->frequency) / SAMPLE_RATE;


            sound->samples_remaining--;
        }

        if (sound->enemy_samples_remaining > 0)
        {
            // Lower, harsher square wave for the Scout's weapon.
            sample +=
            (sin(sound->enemy_phase) > 0.0) ? 0.15f : -0.15f;

            sound->enemy_frequency -= 1.0;

            if (sound->enemy_frequency < 80.0)
            {
                sound->enemy_frequency = 80.0;
            }

            sound->enemy_phase +=
                (2.0 * M_PI * sound->enemy_frequency) / SAMPLE_RATE;

            sound->enemy_samples_remaining--;
        }

        if (sound->bomb_samples_remaining > 0)
        {
            // A short, low thunk for the Bomber releasing its payload.
            // Pitched well below both lasers so it never gets mistaken
            // for a shot being fired.
            sample +=
            (sin(sound->bomb_phase) > 0.0) ? 0.18f : -0.18f;

            sound->bomb_frequency -= 0.6;

            if (sound->bomb_frequency < 60.0)
            {
                sound->bomb_frequency = 60.0;
            }

            sound->bomb_phase +=
                (2.0 * M_PI * sound->bomb_frequency) / SAMPLE_RATE;

            sound->bomb_samples_remaining--;
        }

        // Player ship destruction: filtered white noise plus a low
        // rumble tone, both fading out as the burst plays.
        if (sound->explosion_samples_remaining > 0)
        {
            float progress =
                (float)sound->explosion_samples_remaining /
                (float)sound->explosion_samples_total;

            float noise = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
            float rumble = (sin(sound->explosion_rumble_phase) > 0.0) ? 1.0f : -1.0f;

            sample += (noise * 0.5f + rumble * 0.25f) * progress * 0.3f;

            sound->explosion_rumble_phase +=
                (2.0 * M_PI * EXPLOSION_RUMBLE_FREQUENCY) / SAMPLE_RATE;

            sound->explosion_samples_remaining--;
        }

        // Scout destruction: the same noise-burst technique, but
        // shorter and higher pitched so it's easy to tell apart
        // from the player's explosion.
        if (sound->enemy_explosion_samples_remaining > 0)
        {
            float progress =
                (float)sound->enemy_explosion_samples_remaining /
                (float)sound->enemy_explosion_samples_total;

            float noise = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
            float rumble =
                (sin(sound->enemy_explosion_rumble_phase) > 0.0) ? 1.0f : -1.0f;

            sample += (noise * 0.35f + rumble * 0.2f) * progress * 0.25f;

            sound->enemy_explosion_rumble_phase +=
                (2.0 * M_PI * ENEMY_EXPLOSION_RUMBLE_FREQUENCY) / SAMPLE_RATE;

            sound->enemy_explosion_samples_remaining--;
        }

        // Power-up pickup: a bright square wave sliding UP in pitch -
        // the only rising sound in the game.
        if (sound->pickup_samples_remaining > 0)
        {
            sample +=
            (sin(sound->pickup_phase) > 0.0) ? 0.2f : -0.2f;

            sound->pickup_frequency += 3.0;

            if (sound->pickup_frequency > PICKUP_MAX_FREQUENCY)
            {
                sound->pickup_frequency = PICKUP_MAX_FREQUENCY;
            }

            sound->pickup_phase +=
                (2.0 * M_PI * sound->pickup_frequency) / SAMPLE_RATE;

            sound->pickup_samples_remaining--;
        }

        // Extra life fanfare: step through EXTRA_LIFE_NOTES one note
        // at a time - each note is its own fixed-frequency square
        // wave, so the notes land as a crisp arpeggio instead of
        // sliding into each other.
        if (sound->extra_life_note_index < EXTRA_LIFE_NOTE_COUNT)
        {
            double frequency = EXTRA_LIFE_NOTES[sound->extra_life_note_index];

            sample +=
            (sin(sound->extra_life_phase) > 0.0) ? 0.22f : -0.22f;

            sound->extra_life_phase +=
                (2.0 * M_PI * frequency) / SAMPLE_RATE;

            sound->extra_life_samples_remaining--;

            if (sound->extra_life_samples_remaining <= 0)
            {
                sound->extra_life_note_index++;
                sound->extra_life_samples_remaining = EXTRA_LIFE_NOTE_DURATION;
                sound->extra_life_phase = 0.0;
            }
        }

        // New high score fanfare: same note-stepping technique as the
        // extra life fanfare above, but every note past the first is
        // only reached once the previous one finishes, and the last
        // note (index NEW_HIGH_SCORE_NOTE_COUNT - 1) gets the much
        // longer NEW_HIGH_SCORE_FINAL_NOTE_DURATION instead of the
        // brief duration every earlier note uses - that's what turns
        // the last step of the scale into a held ending rather than
        // just another quick note.
        if (sound->new_high_score_note_index < NEW_HIGH_SCORE_NOTE_COUNT)
        {
            double frequency =
                NEW_HIGH_SCORE_NOTES[sound->new_high_score_note_index];

            sample +=
            (sin(sound->new_high_score_phase) > 0.0) ? 0.22f : -0.22f;

            sound->new_high_score_phase +=
                (2.0 * M_PI * frequency) / SAMPLE_RATE;

            sound->new_high_score_samples_remaining--;

            if (sound->new_high_score_samples_remaining <= 0)
            {
                sound->new_high_score_note_index++;
                sound->new_high_score_phase = 0.0;

                int is_final_note =
                    (sound->new_high_score_note_index ==
                     NEW_HIGH_SCORE_NOTE_COUNT - 1);

                sound->new_high_score_samples_remaining =
                    is_final_note
                    ? NEW_HIGH_SCORE_FINAL_NOTE_DURATION
                    : NEW_HIGH_SCORE_NOTE_DURATION;
            }
        }

        // Dreadnought warning klaxon: unlike every sound above, this
        // has no samples_remaining counting down to silence - it just
        // keeps sweeping for as long as boss_warning_active stays set.
        // boss_warning_pulse_sample counts up through one sweep and
        // wraps back to 0, so the frequency ramps LOW -> HIGH, snaps
        // back to LOW, and repeats indefinitely.
        if (sound->boss_warning_active)
        {
            sound->boss_warning_pulse_sample++;

            if (sound->boss_warning_pulse_sample >= BOSS_WARNING_PULSE_SAMPLES)
            {
                sound->boss_warning_pulse_sample = 0;
            }

            double pulse_fraction =
                (double)sound->boss_warning_pulse_sample /
                (double)BOSS_WARNING_PULSE_SAMPLES;

            double frequency =
                BOSS_WARNING_LOW_FREQ +
                (BOSS_WARNING_HIGH_FREQ - BOSS_WARNING_LOW_FREQ) * pulse_fraction;

            sample +=
            (sin(sound->boss_warning_phase) > 0.0) ? 0.2f : -0.2f;

            sound->boss_warning_phase +=
                (2.0 * M_PI * frequency) / SAMPLE_RATE;
        }

        // Music (see music.h) - mixed in last, already scaled down by
        // its own independent volume so it sits below whatever SFX
        // happen to be playing at the same time.
        sample += music_next_sample(&sound->music);

        // Safety clamp on the final mixed signal - with this many
        // independent voices able to overlap (SFX and now music
        // together), nothing before this point guarantees the sum
        // stays within the [-1, 1] range AUDIO_F32SYS expects, and an
        // unclamped sample here would clip/distort at the driver
        // instead of just capping cleanly.
        if (sample > 1.0f)
        {
            sample = 1.0f;
        }
        else if (sample < -1.0f)
        {
            sample = -1.0f;
        }

        buffer[i] = sample;
    }
}


// Open the default playback device and zero out every sound effect.
int audio_init(LaserSound *laser)
{
    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;


    SDL_zero(desired);


    desired.freq = SAMPLE_RATE;
    desired.format = AUDIO_F32SYS;
    desired.channels = 1;
    desired.samples = 512;

    desired.callback = audio_callback;
    desired.userdata = laser;


    laser->phase = 0.0;
    laser->frequency = LASER_FREQUENCY;
    laser->samples_remaining = 0;

    laser->enemy_phase = 0.0;
    laser->enemy_frequency = ENEMY_LASER_FREQUENCY;
    laser->enemy_samples_remaining = 0;

    laser->bomb_phase = 0.0;
    laser->bomb_frequency = BOMB_DROP_FREQUENCY;
    laser->bomb_samples_remaining = 0;

    laser->explosion_rumble_phase = 0.0;
    laser->explosion_samples_remaining = 0;
    laser->explosion_samples_total = EXPLOSION_DURATION;

    laser->enemy_explosion_rumble_phase = 0.0;
    laser->enemy_explosion_samples_remaining = 0;
    laser->enemy_explosion_samples_total = ENEMY_EXPLOSION_DURATION;

    laser->pickup_phase = 0.0;
    laser->pickup_frequency = PICKUP_FREQUENCY;
    laser->pickup_samples_remaining = 0;

    laser->extra_life_phase = 0.0;
    laser->extra_life_note_index = EXTRA_LIFE_NOTE_COUNT; // idle
    laser->extra_life_samples_remaining = 0;

    laser->new_high_score_phase = 0.0;
    laser->new_high_score_note_index = NEW_HIGH_SCORE_NOTE_COUNT; // idle
    laser->new_high_score_samples_remaining = 0;

    laser->boss_warning_active = 0;
    laser->boss_warning_phase = 0.0;
    laser->boss_warning_pulse_sample = 0;

    music_init(&laser->music);

    audio_device = SDL_OpenAudioDevice(
        NULL,
        0,
        &desired,
        &obtained,
        0
    );


    if (audio_device == 0)
    {
        printf(
            "Failed to open audio device: %s\n",
            SDL_GetError()
        );

        return 0;
    }


    // Audio devices start paused; unpause to begin playback.
    SDL_PauseAudioDevice(audio_device, 0);


    return 1;
}


// Trigger the player's laser sound.
void audio_play_laser(LaserSound *laser)
{
    if (audio_device == 0)
    {
        return;
    }


    /*
     * SDL's audio callback can access LaserSound at the same
     * time as the main game loop.
     *
     * Lock the device while changing this shared data so the
     * callback cannot read it halfway through an update.
     */
    SDL_LockAudioDevice(audio_device);


    laser->phase = 0.0;
    laser->frequency = LASER_FREQUENCY;
    laser->samples_remaining = LASER_DURATION;


    SDL_UnlockAudioDevice(audio_device);
}

// Trigger the Scout's laser sound.
void audio_play_enemy_laser(LaserSound *laser)
{
    if (audio_device == 0)
    {
        return;
    }

    // Protect shared audio state from the SDL audio callback.
    SDL_LockAudioDevice(audio_device);

    laser->enemy_phase = 0.0;
    laser->enemy_frequency = ENEMY_LASER_FREQUENCY;
    laser->enemy_samples_remaining = ENEMY_LASER_DURATION;

    SDL_UnlockAudioDevice(audio_device);
}

// Trigger the Bomber's bomb-drop sound.
void audio_play_bomb_drop(LaserSound *laser)
{
    if (audio_device == 0)
    {
        return;
    }

    // Protect shared audio state from the SDL audio callback.
    SDL_LockAudioDevice(audio_device);

    laser->bomb_phase = 0.0;
    laser->bomb_frequency = BOMB_DROP_FREQUENCY;
    laser->bomb_samples_remaining = BOMB_DROP_DURATION;

    SDL_UnlockAudioDevice(audio_device);
}

// Trigger the player ship's destruction sound.
void audio_play_explosion(LaserSound *laser)
{
    if (audio_device == 0)
    {
        return;
    }

    SDL_LockAudioDevice(audio_device);

    laser->explosion_rumble_phase = 0.0;
    laser->explosion_samples_total = EXPLOSION_DURATION;
    laser->explosion_samples_remaining = EXPLOSION_DURATION;

    SDL_UnlockAudioDevice(audio_device);
}

// Trigger the Scout's destruction sound.
void audio_play_enemy_explosion(LaserSound *laser)
{
    if (audio_device == 0)
    {
        return;
    }

    SDL_LockAudioDevice(audio_device);

    laser->enemy_explosion_rumble_phase = 0.0;
    laser->enemy_explosion_samples_total = ENEMY_EXPLOSION_DURATION;
    laser->enemy_explosion_samples_remaining = ENEMY_EXPLOSION_DURATION;

    SDL_UnlockAudioDevice(audio_device);
}

// Trigger the power-up pickup sound.
void audio_play_pickup(LaserSound *laser)
{
    if (audio_device == 0)
    {
        return;
    }

    SDL_LockAudioDevice(audio_device);

    laser->pickup_phase = 0.0;
    laser->pickup_frequency = PICKUP_FREQUENCY;
    laser->pickup_samples_remaining = PICKUP_DURATION;

    SDL_UnlockAudioDevice(audio_device);
}

// Trigger the extra life fanfare.
void audio_play_extra_life(LaserSound *laser)
{
    if (audio_device == 0)
    {
        return;
    }

    SDL_LockAudioDevice(audio_device);

    laser->extra_life_phase = 0.0;
    laser->extra_life_note_index = 0;
    laser->extra_life_samples_remaining = EXTRA_LIFE_NOTE_DURATION;

    SDL_UnlockAudioDevice(audio_device);
}

// Trigger the new high score fanfare.
void audio_play_new_high_score(LaserSound *laser)
{
    if (audio_device == 0)
    {
        return;
    }

    SDL_LockAudioDevice(audio_device);

    laser->new_high_score_phase = 0.0;
    laser->new_high_score_note_index = 0;
    laser->new_high_score_samples_remaining = NEW_HIGH_SCORE_NOTE_DURATION;

    SDL_UnlockAudioDevice(audio_device);
}

// True once new_high_score_note_index has reached NEW_HIGH_SCORE_NOTE_COUNT
// - the exact same "idle" value audio_init() and audio_stop_new_high_score()
// both use, and the value the audio callback itself only ever reaches once
// the fanfare's real last note has actually finished. If the device never
// opened, there's nothing to wait for either.
int audio_new_high_score_finished(const LaserSound *laser)
{
    if (audio_device == 0)
    {
        return 1;
    }

    SDL_LockAudioDevice(audio_device);

    int finished =
        (laser->new_high_score_note_index >= NEW_HIGH_SCORE_NOTE_COUNT);

    SDL_UnlockAudioDevice(audio_device);

    return finished;
}

// Force the fanfare to the same idle state a normal completion leaves
// it in - see audio_init()'s identical assignment. Silences it
// immediately rather than waiting for its remaining notes to play out.
void audio_stop_new_high_score(LaserSound *laser)
{
    if (audio_device == 0)
    {
        return;
    }

    SDL_LockAudioDevice(audio_device);

    laser->new_high_score_note_index = NEW_HIGH_SCORE_NOTE_COUNT;

    SDL_UnlockAudioDevice(audio_device);
}

// Turn the Dreadnought warning klaxon on or off.
void audio_set_boss_warning(LaserSound *laser, int active)
{
    if (audio_device == 0)
    {
        return;
    }

    SDL_LockAudioDevice(audio_device);

    laser->boss_warning_active = active;

    // Reset the sweep to the start every time this is (re)enabled, so
    // a fresh warning always begins at the low end of the pulse rather
    // than wherever a previous warning happened to leave off.
    if (active)
    {
        laser->boss_warning_phase = 0.0;
        laser->boss_warning_pulse_sample = 0;
    }

    SDL_UnlockAudioDevice(audio_device);
}

// Start a music track on the melody voice. Locked the same way every
// audio_play_*() function above already is, delegating the actual
// field mutation to music.c's own (lock-free) music_start_voice() -
// music.c has no idea SDL, an audio device, or locking exist.
void audio_music_start(
    LaserSound *laser,
    const MusicNote *notes,
    int note_count,
    int loop
)
{
    if (audio_device == 0)
    {
        return;
    }

    SDL_LockAudioDevice(audio_device);

    // Every new composition explicitly starts at normal speed - see
    // this function's doc comment in audio.h for why this reset lives
    // here rather than being left to each call site to remember.
    music_set_playback_rate(&laser->music, 1.0);

    music_start_voice(&laser->music, 0, notes, note_count, loop);

    SDL_UnlockAudioDevice(audio_device);
}

// Start a music track on an arbitrary voice, without touching
// playback_rate - see this function's doc comment in audio.h.
void audio_music_start_voice(
    LaserSound *laser,
    int voice_index,
    const MusicNote *notes,
    int note_count,
    int loop
)
{
    if (audio_device == 0)
    {
        return;
    }

    SDL_LockAudioDevice(audio_device);

    music_start_voice(&laser->music, voice_index, notes, note_count, loop);

    SDL_UnlockAudioDevice(audio_device);
}

// Start two voices as a single locked operation, both from event 0,
// with playback_rate reset to 1.0 first. Exists so a two-voice
// composition (e.g. Blue Danube's melody + accompaniment) can begin
// with both voices sample-aligned - calling audio_music_start() then
// audio_music_start_voice() separately would still be correct, but
// leaves a window between the two unlocks where the audio callback
// could run and read voice 0 already active while voice 1 is still
// silent, offsetting the two voices' start positions by however many
// samples that callback produced. Generic over which two tracks -
// like every other function here, it has no idea one of them is a
// title theme.
void audio_music_start_dual(
    LaserSound *laser,
    const MusicNote *notes0,
    int note_count0,
    int loop0,
    const MusicNote *notes1,
    int note_count1,
    int loop1
)
{
    if (audio_device == 0)
    {
        return;
    }

    SDL_LockAudioDevice(audio_device);

    music_set_playback_rate(&laser->music, 1.0);

    music_start_voice(&laser->music, 0, notes0, note_count0, loop0);
    music_start_voice(&laser->music, 1, notes1, note_count1, loop1);

    SDL_UnlockAudioDevice(audio_device);
}

// Stop all music voices immediately.
void audio_music_stop(LaserSound *laser)
{
    if (audio_device == 0)
    {
        return;
    }

    SDL_LockAudioDevice(audio_device);

    music_stop_all(&laser->music);

    SDL_UnlockAudioDevice(audio_device);
}

// Set the overall music volume (0.0-1.0).
void audio_music_set_volume(LaserSound *laser, float volume)
{
    if (audio_device == 0)
    {
        return;
    }

    SDL_LockAudioDevice(audio_device);

    music_set_volume(&laser->music, volume);

    SDL_UnlockAudioDevice(audio_device);
}

// Set how fast musical time advances for every voice.
void audio_music_set_playback_rate(LaserSound *laser, double playback_rate)
{
    if (audio_device == 0)
    {
        return;
    }

    SDL_LockAudioDevice(audio_device);

    music_set_playback_rate(&laser->music, playback_rate);

    SDL_UnlockAudioDevice(audio_device);
}

// Close the audio device during shutdown.
void audio_shutdown(void)
{
    if (audio_device != 0)
    {
        SDL_CloseAudioDevice(audio_device);

        audio_device = 0;
    }
}
