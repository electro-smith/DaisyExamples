#include "daisysp.h"
#include "daisy_pod.h"

using namespace daisysp;
using namespace daisy;

static DaisyPod pod;
static Oscillator osc;
static Tone flt;
static AdEnv ad;
static Parameter pitch, cutoff;

int wave, mode;


static void AudioCallback(float *in, float *out, size_t size)
{
    float sig, p, c, ad_out, attack, release;

    pod.UpdateAnalogControls();
    pod.DebounceControls();
    
    wave += pod.encoder.RisingEdge();
    wave %= osc.WAVE_LAST;
    osc.SetWaveform(wave);
    
    mode += pod.encoder.Increment();
    mode %= 2;
    
    
    if (mode == 0)
    {
        p  = pitch.Process();
	c = cutoff.Process();
      	osc.SetFreq(p);
	flt.SetFreq(c);
    }
    else
    {
	attack   = pod.GetKnobValue(pod.KNOB_1);
	release  = pod.GetKnobValue(pod.KNOB_2);
	ad.SetTime(ADENV_SEG_ATTACK, attack);
	ad.SetTime(ADENV_SEG_DECAY, release);
    }

    if (pod.button1.RisingEdge())
      {
	ad.Trigger();
      }
    
    for (size_t i = 0; i < size; i += 2)
    {
        ad_out = ad.Process();
	
	sig = osc.Process();
	sig = flt.Process(sig);
	sig *= ad_out;
	
	// left out
	out[i] = sig;
	
	// right out
	out[i+1] = sig;
    }
}

int main(void)
{
    // initialize pod hardware and oscillator daisysp module
    float sample_rate;
    mode = wave = 0;
    
    pod.Init();
    sample_rate = pod.AudioSampleRate();
    osc.Init(sample_rate);
    flt.Init(sample_rate);
    ad.Init(sample_rate);
    
    // Set parameters for oscillator
    osc.SetWaveform(osc.WAVE_SIN);
    osc.SetFreq(440);
    osc.SetAmp(0.5);

    //Set envelope parameters
    ad.SetTime( ADENV_SEG_ATTACK, 0.2);
    ad.SetTime( ADENV_SEG_DECAY, .01);
    ad.SetMax(0.5);
    ad.SetMin(0);
    ad.SetCurve(0.5);

    //set parameters for cutoff parameter
    cutoff.Init(pod.knob1, 0, 20000, cutoff.EXPONENTIAL);
    pitch.Init(pod.knob2, 0, 20000, pitch.EXPONENTIAL);
    
    
    // start callback
    pod.StartAdc();
    pod.StartAudio(AudioCallback);

    while(1) {}
}
