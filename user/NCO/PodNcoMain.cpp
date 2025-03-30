#include "daisysp.h"
#include "daisy_pod.h"
#include "nco.h"
#include "harmonicNco.h"

#define MIDI_C4                     60
#define MIDI_A4                     69
#define FREQ_A4                     440
#define NUM_MIDI_NOTES_PER_OCTAVE   12

using namespace daisysp;
using namespace daisy;

enum NoteInterval
{
    ROOT,
    THIRD,
    FIFTH,
    NUM_INTERVALS
};

enum ChordSequence
{
    CHORD_I,
    CHORD_V,
    CHORD_VI_M,
    CHORD_IV,
    CHORD_SEQ_LEN
};

enum BasicChordType
{
    MAJOR,
    MINOR
};

static void UpdateChord(uint16_t root_midi_note, BasicChordType chord_type);
static void AdvanceChord(uint16_t root_midi_note, int direction);

static DaisyPod     pod;
static HarmonicNCO  harmonicNco[NUM_INTERVALS];
static float        amp             = 0.5f;
// static uint16_t     s_midi_note     = MIDI_A4;
static uint16_t     s_key_root_note = MIDI_C4;
// static uint16_t     prev_midi_note  = MIDI_A4;

static float        harmonicAmps[] = 
{
    1.0,
    0.7,
    0.6,
    0.3,
    0.0,
    0.0,
    0.0,
    0.0
};

static float        harmonicPhases[] = 
{
    0.0,
    0.9,
    0.125,
    0.45,
    0.65,
    0.95,
    0.55,
    0.6
};

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out, 
                          size_t                                size)
{
    float sample = 0;
    int encoder_dir = 0;

    pod.ProcessAllControls();

    // s_midi_note += pod.encoder.Increment();
    // s_midi_note = DSY_CLAMP(s_midi_note, 0, 127);

    // if (s_midi_note != prev_midi_note) 
    // {
    //     UpdateChord(s_midi_note);

    //     prev_midi_note = s_midi_note;
    // }

    encoder_dir = pod.encoder.Increment();

    if (encoder_dir)
    {
        AdvanceChord(s_key_root_note, encoder_dir);
    }

    // Audio Loop
    for (size_t ndx = 0; ndx < size; ndx += 2)
    {
        for (uint8_t note = ROOT; note < NUM_INTERVALS; note++)
        {
            sample += (amp / NUM_INTERVALS) * harmonicNco[note].NextSample();
        }
        // left out
        out[ndx] = sample;
        // right out
        out[ndx + 1] = sample;

        sample = 0;
    }
}

static void InitChordNcos(uint16_t root_midi_note, uint32_t sample_rate)
{
    for (uint8_t note = ROOT; note < NUM_INTERVALS; note++)
    {
        harmonicNco[note].SetSampleRate(sample_rate);
        harmonicNco[note].SetAmplitudes(harmonicAmps);
        harmonicNco[note].SetPhases(harmonicPhases);
    }

    // Set the frequencies of each NCO
    UpdateChord(root_midi_note, MAJOR);
}

// Sets the NCO's to play a major chord
static void UpdateChord(uint16_t root_midi_note, BasicChordType chord_type)
{
    float       freq        = 0;
    uint16_t    midi_note   = root_midi_note;
    
    for (uint8_t note = ROOT; note < NUM_INTERVALS; note++)
    {
        // Apply offset based on interval
        midi_note  = root_midi_note + (3 * note);

        if (chord_type == MAJOR)
        {
            midi_note += (note != ROOT) ? 1 : 0;
        }
        else
        {
            midi_note += (note == FIFTH) ? 1 : 0;
        }
        // Keep chord notes w/in an octave of the key root note
        midi_note = ((midi_note - s_key_root_note) % NUM_MIDI_NOTES_PER_OCTAVE) + s_key_root_note;
        
        freq = mtof(midi_note);
        harmonicNco[note].SetFrequency(freq);
    }
}

static void AdvanceChord(uint16_t root_midi_note, int direction)
{
    static uint8_t current_chord = CHORD_I;

    // Advance to next chord
    if (direction > 0)
    {
        current_chord = (current_chord + 1) % CHORD_SEQ_LEN;
    }
    else if (direction < 0)
    {
        current_chord = (current_chord + CHORD_SEQ_LEN - 1) % CHORD_SEQ_LEN;
    }

    switch (current_chord)
    {
        case CHORD_I:
            UpdateChord(root_midi_note, MAJOR);
            break;
        
        case CHORD_V:
            UpdateChord((root_midi_note + 7), MAJOR);
            break;

        case CHORD_VI_M:
            UpdateChord((root_midi_note + 9), MINOR);
            break;

        case CHORD_IV:
            UpdateChord((root_midi_note + 5), MAJOR);
            break;

        default:
            break;
    }
}

int main(void)
{
    // Initialize pod hardware and oscillator daisysp module
    float sample_rate;

    pod.Init();
    sample_rate = pod.AudioSampleRate();

    InitChordNcos(s_key_root_note, (uint32_t)sample_rate);

    // start callback
    pod.StartAudio(AudioCallback);


    while(1)
        ;   // Infinite Loop
}
