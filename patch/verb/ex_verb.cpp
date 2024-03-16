#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

using namespace daisy;
using namespace daisysp;
static DaisyPatch hw;
static ReverbSc   verb;
static DcBlock    blk[2];
Parameter         lpParam;
static float      drylevel, send;

static void VerbCallback(AudioHandle::InputBuffer  in,
                         AudioHandle::OutputBuffer out,
                         size_t                    size)
{
    float dryL, dryR, wetL, wetR, sendL, sendR;
    hw.ProcessAnalogControls();
    for(size_t i = 0; i < size; i++)
    {
        // read some controls
        drylevel = hw.GetKnobValue(hw.CTRL_1);
        send     = hw.GetKnobValue(hw.CTRL_2);
        verb.SetFeedback(hw.GetKnobValue(hw.CTRL_3) * .94f);
        verb.SetLpFreq(lpParam.Process());

        // Read Inputs (only stereo in are used)
        dryL = in[0][i];
        dryR = in[1][i];

        // Send Signal to Reverb
        sendL = dryL * send;
        sendR = dryR * send;
        verb.Process(sendL, sendR, &wetL, &wetR);

        // Dc Block
        wetL = blk[0].Process(wetL);
        wetR = blk[1].Process(wetR);

        // Out 1 and 2 are Mixed
        out[0][i] = (dryL * drylevel) + wetL;
        out[1][i] = (dryR * drylevel) + wetR;

        // Out 3 and 4 are just wet
        out[2][i] = wetL;
        out[3][i] = wetR;
    }
}

void UpdateOled();

int main(void)
{
    float samplerate;
    hw.Init();
    samplerate = hw.AudioSampleRate();

    verb.Init(samplerate);
    verb.SetFeedback(0.85f);
    verb.SetLpFreq(18000.0f);

    blk[0].Init(samplerate);
    blk[1].Init(samplerate);

    lpParam.Init(hw.controls[3], 20, 20000, Parameter::LOGARITHMIC);

    //briefly display the module name
    std::string str  = "Stereo Reverb";
    char*       cstr = &str[0];
    hw.display.WriteString(cstr, Font_7x10, true);
    hw.display.Update();
    hw.DelayMs(1000);

    hw.StartAdc();
    hw.StartAudio(VerbCallback);

    while(1)
    {
        UpdateOled();
    }
}

void UpdateOled()
{
    hw.DisplayControls(false);
}
