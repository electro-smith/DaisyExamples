#include "daisy_seed.h"
#include "daisysp.h"

// Use the daisy namespace to prevent having to type 
// daisy:: before all libdaisy functions
using namespace daisy;
using namespace daisysp;

// Declare a DaisySeed object called hardware
DaisySeed hardware;

Oscillator osc;
WhiteNoise noise;

AdEnv kickVolEnv, kickPitchEnv, snareEnv;

Switch kick, snare;

void AudioCallback(float* in, float* out, size_t size)
{
    float osc_out, noise_out, snr_env_out, kck_env_out, sig;
    snare.Debounce();
    kick.Debounce();

    if (kick.RisingEdge())
    {
        kickVolEnv.Trigger();
	kickPitchEnv.Trigger();
    }

    if (snare.RisingEdge())
    {
        snareEnv.Trigger();
    }

    for (size_t i = 0; i < size; i+= 2)
    {
        snr_env_out = snareEnv.Process();
	kck_env_out = kickVolEnv.Process();

	osc.SetFreq(kickPitchEnv.Process());
	osc.SetAmp(kck_env_out);
	osc_out = osc.Process();

	noise_out = noise.Process();
	noise_out *= snr_env_out;

	sig = .5 * noise_out + .5 + osc_out;
	
        out[i] = sig;
	out[i+1] = sig;
    }
}

int main(void)
{
    // Configure and Initialize the Daisy Seed
    // These are separate to allow reconfiguration of any of the internal
    // components before initialization.
    hardware.Configure();
    hardware.Init();
    float samplerate = hardware.AudioSampleRate();
    
    //Initialize oscillator for kickdrum
    osc.Init(samplerate);
    osc.SetWaveform(Oscillator::WAVE_TRI);
    osc.SetAmp(1);

    //Initialize noise
    noise.Init();
    
    //Initialize envelopes
    snareEnv.Init(samplerate);
    snareEnv.SetTime(ADENV_SEG_ATTACK, .01);
    snareEnv.SetTime(ADENV_SEG_DECAY, .2);
    snareEnv.SetMax(1);
    snareEnv.SetMin(0);

    //Initialize envelopes
    kickPitchEnv.Init(samplerate);
    kickPitchEnv.SetTime(ADENV_SEG_ATTACK, .01);
    kickPitchEnv.SetTime(ADENV_SEG_DECAY, .05);
    kickPitchEnv.SetMax(400);
    kickPitchEnv.SetMin(50);

    //Initialize envelopes
    kickVolEnv.Init(samplerate);
    kickVolEnv.SetTime(ADENV_SEG_ATTACK, .01);
    kickVolEnv.SetTime(ADENV_SEG_DECAY, 1);
    kickVolEnv.SetMax(1);
    kickVolEnv.SetMin(0);

    snare.Init(hardware.GetPin(27), samplerate / 48.f);
    kick.Init(hardware.GetPin(28), samplerate / 48.f);

    hardware.StartAudio(AudioCallback);
    
    // Loop forever
    for(;;) {}
}
