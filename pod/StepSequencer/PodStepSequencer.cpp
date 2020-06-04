// #PodStepSequencer

// Edit and play mode, press encoder to switch modes. Edit is red, play is green.

// Edit mode
//   *Rotate encoder to run through steps.
//   *Knob one sets decay time,
//   *Knob two sets pitch. (A pentatonic major)
//   *Press the right button to activate or deactivate a step
//   *Press the left button to hear the step you're editing (ping the envelope) (Also activates inactive steps)
//   *If the led is lit, the step is active.
//   *!!!Led color indicates step number

// Play mode
// Knob one controls tempo.
// Knob two controls filter cutoff
// Turning the encoder switches waveform.


#include "daisy_pod.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPod   pod;

Oscillator osc;
AdEnv env;
Parameter parame1, parame2, paramp1, paramp2;
Metro tick;
MoogLadder flt;

bool   edit; //if we aren't editing, we're playing
uint8_t step;
uint8_t wave;
float dec[8];
float pitch[8];
bool   active[8];
float env_out;

float pent[] = {110.f,128.33f,146.66f,174.166f,192.5f};

float oldk1, oldk2;

static void AudioCallback(float *in, float *out, size_t size)
{
    float sig;

    float k1 = pod.knob1.Process();
    float k2 = pod.knob2.Process();
    
    pod.DebounceControls();
    pod.UpdateAnalogControls();
    
    //switch modes
    if (pod.encoder.RisingEdge())
    {
        edit = !edit;
	step = 0;
    }

    if (edit)
    {
        step += pod.encoder.Increment();
	step = (step % 8 + 8) % 8;
        
	if (abs(oldk1 - k1) > .0001 || abs(oldk2 - k2) > .0001)
	{
	    dec[step] = parame1.Process();
	    int temp = (int) parame2.Process();
	    pitch[step] = pent[temp % 5] * (temp / (int) 5 + 1);
	}

	if (pod.button1.RisingEdge())
	{
	    active[step] = !active[step];
	}

	if (pod.button2.RisingEdge())
	{
  	    active[step] = true;
   	    env.SetTime(ADENV_SEG_DECAY, dec[step]);
	    env.Trigger();
	}

	//Tried to be clever, but this means step 0 is off...
	if (active[step])
	{
  	    float l4 = (step & 4) * 0.25f;
	    float l2 = (step & 2) * 0.25f;
	    float l1 = (step & 1) * 0.25f;
	    pod.led1.Set(l4,l2,l1);
	    pod.led2.Set(l4,l2,l1);
	}
	else
	{
	    pod.ClearLeds();
	}
    }

    else
    {
        wave += pod.encoder.Increment();
	wave = (wave % osc.WAVE_LAST + osc.WAVE_LAST) % osc.WAVE_LAST;
	osc.SetWaveform(wave);	

	if (abs(oldk1 - k1) > .0001 || abs(oldk2 - k2) > .0001)
	{
	    tick.SetFreq(paramp1.Process());
	    flt.SetFreq(paramp2.Process());
	}
	
	pod.led2.Set(0,env_out,0);
	pod.led1.Set(0,env_out,0);
    }

    osc.SetFreq(pitch[step]);

    oldk1 = k1;
    oldk2 = k2;
    pod.UpdateLeds();
    
    // Audio Loop
    sig = 0;
    for(size_t i = 0; i < size; i += 2)
    {  
        env_out = env.Process();
	osc.SetAmp(env_out);
	sig = osc.Process();
	sig = flt.Process(sig);

        if (tick.Process() && !edit)
	{
	    step++;
	    step %= 8;
	    if (active[step])
	    {
	        env.SetTime(ADENV_SEG_DECAY, dec[step]);
	        env.Trigger();
	    }
	}
	
	out[i] = sig;
	out[i+1] = sig;
    }
}
  
int main(void)
{
    float sample_rate;
    pod.Init();
    sample_rate = pod.AudioSampleRate();
    
    osc.Init(sample_rate);
    env.Init(sample_rate);
    tick.Init(3, sample_rate);
    flt.Init(sample_rate);
    
    //Set up parameters
    parame1.Init(pod.knob1, .03, 1, parame1.LINEAR);
    parame2.Init(pod.knob2, 0, 10, parame2.LINEAR);
    paramp1.Init(pod.knob1, 4, 13, paramp1.LINEAR);
    paramp2.Init(pod.knob2, 100, 10000, paramp2.LOGARITHMIC);

    //Osc parameters
    osc.SetWaveform(osc.WAVE_TRI);

    //Envelope parameters
    env.SetTime(ADENV_SEG_ATTACK, 0.01);
    env.SetMin(0.0);
    env.SetMax(1);
    
    //Set filter parameters
    flt.SetFreq(20000);
    flt.SetRes(0.7);
    
    //Global variables
    oldk1 = oldk2 = 0;
    edit = true;
    step = 0;
    env_out = 0;
    wave = 2;

    for (int i = 0; i < 8; i++)
    {
        dec[i] = .5;
	active[i] = false;
	pitch[i] = 12;
    }

    pod.StartAdc();
    pod.StartAudio(AudioCallback);

    while (1){}
}
