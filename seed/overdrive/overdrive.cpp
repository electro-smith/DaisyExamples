#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed  hw;
Overdrive  drive;
Oscillator osc, lfo;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        drive.SetDrive(fabsf(lfo.Process()));
        float sig = drive.Process(osc.Process());
        out[0][i] = out[1][i] = sig;
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    osc.Init(sample_rate);
    lfo.Init(sample_rate);
    lfo.SetAmp(.8f);
    lfo.SetWaveform(Oscillator::WAVE_TRI);
    lfo.SetFreq(.25f);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
