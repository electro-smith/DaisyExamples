#include "daisysp.h"
#include "daisy_pod.h"

using namespace daisysp;
using namespace daisy;

static DaisyPod pod;
static Oscillator osc, lfo;
static Tone flt;
static AdEnv ad;
static Parameter pitch, cutoff, lfof;

int wave, mode;
float v, p;


// Simple Synth voice example. Encoder cycles through three modes. Mode one controls cutoff and oscillator frequency. Mode 2 controls attack and decay times. Mode 3 controls vibrato rate and depth.
// Click encoder to change waveform. Press either button to trigger the envelope.
// Leds light to indicate what mode is selected.

static void AudioCallback(float *in, float *out, size_t size)
{
  float sig, c, ad_out, attack, release, lf, la;

    pod.UpdateAnalogControls();
    pod.DebounceControls();
    
    wave += pod.encoder.RisingEdge();
    wave %= osc.WAVE_LAST;
    osc.SetWaveform(wave);
    
    mode += pod.encoder.Increment();
    mode %= 3;
    
    switch (mode) {
      case 0:
          p  = pitch.Process();
	  c = cutoff.Process();
	  flt.SetFreq(c);
	  pod.led1.Set(0,0,1);
	  pod.led2.Set(0,0,0);
	  break;
      case 1:
   	  attack   = pod.GetKnobValue(pod.KNOB_1);
	  release  = pod.GetKnobValue(pod.KNOB_2);
	  ad.SetTime(ADENV_SEG_ATTACK, attack);
	  ad.SetTime(ADENV_SEG_DECAY, release);
	  pod.led1.Set(0,0,0);
	  pod.led2.Set(0,1,0);
	  break;
      case 2:
  	  lf = lfof.Process();
	  la = pod.GetKnobValue(pod.KNOB_2);
	  lfo.SetFreq(lf * 500);
	  lfo.SetAmp(la * 100);
	  pod.led1.Set(1,0,0);
	  pod.led2.Set(1,0,0);
      default:
	  break;
    }	  

    pod.UpdateLeds();
    
    if (pod.button1.RisingEdge() || pod.button2.RisingEdge())
    {
  	ad.Trigger();
    }
    
    for (size_t i = 0; i < size; i += 2)
    {
        ad_out = ad.Process();
	v = lfo.Process();

	osc.SetFreq(p + v);
	
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
    v = 0.0f;
    p = 1000.0f;
    
    pod.Init();
    sample_rate = pod.AudioSampleRate();
    osc.Init(sample_rate);
    flt.Init(sample_rate);
    ad.Init(sample_rate);
    lfo.Init(sample_rate);
    
    // Set parameters for oscillator
    osc.SetWaveform(osc.WAVE_SIN);
    osc.SetFreq(440);
    osc.SetAmp(0.5);

    // Set parameters for lfo
    lfo.SetWaveform(osc.WAVE_SIN);
    lfo.SetFreq(0.1);
    lfo.SetAmp(1);
    
    //Set envelope parameters
    ad.SetTime( ADENV_SEG_ATTACK, 0.01);
    ad.SetTime( ADENV_SEG_DECAY, .2);
    ad.SetMax(0.5);
    ad.SetMin(0);
    ad.SetCurve(0.5);

    //set parameters for cutoff parameter
    cutoff.Init(pod.knob1, 0, 10000, cutoff.EXPONENTIAL);
    pitch.Init(pod.knob2, 0, 10000, pitch.EXPONENTIAL);
    lfof.Init(pod.knob1, 0, 1000, lfof.LOGARITHMIC);
    
    // start callback
    pod.StartAdc();
    pod.StartAudio(AudioCallback);

    while(1) {}
}
