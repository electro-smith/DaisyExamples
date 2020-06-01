// #PodStepSequencer

// Edit and play mode, press encoder to switch modes. Edit is red, play is green.

// Edit mode
// Rotate encoder to run through steps. Knob one sets decay time, knob two sets pitch.
// Press either button to activate or deactivate a step.
// If the led is lit, the step is active.
//   * Note: rotating through a step changes its settings.
//   * Note: decay times are relative to tempo. I.e. decay = % of step length.

// Play mode
// Knob one controls tempo.
// Turning the encoder switches waveform.
// Knob 2 and buttons do nothing.

#include "daisy_pod.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPod   pod;

Oscillator osc;
AdEnv env;
Parameter parame1, parame2, paramp1;
Metro tick;

bool   edit; //if we aren't editing, we're playing
uint8_t step;
uint8_t wave;
float dec[8];
float pitch[8];
bool   active[8];
float env_out;

static void AudioCallback(float *in, float *out, size_t size)
{
    float sig;
  
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
        
	dec[step] = parame1.Process();

	pitch[step] = (int)parame2.Process();
	pitch[step] = mtof(pitch[step]) + 220;

	if (pod.button1.RisingEdge() || pod.button2.RisingEdge())
	{
	    active[step] = !active[step];
	}
	
	pod.led1.Set(active[step],0,0);
	pod.led2.Set(active[step],0,0);
    }

    else
    {
        wave += pod.encoder.Increment();
	wave = (wave % osc.WAVE_LAST + osc.WAVE_LAST) % osc.WAVE_LAST;
	osc.SetWaveform(wave);
	
	osc.SetFreq(pitch[step]);

	uint8_t temp = paramp1.Process();
	env.SetTime(ADENV_SEG_DECAY, dec[step] * (1.0f / (float)temp));	
	
	tick.SetFreq(temp);

	pod.led2.Set(0,env_out,0);
	pod.led1.Set(0,env_out,0);
    }

    pod.UpdateLeds();
    
    // Audio Loop
    sig = 0;
    for(size_t i = 0; i < size; i += 2)
    {
        if (!edit)
	{
            env_out = env.Process();
	    osc.SetAmp(env_out);
	    sig = osc.Process();
	}

        if (tick.Process() && !edit)
	{
	    step++;
	    step %= 8;
	    if (active[step])
	    {
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

    //Set up parameters
    parame1.Init(pod.knob1, .01, 1, parame1.LINEAR);
    parame2.Init(pod.knob2, 0, 36, parame2.LINEAR);
    paramp1.Init(pod.knob1, 1, 15, paramp1.LINEAR);

    //Osc parameters
    osc.SetWaveform(osc.WAVE_TRI);

    //Envelope parameters
    env.SetTime(ADENV_SEG_ATTACK, 0.01);
    env.SetMin(0.0);
    env.SetMax(1);
    
    edit = true;
    step = 0;
    env_out = 0;
    wave = 0;

    for (int i = 0; i < 8; i++)
    {
        dec[i] = .5;
	active[i] = false;
	pitch[i] = 0;
    }

    pod.StartAdc();
    pod.StartAudio(AudioCallback);

    while (1){}
}
