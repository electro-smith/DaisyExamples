#include "daisy_petal.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPetal hw;
Tremolo    treml, tremr;

bool effectOn;
int  waveform;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();

    treml.SetFreq(hw.knob[2].Process() * 20.f); //0 - 20 Hz
    tremr.SetFreq(hw.knob[2].Value() * 20.f);

    treml.SetDepth(hw.knob[3].Process());
    tremr.SetDepth(hw.knob[3].Value());

    waveform += hw.encoder.Increment();
    waveform = (waveform % 5 + 5) % 5;
    waveform *= !(hw.encoder.RisingEdge()); //reset wf to 0 if pressed
    treml.SetWaveform(waveform);
    tremr.SetWaveform(waveform);

    effectOn ^= hw.switches[0].RisingEdge(); //flip effectOn when you press fs

    for(size_t i = 0; i < size; i++)
    {
        out[0][i] = effectOn ? treml.Process(in[0][i]) : in[0][i];
        out[1][i] = effectOn ? tremr.Process(in[1][i]) : in[1][i];
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    treml.Init(sample_rate);
    tremr.Init(sample_rate);

    waveform = 0;
    effectOn = true;

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while(1)
    {
        hw.DelayMs(6);

        hw.ClearLeds();
        hw.SetFootswitchLed(hw.FOOTSWITCH_LED_1, (float)effectOn);
        for(int i = 0; i < 8; i++)
        {
            int tmp = waveform + 1;
            hw.SetRingLed(static_cast<DaisyPetal::RingLed>(i),
                          (float)(tmp & 1),
                          (float)(tmp & 2),
                          (float)(tmp & 4));
        }
        hw.UpdateLeds();
    }
}
