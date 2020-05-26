#include "daisysp.h"
#include "daisy_pod.h"

using namespace daisysp;
using namespace daisy;

static DaisyPod pod;
static Oscillator osc;
static Tone flt;
static Adsr adsr;

int wave, mode;


static void AudioCallback(float *in, float *out, size_t size)
{
    float sig, pitch, cutoff, adsr_out, attack, release;
  
    pod.encoder.Debounce();
    wave += pod.encoder.RisingEdge();
    wave %= osc.WAVE_LAST;
    osc.SetWaveform(wave);

    mode += pod.encoder.Increment();
    mode %= 2;
    
    pod.UpdateAnalogControls();
    pod.DebounceControls();
    
    adsr_out = adsr.Process(pod.button1.Pressed());

    if (mode == 0)
    {
        pitch  = pod.GetKnobValue(pod.KNOB_2) * 5000;
	cutoff = pod.GetKnobValue(pod.KNOB_1) * 10000;
	osc.SetFreq(pitch);
        flt.SetFreq(cutoff);
    }
    else
    {
        attack   = pod.GetKnobValue(pod.KNOB_1) * 0.05;
	release  = pod.GetKnobValue(pod.KNOB_2) * 0.05;
	adsr.SetTime(ADSR_SEG_ATTACK, attack);
	adsr.SetTime(ADSR_SEG_RELEASE, release);
    }
    
    for (size_t i = 0; i < size; i += 2)
    {	
    	sig = osc.Process();
	sig = flt.Process(sig);
	sig *= adsr_out;
	
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
    adsr.Init(sample_rate);
    
    // Set parameters for oscillator
    osc.SetWaveform(osc.WAVE_SIN);
    osc.SetFreq(440);
    osc.SetAmp(0.5);

    //Set envelope parameters
    adsr.SetTime( ADSR_SEG_ATTACK, 0.02);
    adsr.SetTime( ADSR_SEG_DECAY, .00001);
    adsr.SetSustainLevel(1);
    adsr.SetTime( ADSR_SEG_RELEASE, 0.02);
    
    // start callback
    pod.StartAdc();
    pod.StartAudio(AudioCallback);

    while(1) {}
}
