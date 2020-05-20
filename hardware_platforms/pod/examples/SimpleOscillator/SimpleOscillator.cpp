#include "daisy_pod.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPod   hw;
Oscillator osc;
Parameter  p_freq;

uint8_t waveforms[4] = {
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
static int32_t inc, waveform = 0, octave = 0;

static void audioCallback(float *in, float *out, size_t size)
{
    hw.DebounceControls();
    inc = hw.encoder.Increment();
    if(inc > 0)
    {
        waveform++;
        if(waveform > 3)
            waveform = 3;
    }
    if(inc < 0)
    {
        waveform--;
        if(waveform < 0)
            waveform = 0;
    }
    osc.SetWaveform(waveforms[waveform]);

    if(hw.button2.RisingEdge())
    {
        octave++;
        if(octave > 4)
            octave = 4;
    }
    if(hw.button1.RisingEdge())
    {
        octave--;
        if(octave < 0)
            octave = 0;
    }
    freq = p_freq.Process();
    osc.SetFreq(freq*octave_factor[octave]);

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
    // Parameters
    p_freq.Init(hw.knob1, 10.0f, 12000.0f, Parameter::LOGARITHMIC);

    // Init Osc
    osc.Init(samplerate);
    osc.SetWaveform(Oscillator::WAVE_SIN);
    osc.SetFreq(100.0f);
    osc.SetAmp(0.5f);
}

int main(void)
{
    float samplerate;
    // Init
    hw.Init();
    samplerate = hw.AudioSampleRate();
    InitSynth(samplerate);
    // Start Callbacks
    hw.StartAdc();
    hw.StartAudio(audioCallback);
    while(1) {}
}
