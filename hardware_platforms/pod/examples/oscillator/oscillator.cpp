#include "daisy_pod.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPod   hw;
Oscillator osc;
parameter  p_freq;

static float   freq;
float          sig, waveform;
static int32_t inc, octave = 2;

static void audioCallback(float *in, float *out, size_t size)
{
    hw.encoder.Debounce();
    inc = hw.encoder.Increment();
    if(inc > 0)
    {
        octave *= 2;
        if(octave >= 10)
            octave = 10;
    }

    if(inc < 0)
    {
        octave *= .5;
        if(octave <= 2)
            octave = 2;
    }
    
    // Get Parameters
    freq = p_freq.process();

    osc.SetFreq(freq*octave);

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
    p_freq.init(hw.knob1, 10.0f, 12000.0f, parameter::LOG);

    // Init Osc
    osc.Init(samplerate);
    osc.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
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
