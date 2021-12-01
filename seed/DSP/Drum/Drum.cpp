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

void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
    float osc_out, noise_out, snr_env_out, kck_env_out, sig;
    //Get rid of any bouncing
    snare.Debounce();
    kick.Debounce();

    //If you press the kick button...
    if(kick.RisingEdge())
    {
        //Trigger both envelopes!
        kickVolEnv.Trigger();
        kickPitchEnv.Trigger();
    }

    //If press the snare button trigger its envelope
    if(snare.RisingEdge())
    {
        snareEnv.Trigger();
    }

    //Prepare the audio block
    for(size_t i = 0; i < size; i += 2)
    {
        //Get the next volume samples
        snr_env_out = snareEnv.Process();
        kck_env_out = kickVolEnv.Process();

        //Apply the pitch envelope to the kick
        osc.SetFreq(kickPitchEnv.Process());
        //Set the kick volume to the envelope's output
        osc.SetAmp(kck_env_out);
        //Process the next oscillator sample
        osc_out = osc.Process();

        //Get the next snare sample
        noise_out = noise.Process();
        //Set the sample to the correct volume
        noise_out *= snr_env_out;

        //Mix the two signals at half volume
        sig = .5 * noise_out + .5 * osc_out;

        //Set the left and right outputs to the mixed signals
        out[i]     = sig;
        out[i + 1] = sig;
    }
}

int main(void)
{
    // Configure and Initialize the Daisy Seed
    // These are separate to allow reconfiguration of any of the internal
    // components before initialization.
    hardware.Configure();
    hardware.Init();
    hardware.SetAudioBlockSize(4);
    float samplerate = hardware.AudioSampleRate();

    //Initialize oscillator for kickdrum
    osc.Init(samplerate);
    osc.SetWaveform(Oscillator::WAVE_TRI);
    osc.SetAmp(1);

    //Initialize noise
    noise.Init();

    //Initialize envelopes, this one's for the snare amplitude
    snareEnv.Init(samplerate);
    snareEnv.SetTime(ADENV_SEG_ATTACK, .01);
    snareEnv.SetTime(ADENV_SEG_DECAY, .2);
    snareEnv.SetMax(1);
    snareEnv.SetMin(0);

    //This envelope will control the kick oscillator's pitch
    //Note that this envelope is much faster than the volume
    kickPitchEnv.Init(samplerate);
    kickPitchEnv.SetTime(ADENV_SEG_ATTACK, .01);
    kickPitchEnv.SetTime(ADENV_SEG_DECAY, .05);
    kickPitchEnv.SetMax(400);
    kickPitchEnv.SetMin(50);

    //This one will control the kick's volume
    kickVolEnv.Init(samplerate);
    kickVolEnv.SetTime(ADENV_SEG_ATTACK, .01);
    kickVolEnv.SetTime(ADENV_SEG_DECAY, 1);
    kickVolEnv.SetMax(1);
    kickVolEnv.SetMin(0);

    //Initialize the kick and snare buttons on pins 27 and 28
    //The callback rate is samplerate / blocksize (48)
    snare.Init(hardware.GetPin(27), samplerate / 48.f);
    kick.Init(hardware.GetPin(28), samplerate / 48.f);

    //Start calling the callback function
    hardware.StartAudio(AudioCallback);

    // Loop forever
    for(;;) {}
}
