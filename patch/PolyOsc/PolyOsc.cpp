#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyPatch hw;

Oscillator osc[3];

std::string waveNames[5];

int waveform;
int final_wave;

float testval;

void UpdateControls();

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    UpdateControls();
    for(size_t i = 0; i < size; i++)
    {
        float mix = 0;
        //Process and output the three oscillators
        for(size_t chn = 0; chn < 3; chn++)
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
    for(int i = 0; i < 3; i++)
    {
        osc[i].Init(samplerate);
        osc[i].SetAmp(.7);
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
    hw.Init(); // Initialize hardware (daisy seed, and patch)
    samplerate = hw.AudioSampleRate();

    waveform   = 0;
    final_wave = Oscillator::WAVE_POLYBLEP_TRI;

    SetupOsc(samplerate);
    SetupWaveNames();

    testval = 0.f;

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while(1)
    {
        UpdateOled();
    }
}

void UpdateOled()
{
    hw.display.Fill(false);

    hw.display.SetCursor(0, 0);
    std::string str  = "PolyOsc";
    char*       cstr = &str[0];
    hw.display.WriteString(cstr, Font_7x10, true);

    str = "waveform:";
    hw.display.SetCursor(0, 30);
    hw.display.WriteString(cstr, Font_7x10, true);

    hw.display.SetCursor(70, 30);
    cstr = &waveNames[waveform][0];
    hw.display.WriteString(cstr, Font_7x10, true);

    hw.display.Update();
}

void UpdateControls()
{
    hw.ProcessDigitalControls();
    hw.ProcessAnalogControls();

    //knobs
    float ctrl[4];
    for(int i = 0; i < 4; i++)
    {
        ctrl[i] = hw.GetKnobValue((DaisyPatch::Ctrl)i);
    }

    for(int i = 0; i < 3; i++)
    {
        ctrl[i] += ctrl[3];
        ctrl[i] = ctrl[i] * 5.f;           //voltage
        ctrl[i] = powf(2.f, ctrl[i]) * 55; //Hz
    }

    testval = hw.GetKnobValue((DaisyPatch::Ctrl)2) * 5.f;

    //encoder
    waveform += hw.encoder.Increment();
    waveform = (waveform % final_wave + final_wave) % final_wave;

    //Adjust oscillators based on inputs
    for(int i = 0; i < 3; i++)
    {
        osc[i].SetFreq(ctrl[i]);
        osc[i].SetWaveform((uint8_t)waveform);
    }
}
