// Simple Synth voice example. Turning the encoder cycles through three control modes. Each mode has different knob functions.
// In mode one (blue), knob one controls filter cutoff and knob two controls oscillator frequency.
// In mode two (green), knob one 1 controls envelope attack time and knob two controls envelope decay time.
// In mode three (red), knob one controls vibrato rate and knob two controls vibrato depth. Very fast vibratos are heard as simple FM synthesis!
// Click the encoder to cycle through waveforms. Press either button to trigger the envelope.
// Leds light to indicate what mode is selected.

#include "daisysp.h"
#include "daisy_pod.h"

using namespace daisysp;
using namespace daisy;

static DaisyPod pod;
static Oscillator osc, lfo;
static MoogLadder flt;
static AdEnv ad;
static Parameter pitch, cutoff, lfof;

int wave, mode;
float v, p;
float oldk1, oldk2;

static void AudioCallback(float *in, float *out, size_t size)
{
    float sig, c, ad_out, attack, release, lf, la;

    pod.UpdateAnalogControls();
    pod.DebounceControls();
    
    wave += pod.encoder.RisingEdge();
    wave %= osc.WAVE_LAST;
    osc.SetWaveform(wave);
    
    mode += pod.encoder.Increment();
    mode = (mode % 3 + 3) % 3;

    float k1 = pod.knob1.Process();
    float k2 = pod.knob2.Process();

    //knobs
    if (abs(oldk1 - k1) > 0.0005 || abs(oldk2 - k2) > 0.0005){
        switch (mode)
	{
	    case 0:
	        p  = pitch.Process();
		c = cutoff.Process();
		flt.SetFreq(c);
		break;
	    case 1:
  	        attack   = pod.GetKnobValue(pod.KNOB_1);
		release  = pod.GetKnobValue(pod.KNOB_2);
		ad.SetTime(ADENV_SEG_ATTACK, attack);
		ad.SetTime(ADENV_SEG_DECAY, release);
		break;
	    case 2:
  	        lf = lfof.Process();
		la = pod.GetKnobValue(pod.KNOB_2);
		lfo.SetFreq(lf * 500);
		lfo.SetAmp(la * 100);
	    default:
	        break;
	}
    }

    //leds
    switch (mode)
    {
        case 0:
	    pod.led1.Set(0,0,1);
	    pod.led2.Set(0,0,1);
	    break;
        case 1:
	    pod.led1.Set(0,1,0);
	    pod.led2.Set(0,1,0);
	    break;
        case 2:
	    pod.led1.Set(1,0,0);
	    pod.led2.Set(1,0,0);
        default: 
	    break;
    }
    
    oldk1 = k1;
    oldk2 = k2;
    
    pod.UpdateLeds();

    //buttons
    if (pod.button1.RisingEdge() || pod.button2.RisingEdge())
    {
  	ad.Trigger();
    }

    //audio
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
    mode = 0;
    v = 0.0f;
    p = 1000.0f;
    oldk1 = oldk2 = 0;
    
    pod.Init();
    sample_rate = pod.AudioSampleRate();
    osc.Init(sample_rate);
    flt.Init(sample_rate);
    ad.Init(sample_rate);
    lfo.Init(sample_rate);

    //Set filter parameters
    flt.SetFreq(1000);
    flt.SetRes(0.8);
    
    // Set parameters for oscillator
    osc.SetWaveform(osc.WAVE_SAW);
    wave = osc.WAVE_SAW;
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
    cutoff.Init(pod.knob1, 100, 10000, cutoff.LOGARITHMIC);
    pitch.Init(pod.knob2, 200, 10000, pitch.LOGARITHMIC);
    lfof.Init(pod.knob1, 0, 1000, lfof.LOGARITHMIC);
    
    // start callback
    pod.StartAdc();
    pod.StartAudio(AudioCallback);

    while(1) {}
}
