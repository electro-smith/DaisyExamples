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
    Parameter freqCtrl;
    Parameter ampCtrl;
    float amp;
    float freq;
    int waveform;
    float value;
};

bool menuSelect;
int lfoSelect;
std::string waveNames[5];

lfoStruct lfos[2];

void UpdateOled();
void UpdateControls();

static void AudioCallback(float **in, float **out, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        dsy_dac_write(DSY_DAC_CHN1, (lfos[0].osc.Process() + 1) / 2 * 4095);
        dsy_dac_write(DSY_DAC_CHN2, (lfos[1].osc.Process() + 1) / 2 * 4095);
    }
}

void InitLfos(float samplerate)
{
    for (int i = 0; i < 2; i++)
    {
        lfos[i].osc.Init(samplerate);
        lfos[i].waveform = 0;
        lfos[i].freqCtrl.Init(patch.controls[i * 2], .1, 35, Parameter::LOGARITHMIC);
        lfos[i].ampCtrl.Init(patch.controls[i * 2 + 1], 0, 1, Parameter::LINEAR);
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

    InitLfos(patch.AudioSampleRate());

    lfoSelect = menuSelect = 0;

    SetupWaveNames();

    patch.StartAdc();
    patch.StartAudio(AudioCallback);
    
    while(1) 
    {
        patch.DelayMs(1);
        UpdateOled();
        UpdateControls();
    } 
}

void UpdateOled()
{
    patch.display.Fill(false);

    patch.display.SetCursor(0,0);
    std::string str = "Dual LFO";
    char* cstr = &str[0];
    patch.display.WriteString(cstr, Font_7x10, true);

    //cursor
    patch.display.SetCursor(lfoSelect * 70,25);
    char select = menuSelect ? '@' : 'o';
    patch.display.WriteChar(select,Font_7x10, true);

    //waveforms
    for (int i = 0; i < 2; i++){
        cstr = &waveNames[lfos[i].waveform][0];
        patch.display.SetCursor(i * 70, 35);
        patch.display.WriteString(cstr, Font_7x10, true);
    }

    patch.display.Update();
}

void UpdateEncoder()
{
    //annoying menu stuff
    if (patch.encoder.RisingEdge())
    {
        menuSelect = !menuSelect;
    }

    if (menuSelect)
    {
        lfos[lfoSelect].waveform  += patch.encoder.Increment();
        lfos[lfoSelect].waveform  = (lfos[lfoSelect].waveform % MAX_WAVE + MAX_WAVE) % MAX_WAVE;
    }
    else
    {
        lfoSelect  += patch.encoder.Increment();
        lfoSelect  = (lfoSelect % 2 + 2) % 2;
    }
}

void UpdateControls()
{
    patch.UpdateAnalogControls();
    patch.DebounceControls();

    UpdateEncoder();

    //knobs and update waveform
    for (int i = 0; i < 2; i++)
    {
        lfos[i].osc.SetFreq(lfos[i].freqCtrl.Process());
        lfos[i].osc.SetAmp(lfos[i].ampCtrl.Process());        
        lfos[i].osc.SetWaveform(lfos[i].waveform);   //waveform
    }
}