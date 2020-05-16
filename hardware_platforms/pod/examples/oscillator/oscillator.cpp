#include "daisy_pod.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPod   hw;
Oscillator osc;
parameter  p_freq;

static float freq;
float        sig, waveform;

static void  audioCallback(float *in, float *out, size_t size)
{
    // Get Parameters
    freq = p_freq.process();

    osc.SetFreq(freq);

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
