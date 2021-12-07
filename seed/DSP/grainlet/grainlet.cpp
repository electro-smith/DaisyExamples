#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed          hw;
GrainletOscillator grainlet, subgrainlet;
Oscillator         lfo;
Metro              tick;

float notes[]
    = {329.63f, 392.f, 440.f, 392.f, 587.33f, 523.25f, 587.33f, 659.25};
int idx = 0;

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    for(size_t i = 0; i < size; i += 2)
    {
        if(tick.Process())
        {
            grainlet.SetFreq(notes[idx] * .5f);
            subgrainlet.SetFreq(notes[idx] * .25f);
            idx = (idx + 1) % 8;
        }

        float sig = fabsf(lfo.Process());
        grainlet.SetShape(sig * .5f);
        grainlet.SetFormantFreq(2000.f - 2000.f * sig);
        subgrainlet.SetShape(.5f - sig * .5f);
        subgrainlet.SetFormantFreq(2000.f * sig);

        out[i]     = grainlet.Process();
        out[i + 1] = subgrainlet.Process();
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    grainlet.Init(sample_rate);
    grainlet.SetBleed(.5f);

    subgrainlet.Init(sample_rate);
    subgrainlet.SetBleed(1.f);

    grainlet.SetFreq(notes[idx] * .5f);
    subgrainlet.SetFreq(notes[idx] * .25f);

    lfo.Init(sample_rate);
    lfo.SetAmp(1.f);
    lfo.SetFreq(.125f);

    tick.Init(11.f, sample_rate);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
