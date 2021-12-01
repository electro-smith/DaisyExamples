#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;

Phaser     phaser;
Oscillator osc;

Metro tick;

bool bass_note; // Toggles between root/fifth

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        if(tick.Process())
        {
            bass_note = !bass_note;
            osc.SetFreq(mtof(bass_note ? 31.f : 24.f));
        }

        out[0][i] = out[1][i] = phaser.Process(osc.Process() * .6f);
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    phaser.Init(sample_rate);
    phaser.SetFreq(500.f);
    phaser.SetLfoDepth(1.f);

    tick.Init(1.f, sample_rate);

    bass_note = false;
    osc.Init(sample_rate);
    osc.SetFreq(mtof(bass_note ? 31.f : 24.f));
    osc.SetWaveform(Oscillator::WAVE_SAW);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
