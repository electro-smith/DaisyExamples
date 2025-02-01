#include "daisysp.h"
#include "daisy_pod.h"
#include "nco.h"
#include "harmonicNco.h"

#define MIDI_A4         69
#define FREQ_A4         440

using namespace daisysp;
using namespace daisy;

static DaisyPod      pod;
//static NCO         nco;
static HarmonicNCO   harmonicNco;
static uint16_t      nco_kval;
static float         amp            = 0.5f;
static float         freq           = FREQ_A4;
static int           midi_note      = MIDI_A4;
static int           prev_midi_note = MIDI_A4;

/*
static float         harmonicAmps[] = {
   1.0,
   0.8,
   0.7,
   0.5,
   0.6,
   0.3,
   0.4,
   0.6
};
*/
static float         harmonicAmps[] = {
   1.0,
   0.8,
   0.7,
   0.0,
   0.0,
   0.0,
   0.0,
   0.0
};

static float         harmonicPhases[] = {
   0.0,
   0.9,
   0.125,
   0.45,
   0.65,
   0.95,
   0.55,
   0.6
};

static void AudioCallback(float *in, float *out, size_t size)
{
    float sig;

    pod.DebounceControls();

    midi_note += pod.encoder.Increment();
    midi_note = DSY_CLAMP(midi_note, 0, 127);

    if (midi_note != prev_midi_note) {
        freq = mtof(midi_note);
        harmonicNco.SetFrequency(freq);
    }
    prev_midi_note = midi_note;

    // Audio Loop
    for(size_t i = 0; i < size; i += 2)
    {
        sig = amp * harmonicNco.NextSample();
        // left out
        out[i] = sig;
        // right out
        out[i + 1] = sig;
    }
}

int main(void)
{
    // initialize pod hardware and oscillator daisysp module
    float sample_rate;

    pod.Init();
    sample_rate = pod.AudioSampleRate();
    harmonicNco.SetSampleRate((uint32_t)sample_rate);

    harmonicNco.SetFrequency(freq);
    harmonicNco.SetAmplitudes(harmonicAmps);
    harmonicNco.SetPhases(harmonicPhases);

    // start callback
    pod.StartAudio(AudioCallback);


    while(1)
        ;   // Infinite Loop
}
