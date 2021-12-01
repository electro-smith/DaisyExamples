#include "daisysp.h"
#include "daisy_petal.h"
#include <string>

#define MAX_DELAY static_cast<size_t>(48000 * 1.f)

using namespace daisy;
using namespace daisysp;

DaisyPetal petal;

DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delMems[3];

struct delay
{
    DelayLine<float, MAX_DELAY> *del;
    float                        currentDelay;
    float                        delayTarget;
    float                        feedback;

    float Process(float in)
    {
        //set delay times
        fonepole(currentDelay, delayTarget, .0002f);
        del->SetDelay(currentDelay);

        float read = del->Read();
        del->Write((feedback * read) + in);

        return read;
    }
};

delay     delays[3];
Parameter params[3];

float feedback;
int   drywet;
bool  passThruOn;

void ProcessControls();

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    ProcessControls();

    for(size_t i = 0; i < size; i++)
    {
        float mix     = 0;
        float fdrywet = passThruOn ? 0.f : (float)drywet / 100.f;

        //update delayline with feedback
        for(int d = 0; d < 3; d++)
        {
            float sig = delays[d].Process(in[0][i]);
            mix += sig;
        }

        //apply drywet and attenuate
        mix       = fdrywet * mix * .3f + (1.0f - fdrywet) * in[0][i];
        out[0][i] = out[1][i] = mix;
    }
}

void InitDelays(float samplerate)
{
    for(int i = 0; i < 3; i++)
    {
        //Init delays
        delMems[i].Init();
        delays[i].del = &delMems[i];
        //3 delay times
        params[i].Init(petal.knob[i * 2],
                       samplerate * .05,
                       MAX_DELAY,
                       Parameter::LOGARITHMIC);
    }
}

void UpdateOled();

int main(void)
{
    float samplerate;
    petal.Init();
    petal.SetAudioBlockSize(4); // Initialize hardware (daisy seed, and petal)
    samplerate = petal.AudioSampleRate();

    InitDelays(samplerate);

    drywet     = 50;
    passThruOn = false;

    petal.StartAdc();
    petal.StartAudio(AudioCallback);
    while(1)
    {
        int32_t whole;
        float   frac;
        whole = (int32_t)((float)drywet / 12.5f);
        frac  = (float)drywet / 12.5f - whole;
        petal.ClearLeds();

        // Set full bright
        for(int i = 0; i < whole; i++)
        {
            petal.SetRingLed(
                static_cast<DaisyPetal::RingLed>(i), 0.f, 0.f, 1.f);
        }

        // Set Frac
        if(whole < 7 && whole > 0)
            petal.SetRingLed(
                static_cast<DaisyPetal::RingLed>(whole - 1), 0.f, 0.f, frac);

        // Update Pass thru
        petal.SetFootswitchLed(DaisyPetal::FOOTSWITCH_LED_1, passThruOn);
        petal.UpdateLeds();
        System::Delay(6);
    }
}

void ProcessControls()
{
    petal.ProcessAnalogControls();
    petal.ProcessDigitalControls();

    //knobs
    for(int i = 0; i < 3; i++)
    {
        delays[i].delayTarget = params[i].Process();
        delays[i].feedback    = petal.knob[(i * 2) + 1].Process();
    }

    //encoder
    drywet += 5 * petal.encoder.Increment();
    drywet > 100 ? drywet = 100 : drywet = drywet;
    drywet < 0 ? drywet = 0 : drywet = drywet;

    //footswitch
    if(petal.switches[0].RisingEdge())
    {
        passThruOn = !passThruOn;
    }
}
