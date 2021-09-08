#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

using namespace daisy;
using namespace daisysp;

#define MAX_WAVE Oscillator::WAVE_POLYBLEP_TRI

DaisyPatch patch;

struct lfoStruct
{
    Oscillator osc;
    Parameter  freqCtrl;
    Parameter  ampCtrl;
    float      amp;
    float      freq;
    int        waveform;
    float      value;

    void Init(float samplerate, AnalogControl freqKnob, AnalogControl ampKnob)
    {
        osc.Init(samplerate);
        osc.SetAmp(1);
        waveform = 0;
        freqCtrl.Init(freqKnob, .1, 35, Parameter::LOGARITHMIC);
        ampCtrl.Init(ampKnob, 0, 1, Parameter::LINEAR);
    }

    void Process(DacHandle::Channel chn)
    {
        //read the knobs and set params
        osc.SetFreq(freqCtrl.Process());
        osc.SetWaveform(waveform);

        //write to the DAC
        patch.seed.dac.WriteValue(
            chn,
            uint16_t((osc.Process() + 1.f) * .5f * ampCtrl.Process() * 4095.f));
    }
};

bool        menuSelect;
int         lfoSelect;
std::string waveNames[5];

lfoStruct lfos[2];

void UpdateOled();
void UpdateEncoder();

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        lfos[0].Process(DacHandle::Channel::ONE);
        lfos[1].Process(DacHandle::Channel::TWO);
    }
}

void SetupWaveNames()
{
    waveNames[0] = "sine";
    waveNames[1] = "triangle";
    waveNames[2] = "saw";
    waveNames[3] = "ramp";
    waveNames[4] = "square";
}

int main(void)
{
    patch.Init(); // Initialize hardware (daisy seed, and patch)
    float samplerate = patch.AudioSampleRate();

    //init the lfos
    lfos[0].Init(samplerate, patch.controls[0], patch.controls[1]);
    lfos[1].Init(samplerate, patch.controls[2], patch.controls[3]);

    lfoSelect = menuSelect = 0;

    SetupWaveNames();

    patch.StartAdc();
    patch.StartAudio(AudioCallback);

    while(1)
    {
        patch.DelayMs(1);
        UpdateOled();
        UpdateEncoder();
    }
}

void UpdateOled()
{
    patch.display.Fill(false);

    patch.display.SetCursor(0, 0);
    std::string str  = "Dual LFO";
    char*       cstr = &str[0];
    patch.display.WriteString(cstr, Font_7x10, true);

    //cursor
    patch.display.SetCursor(lfoSelect * 70, 25);
    char select = menuSelect ? '@' : 'o';
    patch.display.WriteChar(select, Font_7x10, true);

    //waveforms
    for(int i = 0; i < 2; i++)
    {
        cstr = &waveNames[lfos[i].waveform][0];
        patch.display.SetCursor(i * 70, 35);
        patch.display.WriteString(cstr, Font_7x10, true);
    }

    patch.display.Update();
}

void UpdateEncoder()
{
    patch.ProcessAnalogControls();
    patch.ProcessDigitalControls();

    //annoying menu stuff
    if(patch.encoder.RisingEdge())
    {
        menuSelect = !menuSelect;
    }

    if(menuSelect)
    {
        lfos[lfoSelect].waveform += patch.encoder.Increment();
        lfos[lfoSelect].waveform
            = (lfos[lfoSelect].waveform % MAX_WAVE + MAX_WAVE) % MAX_WAVE;
    }
    else
    {
        lfoSelect += patch.encoder.Increment();
        lfoSelect = (lfoSelect % 2 + 2) % 2;
    }
}