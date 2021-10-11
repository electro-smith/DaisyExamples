#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;

Chorus                chorus;
VariableSawOscillator osc;
HiHat<>               hat;
AnalogBassDrum        kick;


Metro tick;

uint8_t h_patt = 0B11111111;
uint8_t k_patt = 0B10001000;
uint8_t pos    = 0B10000000;

bool bass_note; // Toggles between root/fifth

float ProcessDrums()
{
    bool t = tick.Process();
    pos    = pos >> t;

    if(pos == 0 && t)
    {
        pos       = 0B10000000;
        bass_note = !bass_note;
        osc.SetFreq(mtof(bass_note ? 31.f : 24.f));
    }

    float sig = (hat.Process(t && pos & (h_patt)) * .5f);
    sig += (kick.Process(t && pos & (k_patt)) * 12.4f);
    sig += (osc.Process() * 0.05f);

    return sig * 8.f;
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        chorus.Process(ProcessDrums());
        out[0][i] = chorus.GetLeft();
        out[1][i] = chorus.GetRight();
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    chorus.Init(sample_rate);
    chorus.SetLfoFreq(.33f, .2f);
    chorus.SetLfoDepth(1.f, 1.f);
    chorus.SetDelay(.75f, .9f);

    tick.Init(8.f, sample_rate);

    hat.Init(sample_rate);
    hat.SetDecay(.6f);

    kick.Init(sample_rate);

    bass_note = false;
    osc.Init(sample_rate);
    osc.SetPW(0.25f);
    osc.SetWaveshape(0.11f);
    osc.SetFreq(mtof(bass_note ? 31.f : 24.f));

    hw.StartAudio(AudioCallback);
    while(1) {}
}
