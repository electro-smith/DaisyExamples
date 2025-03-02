#include "daisysp.h"
#include "daisy_pod.h"
#include "nco.h"
#include "harmonicNco.h"

#define MIDI_A4     69
#define FREQ_A4     440

using namespace daisysp;
using namespace daisy;

enum NoteInterval
{
    ROOT,
    THIRD,
    FIFTH,
    NUM_INTERVALS
};

static void UpdateChord(uint16_t root_midi_note);

static DaisyPod     pod;
static HarmonicNCO  harmonicNco[NUM_INTERVALS];
static float        amp            = 0.5f;
static uint16_t     s_midi_note    = MIDI_A4;
static uint16_t     prev_midi_note = MIDI_A4;  

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

    pod.ProcessAllControls();

    s_midi_note += pod.encoder.Increment();
    s_midi_note = DSY_CLAMP(s_midi_note, 0, 127);

    if (s_midi_note != prev_midi_note) 
    {
        UpdateChord(s_midi_note);

        prev_midi_note = s_midi_note;
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
    UpdateChord(root_midi_note);
}

// Sets the NCO's to play a major chord
static void UpdateChord(uint16_t root_midi_note)
{
    float       freq        = 0;
    uint16_t    midi_note   = root_midi_note;
    
    for (uint8_t note = ROOT; note < NUM_INTERVALS; note++)
    {
        // Apply offset based on interval
        midi_note  = root_midi_note + (3 * note);
        midi_note += (note != ROOT) ? 1 : 0;
        
        freq = mtof(midi_note);
        harmonicNco[note].SetFrequency(freq);
    }
}

int main(void)
{
    // initialize pod hardware and oscillator daisysp module
    float sample_rate;

    pod.Init();
    sample_rate = pod.AudioSampleRate();

    InitChordNcos(MIDI_A4, (uint32_t)sample_rate);

    // start callback
    pod.StartAudio(AudioCallback);


    while(1)
        ;   // Infinite Loop
}
