#include "daisy_field.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyField hw;
Chorus<4>  ch; //2 delay lines

bool effectOn;

void Controls()
{
    hw.ProcessAllControls();

    //knobs
    float spread = hw.knob[0].Process();

    ch.SetPan(.5f - .25f * spread, 0);
    ch.SetPan(.4f - .4f * spread, 1);
    ch.SetPan(.5f + .25f * spread, 2);
    ch.SetPan(.6f + .4f * spread, 3);

    ch.SetLfoFreqAll(hw.knob[1].Process());
    ch.SetLfoDepthAll(hw.knob[2].Process());

    for(int i = 0; i < 4; i++)
    {
        ch.SetDelay(hw.knob[3 + i].Process(), i);
    }

    //activate / deactivate effect
    effectOn ^= hw.sw[0].RisingEdge();
}

void AudioCallback(float **in, float **out, size_t size)
{
    Controls();

    for(size_t i = 0; i < size; i++)
    {
        out[0][i] = in[0][i];
        out[1][i] = in[1][i];

        if(effectOn)
        {
            ch.Process(in[0][i], out[0][i], out[1][i]);
        }
    }
}

int main(void)
{
    hw.Init();
    float sample_rate = hw.AudioSampleRate();

    ch.Init(sample_rate);

    effectOn = true;

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while(1) {
		hw.display.Fill(false);
		
		char cstr[15];
		sprintf(cstr, "Effect: %s", effectOn ? "On" : "Off");
		hw.display.SetCursor(0,0);
		hw.display.WriteString(cstr, Font_7x10, true);
		
		hw.display.Update();
	}
}
