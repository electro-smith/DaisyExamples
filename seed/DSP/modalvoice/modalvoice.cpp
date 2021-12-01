#include "daisy_seed.h"
#include "daisysp.h"
#include <stdlib.h>

using namespace daisy;
using namespace daisysp;

DaisySeed  hw;
ModalVoice modal;
Metro      tick;
Oscillator lfo;

// A minor pentatonic
float freqs[5] = {440.f, 523.25f, 587.33f, 659.25f, 783.99f};
bool  sus      = false;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        bool t = tick.Process();
        if(t)
        {
            modal.SetSustain(sus = !sus);
        }

        if(t || modal.GetAux() > .1f)
        {
            modal.SetFreq(freqs[rand() % 5]);
        }

        float sig = fabsf(lfo.Process());
        modal.SetStructure(sig);
        modal.SetBrightness(.1f + sig * .1f);

        out[0][i] = out[1][i] = modal.Process(t);
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    modal.Init(sample_rate);
    modal.SetDamping(.5);


    tick.Init(2.f, sample_rate);

    lfo.Init(sample_rate);
    lfo.SetFreq(.005f);
    lfo.SetAmp(1.f);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
