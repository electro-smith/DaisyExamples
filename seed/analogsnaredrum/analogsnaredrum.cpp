#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed       hw;
AnalogSnareDrum sd;
Metro           tick;

void AudioCallback(float **in, float **out, size_t size)
{
    for(size_t i = 0; i < size; i++)
    {
        bool t = tick.Process();
        if(t)
        {
            sd.SetDecay(random() / (float)RAND_MAX);
            sd.SetSnappy(random() / (float)RAND_MAX);
            sd.SetTone(.8f * random() / (float)RAND_MAX);
        }
        out[0][i] = out[1][i] = sd.Process(t);
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    float sample_rate = hw.AudioSampleRate();

    tick.Init(2.f, sample_rate);

    sd.Init(sample_rate);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
