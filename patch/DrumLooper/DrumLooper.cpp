#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

using namespace daisy;
using namespace daisysp;

#define MAX_SIZE (1000 * 1) // 1 minute at 1000Hz

DaisyPatch patch;

Oscillator osc;
WhiteNoise noise;

AdEnv kckPitchEnv;
AdEnv volEnvs[3];

bool first = true;  //first loop (sets length)
bool rec   = false; //currently recording
bool play  = false; //currently playing

int  pos = 0;
bool DSY_SDRAM_BSS buf[3][MAX_SIZE];
int   mod = MAX_SIZE;
int   len = 0;
float drywet = 0;
bool  res = false;

int recChan = 0;

void UpdateControls();

void UpdateEnvs()
{
    for (int chn = 0; chn < 3; chn++)
    {
        if (buf[chn][pos])
        {
            volEnvs[chn].Trigger();
            if (chn == 0)
            {
                kckPitchEnv.Trigger();
            }
        }
    }
 
    if(play)
    {
        pos++;
        pos %= mod;
    }

    //automatic looptime
    if(len >= MAX_SIZE)
    {
        first = false;
        mod   = MAX_SIZE;
        len   = 0;
    }
}

static void AudioCallback(float **in, float **out, size_t size)
{
    UpdateControls();
    UpdateEnvs();

    for (size_t i = 0; i < size; i ++){
        float outs[3]; 

        osc.SetFreq(kckPitchEnv.Process());
        osc.SetAmp(volEnvs[0].Process());
        outs[0] = osc.Process();

        float noise_out = noise.Process();
        outs[1] = noise_out * volEnvs[1].Process();

        outs[2]= noise_out * volEnvs[2].Process();

        float mix = 0.f;

        for (size_t chn = 0; chn < 3; chn++)
        {
            out[chn][i] = outs[chn];
            mix += .3f * outs[chn];
        }
        out[4][i] = mix;
    }
}

void UpdateOled();

void InitEnvs(float samplerate)
{
    for (int i = 0; i < 3; i++)
    {
        volEnvs[i].Init(samplerate);
        volEnvs[i].SetTime(ADENV_SEG_ATTACK, .01);
    }

    //This envelope will control the kick oscillator's pitch
    //Note that this envelope is much faster than the volume
    kckPitchEnv.Init(samplerate);
    kckPitchEnv.SetTime(ADENV_SEG_ATTACK, .01);
    kckPitchEnv.SetMax(400);
    kckPitchEnv.SetMin(50);
}

int main(void)
{
    float samplerate;
    int num_waves = Oscillator::WAVE_LAST - 1;
    patch.Init(); // Initialize hardware (daisy seed, and patch)
    samplerate = patch.AudioSampleRate();
   
    //Initialize oscillator for kickdrum
    osc.Init(samplerate);
    osc.SetWaveform(Oscillator::WAVE_TRI);

    //Initialize noise
    noise.Init();
    
    InitEnvs(samplerate);

    //Init loop stuff
    ResetBuffer();

    patch.StartAdc();
    patch.StartAudio(AudioCallback);
    while(1) 
    {
        UpdateOled();
    }
}

//Resets the buffer
void ResetBuffer()
{
    play  = false;
    rec   = false;
    first = true;
    pos   = 0;
    len   = 0;
    
    for (int chn = 0; chn < 3; chn++){
        for(int i = 0; i < mod; i++)
        {
            buf[chn][i] = 0;
        }
    }
    
    mod = MAX_SIZE;
}

void UpdateOled()
{

}
void UpdateControls()
{
    patch.UpdateAnalogControls();
    patch.DebounceControls();

    //encoder pressed
    if(patch.encoder.RisingEdge())
    {
        if(first && rec)
        {
            first = false;
            mod   = len;
            len   = 0;
        }

    	res = true;
        play = true;
        rec  = !rec;
    }

    //encoder held
    if(patch.encoder.TimeHeldMs() >= 1000 && res)
    {
        ResetBuffer();
    	res = false;
    }

    //encoder turned
    recChan = patch.encoder.Increment();
    recChan = (recChan % 3 + 3) % 3;

    //gate in 
    if (patch.gate_input[0].Trig() && rec)
    {
        buf[recChan][pos] = true;
    }
}
