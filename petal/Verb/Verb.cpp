#include "daisy_petal.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

// Declare a local daisy_petal for hardware access
DaisyPetal hw;

Parameter vtime, vfreq, vsend;
bool      bypass;
ReverbSc  verb;

// This runs at a fixed rate, to prepare audio samples
void callback(AudioHandle::InterleavingInputBuffer  in,
              AudioHandle::InterleavingOutputBuffer out,
              size_t                                size)
{
    float dryl, dryr, wetl, wetr, sendl, sendr;
    hw.ProcessDigitalControls();
    verb.SetFeedback(vtime.Process());
    verb.SetLpFreq(vfreq.Process());
    vsend.Process(); // Process Send to use later
    //bypass = hw.switches[DaisyPetal::SW_5].Pressed();
    if(hw.switches[DaisyPetal::SW_1].RisingEdge())
        bypass = !bypass;
    for(size_t i = 0; i < size; i += 2)
    {
        dryl  = in[i];
        dryr  = in[i + 1];
        sendl = dryl * vsend.Value();
        sendr = dryr * vsend.Value();
        verb.Process(sendl, sendr, &wetl, &wetr);
        if(bypass)
        {
            out[i]     = in[i];     // left
            out[i + 1] = in[i + 1]; // right
        }
        else
        {
            out[i]     = dryl + wetl;
            out[i + 1] = dryr + wetr;
        }
    }
}

int main(void)
{
    float samplerate;

    hw.Init();
    hw.SetAudioBlockSize(4);
    samplerate = hw.AudioSampleRate();

    vtime.Init(hw.knob[hw.KNOB_1], 0.6f, 0.999f, Parameter::LOGARITHMIC);
    vfreq.Init(hw.knob[hw.KNOB_2], 500.0f, 20000.0f, Parameter::LOGARITHMIC);
    vsend.Init(hw.knob[hw.KNOB_3], 0.0f, 1.0f, Parameter::LINEAR);
    verb.Init(samplerate);

    hw.StartAdc();
    hw.StartAudio(callback);
    while(1)
    {
        // Do Stuff InfInitely Here
        System::Delay(10);
        hw.ClearLeds();
        hw.SetFootswitchLed(hw.FOOTSWITCH_LED_1, bypass ? 0.0f : 1.0f);
        hw.UpdateLeds();
    }
}
