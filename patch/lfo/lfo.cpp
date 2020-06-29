#include "daisysp.h"
#include "daisy_patch.h"

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
};

bool menuSelect;
int lfoSelect;

lfoStruct lfos[2];

void InitLfos()
{
    for (int i = 0; i < 2; i++)
    {
        lfos[i].osc.Init(1000);
        lfos[i].waveform = 0;
        lfos[i].freqCtrl.Init(patch.controls[i * 2], .5, 10, Parameter::LOGARITHMIC);
        lfos[i].ampCtrl.Init(patch.controls[i * 2 + 1], 0, 1, Parameter::LINEAR);
    }
}

void UpdateOled();
void UpdateControls();
void UpdateOutputs();

int main(void)
{
    patch.Init(); // Initialize hardware (daisy seed, and patch)

    InitLfos();

    lfoSelect = menuSelect = 0;

    patch.StartAdc();
    while(1) 
    {
        patch.DelayMs(1);
        UpdateOled();
        UpdateControls();
        UpdateOutputs();
    } 
}

void UpdateOled()
{
    patch.display.Fill(true);
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

void UpdateOutputs()
{
 	dsy_dac_write(DSY_DAC_CHN1, (lfos[0].osc.Process() + 1)/2 * 4096);
	dsy_dac_write(DSY_DAC_CHN2, (lfos[1].osc.Process() + 1)/2 * 4096);
}