// pitch, pitch, pitch, master pitch
// individual waveforms, hold to set global
// out out out, mix out

#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;

Oscillator osc[3];
Parameter freqParam[4];

std::string waveNames[5];

int waveform;
int final_wave;

void UpdateControls();

static void AudioCallback(float **in, float **out, size_t size)
{
    UpdateControls();    
    for (size_t i = 0; i < size; i ++)
    {
	float mix = 0;
	//Process and output the three oscillators
        for (size_t chn = 0; chn < 3; chn++)
        {
            float sig = osc[chn].Process();	    
	    mix += sig * .25f;
	    out[chn][i] = sig;
	}
	
	//output the mixed oscillators
	out[3][i] = mix;
    }
}

void SetupOsc(float samplerate)
{
    for (int i = 0; i < 3; i++)
    {
	osc[i].Init(samplerate);
	osc[i].SetAmp(1);
    }
}

void SetupParams()
{
    for (int i = 0; i < 4; i++)
    {
	freqParam[i].Init(patch.controls[i], 0, 63, Parameter::LINEAR);
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

void UpdateOled();

int main(void)
{
    float samplerate;
    patch.Init(); // Initialize hardware (daisy seed, and patch)
    samplerate = patch.AudioSampleRate();

    waveform = 0;
    final_wave = Oscillator::WAVE_POLYBLEP_TRI;

    SetupOsc(samplerate);
    SetupParams();
    SetupWaveNames();
    
    patch.StartAdc();
    patch.StartAudio(AudioCallback);
    while(1)
    {
	UpdateOled();
    } 
}

void UpdateOled()
{
    patch.display.Fill(false);

    patch.display.SetCursor(0,0);
    patch.display.WriteString("PolyOsc", Font_7x10, true);
    
    patch.display.SetCursor(0,30);
    patch.display.WriteString("waveform:", Font_7x10, true);

    patch.display.SetCursor(70,30);
    char* cstr = &waveNames[waveform][0];
    patch.display.WriteString(cstr, Font_7x10, true);

    patch.display.Update();
}

void UpdateControls()
{
    patch.DebounceControls();
    patch.UpdateAnalogControls();
    
    //knobs
    int ctrl[4];
    for (int i = 0; i < 4; i++)
    {
	ctrl[i] = round(freqParam[i].Process());
    }

    //encoder
    waveform += patch.encoder.Increment();
    waveform = (waveform % final_wave + final_wave) % final_wave;
    
    //Adjust oscillators based on inputs
    for (int i = 0; i < 3; i++)
    {
	osc[i].SetFreq(mtof(ctrl[i] + ctrl[3]));
	osc[i].SetWaveform((uint8_t)waveform);
    }
}
