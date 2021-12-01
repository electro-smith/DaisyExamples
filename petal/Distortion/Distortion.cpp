#include "daisysp.h"
#include "daisy_petal.h"

using namespace daisysp;
using namespace daisy;

static DaisyPetal petal;

float hardClip(float in)
{
    in = in > 1.f ? 1.f : in;
    in = in < -1.f ? -1.f : in;
    return in;
}

float softClip(float in)
{
    if(in > 0)
        return 1 - expf(-in);
    return -1 + expf(in);
}

bool        bypassHard, bypassSoft;
static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    petal.ProcessAnalogControls();
    petal.ProcessDigitalControls();

    float Pregain = petal.knob[2].Process() * 10 + 1.2;
    float Gain    = petal.knob[3].Process() * 100 + 1.2;
    float drywet  = petal.knob[4].Process();

    bypassSoft = petal.switches[0].RisingEdge() ? !bypassSoft : bypassSoft;
    bypassHard = petal.switches[1].RisingEdge() ? !bypassHard : bypassHard;

    for(size_t i = 0; i < size; i++)
    {
        for(int chn = 0; chn < 2; chn++)
        {
            float input = in[chn][i] * Pregain;
            float wet   = input;

            if(!bypassSoft || !bypassHard)
            {
                wet *= Gain;
            }

            if(!bypassSoft)
            {
                wet = softClip(wet);
            }

            if(!bypassHard)
            {
                wet = hardClip(wet);
            }

            out[chn][i] = wet * drywet + input * (1 - drywet);
        }
    }
}

int main(void)
{
    petal.Init();
    petal.SetAudioBlockSize(4);

    bypassHard = bypassSoft = false;

    // start callback
    petal.StartAdc();
    petal.StartAudio(AudioCallback);
    while(1)
    {
        //LED stuff
        petal.SetFootswitchLed((DaisyPetal::FootswitchLed)0, !bypassSoft);
        petal.SetFootswitchLed((DaisyPetal::FootswitchLed)1, !bypassHard);

        for(int i = 0; i < 8; i++)
        {
            petal.SetRingLed((DaisyPetal::RingLed)i, 1.f, 0.f, 0.f);
        }

        petal.UpdateLeds();
        System::Delay(6);
    }
}
