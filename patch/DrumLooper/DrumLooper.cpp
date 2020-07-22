#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

using namespace daisy;
using namespace daisysp;

#define MAX_SIZE (1000 * 60) // 1 minute at 1000Hz

DaisyPatch patch;

Oscillator osc;
WhiteNoise noise;

AdEnv kckPitchEnv;
AdEnv volEnvs[3];

bool first = true;  //first loop (sets length)
bool rec   = false; //currently recording

int  pos = 0;
bool DSY_SDRAM_BSS buf[3][MAX_SIZE];
int   mod = MAX_SIZE;
int   len = 0;
float drywet = 0;

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
        float mixLevels[] = {.5f, .1f, .05f};

        for (size_t chn = 0; chn < 3; chn++)
        {
            out[chn][i] = outs[chn];
            mix += mixLevels[chn] * outs[chn];
        }

        out[3][i] = mix;
    }
}

void UpdateOled();

void InitEnvs(float samplerate)
{
    for (int i = 0; i < 3; i++)
    {
        volEnvs[i].Init(samplerate);
        volEnvs[i].SetTime(ADENV_SEG_ATTACK, .01);
        volEnvs[i].SetCurve(-4);
    }

    //This envelope will control the kick oscillator's pitch
    //Note that this envelope is much faster than the volume
    kckPitchEnv.Init(samplerate);
    kckPitchEnv.SetTime(ADENV_SEG_ATTACK, .02);
    kckPitchEnv.SetMax(200);
    kckPitchEnv.SetCurve(-4);
}

void ResetBuffer()
{
    rec   = false;
    first = true;
    pos   = 0;
    len   = 0;
    
    for (int chn = 0; chn < 3; chn++){
        for(int i = 0; i < mod; i++)
        {
            buf[chn][i] = false;
        }
    }
    
    mod = MAX_SIZE;
}


int main(void)
{
    float samplerate;
    patch.Init(); // Initialize hardware (daisy seed, and patch)
    samplerate = patch.AudioSampleRate();
   
    //Initialize oscillator for kickdrum
    osc.Init(samplerate);
    osc.SetWaveform(Oscillator::WAVE_TRI);

    //Initialize noise
    noise.Init();
    
    patch.display.Fill(false); 

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

void UpdateOled()
{
    patch.display.Fill(false);
    
    patch.display.SetCursor(0,0);
    std::string str = rec ? "rec" : "play";
    char* cstr = &str[0];  
    patch.display.WriteString(cstr, Font_7x10, true);

    patch.display.SetCursor(0,25);
    str = recChan == 0 ? "Kick" : "";
    str = recChan == 1 ? "Snare": str;
    str = recChan == 2 ? "Hat " : str;
    patch.display.WriteString(cstr, Font_7x10, true);
    
    patch.display.Update();   
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
            pos = 0;
        }

        rec  = !rec;
    }

    //encoder held
    if(patch.encoder.TimeHeldMs() >= 1000)
    {
        ResetBuffer();
    }

    //encoder turned
    recChan += patch.encoder.Increment();
    recChan = (recChan % 3 + 3) % 3;

    //parameters
    volEnvs[0].SetTime(ADENV_SEG_DECAY, patch.controls[0].Process() * 3);

    volEnvs[1].SetTime(ADENV_SEG_DECAY, patch.controls[2].Process());
    volEnvs[2].SetTime(ADENV_SEG_DECAY, patch.controls[3].Process());

    kckPitchEnv.SetMin((patch.controls[1].Process() * 4 + 1) * 20);

    //gate in 
    if (patch.gate_input[0].Trig())
    {
        if(rec || first)
        {
            buf[recChan][pos] = true;
            rec = true;
        }
    }

    //the only situation in which we don't increment is when \
    //we're waiting for the first recording
    if(!(first && !rec))
    {
	//array stuff
	pos++;
	pos %= mod;
    }
	
    //if we're making our first loop
    if(first && rec)
    {
        len++;
        //automatic looptime
        if(len >= MAX_SIZE)
        {
            first = false;
            mod   = MAX_SIZE;
            len   = 0;
	    pos = 0;
        }
    }
}
