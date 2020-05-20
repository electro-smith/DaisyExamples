#include "daisy_pod.h"
#include "daisysp.h"

#define NUM_WAVEFORMS 4

using namespace daisy;
using namespace daisysp;

DaisyPod   hw;
Oscillator osc;
Parameter  p_freq;

uint8_t waveforms[NUM_WAVEFORMS] = {
    Oscillator::WAVE_SIN,
    Oscillator::WAVE_TRI,
    Oscillator::WAVE_POLYBLEP_SAW,
    Oscillator::WAVE_POLYBLEP_SQUARE,
};
uint8_t octave_factor[5] = {
    1,
    2,
    4,
    8,
    16,
};

static float   freq;
float          sig;
static int32_t waveform = 0, octave = 0;

static void AudioCallback(float *in, float *out, size_t size)
{
    hw.DebounceControls();

    // read state from encoder and increment waveform accordingly
    waveform += hw.encoder.Increment();

    // clamp if waveform exceeds limits
    if(waveform >= NUM_WAVEFORMS)
        waveform = NUM_WAVEFORMS - 1;
    else if(waveform <= 0)
        waveform = 0;

    osc.SetWaveform(waveforms[waveform]);

    // if button2 has been pressed, octave up
    if(hw.button2.RisingEdge())
    {
        octave++;
        if(octave > 4)
            octave = 4;
    }

    // if button1 has been pressed, octave down
    if(hw.button1.RisingEdge())
    {
        octave--;
        if(octave < 0)
            octave = 0;
    }

    // assign freq value from pot
    freq = p_freq.Process();
    osc.SetFreq(freq * octave_factor[octave]);

    // Audio Loop
    for(size_t i = 0; i < size; i += 2)
    {
        // Process
        sig        = osc.Process();
        out[i]     = sig;
        out[i + 1] = sig;
    }
}

void InitSynth(float samplerate)
{
    // init freq parameter to knob1, min 10, max 12k, curve log
    p_freq.Init(hw.knob1, 10.0f, 12000.0f, Parameter::LOGARITHMIC);

    // init Osc
    osc.Init(samplerate);
    osc.SetAmp(0.5f);
}

int main(void)
{
    float samplerate;

    // init everything
    hw.Init();
    samplerate = hw.AudioSampleRate();
    InitSynth(samplerate);

    // start callbacks
    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    while(1) {}
}
