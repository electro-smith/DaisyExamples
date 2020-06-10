#include "daisy_seed.h"
#include "daisysp.h"

// Use the daisy namespace to prevent having to type 
// daisy:: before all libdaisy functions
using namespace daisy;
using namespace daisysp;

// Declare a DaisySeed object called hardware
DaisySeed hardware;
Oscillator osc;
AdEnv env;

Switch button1;

void AudioCallback(float* in, float* out, size_t size)
{
    float osc_out, env_out;

    button1.Debounce();

    if (button1.RisingEdge())
    {
	env.Trigger();
    }

    osc.SetFreq(hardware.adc.GetFloat(0) * 5000);
    
    for (size_t i = 0; i < size; i+=2)
    {
        env_out = env.Process();
	osc.SetAmp(env_out);
	osc_out = osc.Process();
		
	out[i] = osc_out;
	out[i+1] = osc_out;
    }
}


int main(void)
{
    // Configure and Initialize the Daisy Seed
    // These are separate to allow reconfiguration of any of the internal
    // components before initialization.
    hardware.Configure();
    hardware.Init();
    hardware.SetAudioBlockSize(48);

    float samplerate = hardware.AudioSampleRate();
    float callbackrate = samplerate / 48.f;
    
    //Set up knob
    AdcChannelConfig adcConfig;
    adcConfig.InitSingle(hardware.GetPin(21));
    hardware.adc.Init(&adcConfig, 1);
    hardware.adc.Start();

    //Set up button
    button1.Init(hardware.GetPin(28), callbackrate);

    //Set up oscillator
    osc.Init(samplerate);
    osc.SetWaveform(osc.WAVE_SIN);
    osc.SetAmp(.25);
    osc.SetFreq(1000);

    //Set up volume envelope
    env.Init(samplerate);
    env.SetTime(ADENV_SEG_DECAY, .4);
    env.SetTime(ADENV_SEG_ATTACK, .01);
    env.SetMin(0.0);
    env.SetMax(0.25);
    env.SetCurve(0); // linear
    
    hardware.StartAudio(AudioCallback);
    // Loop forever
    for(;;) {}
}
