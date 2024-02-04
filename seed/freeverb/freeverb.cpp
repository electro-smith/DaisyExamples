#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

static DaisySeed             hw;
static VariableSawOscillator osc;
static AdEnv                 env;
static Metro                 metro;
static Maytrig               maybe;
static Freeverb              verb;
static constexpr int         kNumNotes = 15;
static int32_t               notes[kNumNotes]
    = {0, 2, 3, 5, 7, 8, 10, 12, 14, 15, 17, 19, 20, 22, 24};

void AudioCallback(float **in, float **out, size_t size)
{
    // Generate Music
    float buff[size];
    float wetleft[size], wetright[size];

    for(size_t i = 0; i < size; i++)
    {
        if(metro.Process())
        {
            if(maybe.Process(0.125f))
            {
                osc.SetFreq(mtof(48.f + notes[rand() % kNumNotes]));
                env.Trigger();
            }
        }
        buff[i] = osc.Process() * env.Process() * 0.5f;
        verb.Process(&buff[i], &buff[i], &out[0][i], &out[1][i]);
        //out[0][i]  = out[1][i] = buff[i];
    }
    // Process through Verb to outputs
    //verb.ProcessBlockReplace(buff, buff, out[0], out[1], size);
}

int main(void)
{
    hw.Configure();
    hw.Init();
    osc.Init(hw.AudioSampleRate());
    osc.SetPW(0.25f);
    osc.SetWaveshape(0.2f);
    env.Init(hw.AudioSampleRate());
    env.SetTime(ADENV_SEG_ATTACK, 0.001f);
    env.SetTime(ADENV_SEG_DECAY, 0.25f);
    metro.Init(8.f, hw.AudioSampleRate());
    verb.Init();
    verb.SetRoomSize(0.95f);
    verb.SetWidth(1.f);
    verb.SetDamp(0.6f);
    verb.SetWidth(0.8f);
    hw.StartAudio(AudioCallback);
    while(1) {}
}
