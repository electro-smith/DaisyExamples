#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;
Compressor comp;

bool isSideChained;

void UpdateControls();

Parameter threshParam, ratioParam, attackParam, releaseParam;

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    float sig;
    float dry_in, dry_sidechain;
    UpdateControls();

    // Scales input by 2 and then the output by 0.5
    // This is because there are 6dB of headroom on the daisy
    // and currently no way to tell where '0dB' is supposed to be
    for(size_t i = 0; i < size; i++)
    {
        dry_in        = in[0][i] * 2.0f;
        dry_sidechain = in[1][i] * 2.0f;

        sig = isSideChained ? comp.Process(dry_in, dry_sidechain)
                            : comp.Process(dry_in);

        // Writes output to all four outputs.
        for(size_t chn = 0; chn < 4; chn++)
        {
            out[chn][i] = sig * 0.5f;
        }
    }
}

int main(void)
{
    float samplerate;
    patch.Init(); // Initialize hardware
    samplerate = patch.AudioSampleRate();

    comp.Init(samplerate);

    isSideChained = false;

    //parameter parameters
    threshParam.Init(patch.controls[0], -80.0f, 0.f, Parameter::LINEAR);
    ratioParam.Init(patch.controls[1], 1.2f, 40.f, Parameter::LINEAR);
    attackParam.Init(patch.controls[2], 0.01f, 1.f, Parameter::EXPONENTIAL);
    releaseParam.Init(patch.controls[3], 0.01f, 1.f, Parameter::EXPONENTIAL);

    patch.StartAdc();
    patch.StartAudio(AudioCallback);
    while(1)
    {
        //update the oled
        patch.display.Fill(false);

        patch.display.SetCursor(0, 0);
        std::string str  = "Compressor";
        char*       cstr = &str[0];
        patch.display.WriteString(cstr, Font_7x10, true);

        patch.display.SetCursor(0, 25);
        str = "Sidechain: ";
        str += isSideChained ? "On" : "Off";
        patch.display.WriteString(cstr, Font_7x10, true);

        patch.display.Update();
    }
}

void UpdateControls()
{
    patch.ProcessAnalogControls();
    patch.ProcessDigitalControls();

    //encoder click
    isSideChained = patch.encoder.RisingEdge() ? !isSideChained : isSideChained;

    //controls
    comp.SetThreshold(threshParam.Process());
    comp.SetRatio(ratioParam.Process());
    comp.SetAttack(attackParam.Process());
    comp.SetRelease(releaseParam.Process());
}
