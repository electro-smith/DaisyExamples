#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;

WhiteNoise noise;

struct filter
{
    Svf filt;

    enum mode
    {
        LOW,
        HIGH,
        BAND,
        NOTCH,
        PEAK,
        LAST,
    };
    mode filterMode;

    Parameter resParam, cutoffParam;

    void Init(float         samplerate,
              mode          setMode,
              AnalogControl cutKnob,
              AnalogControl resKnob)
    {
        filt.Init(samplerate);
        filterMode = setMode;

        cutoffParam.Init(cutKnob, 20, 10000, Parameter::LOGARITHMIC);
        resParam.Init(resKnob, 0, 1, Parameter::LINEAR);
    }

    void UpdateControls()
    {
        filt.SetFreq(cutoffParam.Process());
        filt.SetRes(resParam.Process());
    }

    float Process(float in)
    {
        filt.Process(in);
        switch(filterMode)
        {
            case mode::LOW: return filt.Low();
            case mode::HIGH: return filt.High();
            case mode::BAND: return filt.Band();
            case mode::NOTCH: return filt.Notch();
            case mode::PEAK: return filt.Peak();
            default: break;
        }
        return 0.f;
    }
};

filter lowpass, highpass;

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    patch.ProcessAnalogControls();

    lowpass.UpdateControls();
    highpass.UpdateControls();

    for(size_t i = 0; i < size; i++)
    {
        float sig = noise.Process();
        sig       = lowpass.Process(sig);
        sig       = highpass.Process(sig);

        for(size_t chn = 0; chn < 4; chn++)
        {
            out[chn][i] = sig;
        }
    }
}

void UpdateControls();
void UpdateOled();

int main(void)
{
    float samplerate;
    patch.Init(); // Initialize hardware (daisy seed, and patch)
    samplerate = patch.AudioSampleRate();

    noise.Init();
    lowpass.Init(
        samplerate, filter::mode::LOW, patch.controls[0], patch.controls[1]);
    highpass.Init(
        samplerate, filter::mode::HIGH, patch.controls[2], patch.controls[3]);

    std::string str  = "Noise";
    char*       cstr = &str[0];
    patch.display.WriteString(cstr, Font_7x10, true);
    patch.display.Update();
    patch.DelayMs(1000);

    patch.StartAdc();
    patch.StartAudio(AudioCallback);
    while(1)
    {
        patch.DisplayControls(false);
    }
}
