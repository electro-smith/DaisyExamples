#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisysp;
using namespace daisy;

static DaisySeed  hw;
static Oscillator osc, lfo;
static Allpass    allpass;

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float sig;
    for(size_t i = 0; i < size; i += 2)
    {
        sig     = osc.Process();
        float l = .01 + lfo.Process();
        allpass.SetFreq(l);
        sig = allpass.Process(sig);

        // left out
        out[i] = sig;

        // right out
        out[i + 1] = sig;
    }
}

int main(void)
{
    // initialize seed hardware and oscillator daisysp module
    float sample_rate;
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    sample_rate = hw.AudioSampleRate();
    osc.Init(sample_rate);
    lfo.Init(sample_rate);

    float buff[9600];
    for(int i = 0; i < 9600; i++)
    {
        buff[i] = 0.0f;
    }

    allpass.Init(sample_rate, buff, (int)9600);

    // Set parameters for oscillator
    osc.SetWaveform(osc.WAVE_SIN);
    osc.SetFreq(440);
    osc.SetAmp(0.5);

    // Set parameters for oscillator
    lfo.SetWaveform(osc.WAVE_SIN);
    lfo.SetFreq(1);
    lfo.SetAmp(0.01);


    // start callback
    hw.StartAudio(AudioCallback);


    while(1) {}
}
