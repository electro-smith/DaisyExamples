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
float v, p, lf, la, attack, release, c;
float oldk1, oldk2;

void ConditionalParameter(float o, float n, float &param, float update);

static void AudioCallback(float *in, float *out, size_t size)
{
    float sig, ad_out;
    
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
    switch (mode)
    {
        case 0:
	    ConditionalParameter(oldk1, k1, c, cutoff.Process());
	    ConditionalParameter(oldk2, k2, p, pitch.Process());
	    flt.SetFreq(c);
	    break;
        case 1:
	    ConditionalParameter(oldk1, k1, attack, pod.knob1.Process());
	    ConditionalParameter(oldk2, k2, release, pod.knob2.Process());
	    ad.SetTime(ADENV_SEG_ATTACK, attack);
	    ad.SetTime(ADENV_SEG_DECAY, release);
	    break;
	case 2:
	    ConditionalParameter(oldk1, k1, lf, lfof.Process());
	    ConditionalParameter(oldk2, k2, la, pod.knob2.Process());
	    lfo.SetFreq(lf * 500);
	    lfo.SetAmp(la * 100);
	default:
	    break;
    }

    pod.led1.Set(mode == 2, mode == 1, mode == 0);
    pod.led2.Set(mode == 2, mode == 1, mode == 0);
    
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
    attack = .01f;
    release = .2f;
    c = 10000;
    la = 1.0f;
    lf = 0.1f;
    
    pod.Init();
    sample_rate = pod.AudioSampleRate();
    osc.Init(sample_rate);
    flt.Init(sample_rate);
    ad.Init(sample_rate);
    lfo.Init(sample_rate);

    //Set filter parameters
    flt.SetFreq(10000);
    flt.SetRes(0.8);
    
    // Set parameters for oscillator
    osc.SetWaveform(osc.WAVE_SAW);
    wave = osc.WAVE_SAW;
    osc.SetFreq(440);
    osc.SetAmp(1);

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
    cutoff.Init(pod.knob1, 100, 5000, cutoff.LOGARITHMIC);
    pitch.Init(pod.knob2, 200, 5000, pitch.LOGARITHMIC);
    lfof.Init(pod.knob1, 0, 1000, lfof.LOGARITHMIC);
    
    // start callback
    pod.StartAdc();
    pod.StartAudio(AudioCallback);

    while(1) {}
}

void ConditionalParameter(float o, float n, float &param, float update)
{
    if (abs(o - n) > 0.0005)
    {
        param = update;
    }
}
