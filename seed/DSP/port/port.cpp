#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisysp;
using namespace daisy;

static DaisySeed  hw;
static Port       slew;
static Metro      clock;
static Oscillator osc_sine;

float freq;

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float   sine, slewed_freq;
    uint8_t tic;
    for(size_t i = 0; i < size; i += 2)
    {
        tic = clock.Process();
        if(tic)
        {
            freq = rand() % 500;
        }

        slewed_freq = slew.Process(freq);
        osc_sine.SetFreq(slewed_freq);

        sine = osc_sine.Process();

        // left out
        out[i] = sine;

        // right out
        out[i + 1] = sine;
    }
}

int main(void)
{
    // initialize seed hardware and daisysp modules
    float sample_rate;
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    sample_rate = hw.AudioSampleRate();

    // set params for Port object
    slew.Init(sample_rate, .09);

    clock.Init(1, sample_rate);

    // set parameters for sine oscillator object
    osc_sine.Init(sample_rate);
    osc_sine.SetWaveform(Oscillator::WAVE_SIN);
    osc_sine.SetFreq(100);
    osc_sine.SetAmp(0.25);


    // start callback
    hw.StartAudio(AudioCallback);


    while(1) {}
}
