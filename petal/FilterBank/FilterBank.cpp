#include "daisysp.h"
#include "daisy_petal.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyPetal petal;
int        freqs[8];

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

Filter filters[8];
bool   passthru;
void   UpdateControls();

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    UpdateControls();


    for(size_t i = 0; i < size; i++)
    {
        float sig = 0.f;
        for(int j = 0; j < 8; j++)
        {
            sig += filters[j].Process(in[0][i]);
        }
        sig *= .06;

        if(!passthru)
        {
            out[0][i] = out[1][i] = sig;
        }
        else
        {
            out[0][i] = in[0][i];
            out[1][i] = in[1][i];
        }
    }
}

void InitFreqs()
{
    freqs[0] = 350;
    freqs[1] = 500;
    freqs[2] = 750;
    freqs[3] = 1100;
    freqs[4] = 2200;
    freqs[5] = 3600;
    freqs[6] = 5200;
    freqs[7] = 7500;
}

void InitFilters(float samplerate)
{
    for(int i = 0; i < 8; i++)
    {
        filters[i].Init(samplerate, freqs[i]);
    }
}


void UpdateLeds();

int main(void)
{
    float samplerate;
    petal.Init();
    petal.SetAudioBlockSize(4); // Initialize hardware (daisy seed, and petal)
    samplerate = petal.AudioSampleRate();

    InitFreqs();
    InitFilters(samplerate);
    bank     = 0;
    passthru = false;

    petal.StartAdc();
    petal.StartAudio(AudioCallback);
    while(1)
    {
        UpdateLeds();
        System::Delay(6);
    }
}

void UpdateControls()
{
    petal.ProcessAnalogControls();
    petal.ProcessDigitalControls();

    //encoder
    bank += petal.encoder.Increment();
    bank = (bank % 2 + 2) % 2;

    bank = petal.encoder.RisingEdge() ? 0 : bank;

    // Toggle Pass thru
    if(petal.switches[petal.SW_1].RisingEdge())
        passthru = !passthru;

    //controls
    for(int i = 0; i < 4; i++)
    {
        float val = petal.knob[i + 2].Process();
        if(condUpdates[i].Process(val))
        {
            filters[i + bank * 4].amp = val;
        }
    }
}

void UpdateLeds()
{
    for(int i = 0; i < 4; i++)
    {
        petal.SetRingLed((DaisyPetal::RingLed)i,
                         filters[i].amp,
                         (bank == 0) * filters[i].amp,
                         filters[i].amp);
    }
    for(int i = 4; i < 8; i++)
    {
        petal.SetRingLed((DaisyPetal::RingLed)i,
                         filters[i].amp,
                         (bank == 1) * filters[i].amp,
                         filters[i].amp);
    }

    petal.SetFootswitchLed(DaisyPetal::FOOTSWITCH_LED_1, !passthru);

    petal.UpdateLeds();
}
