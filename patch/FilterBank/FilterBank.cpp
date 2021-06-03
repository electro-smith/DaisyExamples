#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;
int        freqs[16];

int bank;

struct ConditionalUpdate
{
    float oldVal = 0;

    bool Process(float newVal)
    {
        if(abs(newVal - oldVal) > .04)
        {
            oldVal = newVal;
            return true;
        }

        return false;
    }
};

ConditionalUpdate condUpdates[4];

struct Filter
{
    Svf   filt;
    float amp;

    void Init(float samplerate, float freq)
    {
        filt.Init(samplerate);
        filt.SetRes(1);
        filt.SetDrive(.002);
        filt.SetFreq(freq);
        amp = .5f;
    }

    float Process(float in)
    {
        filt.Process(in);
        return filt.Peak() * amp;
    }
};

Filter filters[16];

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        float sig = 0.f;
        for(int j = 0; j < 16; j++)
        {
            sig += filters[j].Process(in[0][i]);
        }
        sig *= .06;

        out[0][i] = out[1][i] = out[2][i] = out[3][i] = sig;
    }
}

void InitFreqs()
{
    freqs[0]  = 50;
    freqs[1]  = 75;
    freqs[2]  = 110;
    freqs[3]  = 150;
    freqs[4]  = 220;
    freqs[5]  = 350;
    freqs[6]  = 500;
    freqs[7]  = 750;
    freqs[8]  = 1100;
    freqs[9]  = 1600;
    freqs[10] = 2200;
    freqs[11] = 3600;
    freqs[12] = 5200;
    freqs[13] = 7500;
    freqs[14] = 11000;
    freqs[15] = 15000;
}

void InitFilters(float samplerate)
{
    for(int i = 0; i < 16; i++)
    {
        filters[i].Init(samplerate, freqs[i]);
    }
}

void UpdateOled();
void UpdateControls();

int main(void)
{
    float samplerate;
    patch.Init(); // Initialize hardware (daisy seed, and patch)
    samplerate = patch.AudioSampleRate();

    InitFreqs();
    InitFilters(samplerate);
    bank = 0;

    patch.StartAdc();
    patch.StartAudio(AudioCallback);
    while(1)
    {
        UpdateOled();
        UpdateControls();
    }
}

void UpdateOled()
{
    patch.display.Fill(false);

    std::string str  = "Filter Bank";
    char*       cstr = &str[0];
    patch.display.SetCursor(0, 0);
    patch.display.WriteString(cstr, Font_7x10, true);

    str = "";
    for(int i = 0; i < 2; i++)
    {
        str += std::to_string(freqs[i + 4 * bank]);
        str += "  ";
    }

    patch.display.SetCursor(0, 25);
    patch.display.WriteString(cstr, Font_7x10, true);

    str = "";
    for(int i = 2; i < 4; i++)
    {
        str += std::to_string(freqs[i + 4 * bank]);
        str += "  ";
    }

    patch.display.SetCursor(0, 35);
    patch.display.WriteString(cstr, Font_7x10, true);


    patch.display.Update();
}

void UpdateControls()
{
    patch.ProcessAnalogControls();
    patch.ProcessDigitalControls();

    //encoder
    bank += patch.encoder.Increment();
    bank = (bank % 4 + 4) % 4;

    bank = patch.encoder.RisingEdge() ? 0 : bank;

    //controls
    for(int i = 0; i < 4; i++)
    {
        float val = patch.controls[i].Process();
        if(condUpdates[i].Process(val))
        {
            filters[i + bank * 4].amp = val;
        }
    }
}
