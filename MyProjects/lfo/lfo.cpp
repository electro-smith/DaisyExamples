#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisysp;
using namespace daisy;

static DaisySeed  hardware;
static MoogLadder flt;
static Oscillator osc, lfo;

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float saw, freq, output;
    float lfo_freq = mtof(hardware.adc.GetFloat(0) * 127) / 627.1925; // this makes the maximum lfo frequency 20hz
    lfo.SetFreq(lfo_freq);

    for(size_t i = 0; i < size; i += 2)
    {
        freq = 5000 + (lfo.Process() * 5000);

        osc.SetFreq(freq);
        saw  = osc.Process();

        //flt.SetFreq(freq); // uncomment to modulate filter cutoff with lfo
        output = flt.Process(saw);

        // left out
        out[i] = output;

        // right out
        out[i + 1] = output;
    }
}

int main(void)
{
    // initialize seed hardware and daisysp modules
    float sample_rate;
    hardware.Configure();
    hardware.Init();
    hardware.SetAudioBlockSize(4);
    sample_rate = hardware.AudioSampleRate();

    // initialize Moogladder object
    flt.Init(sample_rate);
    flt.SetRes(0.7);

    // set parameters for sine oscillator object
    lfo.Init(sample_rate);
    lfo.SetWaveform(Oscillator::WAVE_SIN);
    lfo.SetAmp(1);
    lfo.SetFreq(.4);

    // set parameters for sine oscillator object
    osc.Init(sample_rate);
    osc.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    osc.SetFreq(100);
    osc.SetAmp(0.25);

    /// Set up potentiometer
    
    // Create an ADC configuration
    AdcChannelConfig adcConfig;
    // Add pin 21 as an analog input in this config. We'll use this to read the knob
    adcConfig.InitSingle(hardware.GetPin(21));
    // Set the ADC to use our configuration
    hardware.adc.Init(&adcConfig, 1);
    // Start the adc
    hardware.adc.Start();

    // start callback
    hardware.StartAudio(AudioCallback);


    while(1) {}
}
