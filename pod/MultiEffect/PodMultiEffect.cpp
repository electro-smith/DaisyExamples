//#PodMultiEffect
// ~~
// Multiple effects on audio in.
// Turn encoder to rotate through modes. (Only one effect active at a time)
// Led brightness indicates knob position.
//   * Note: when you cycle through modes it changes their settings!!
// ~~
// ## Mode one (red) : reverb
//       *Knob one sets dry wet
//       *Knob two sets reverb time (feedback)
// ~~
// ## Mode two (green) : delay
//      *Knob one sets delay time
//      *Knob two sets feedback level / drywet amount
// ~~
// ## Mode three (purple) : downsampler
//      *Knob one sets lowpass filter cutoff
//      *Knob two sets downsample amount

#include "daisysp.h"
#include "daisy_pod.h"

// Set max delay time to 0.75 of samplerate.
#define MAX_DELAY static_cast<size_t>(48000 * 2.5f)
#define REV 0
#define DEL 1
#define CRU 2


using namespace daisysp;
using namespace daisy;

static DaisyPod  pod;

static ReverbSc  rev;
static DelayLine <float, MAX_DELAY>  DSY_SDRAM_BSS dell;
static DelayLine <float, MAX_DELAY>  DSY_SDRAM_BSS delr;
static Tone tone;
static Parameter zero_one_l, zero_one_r,  deltime, cutoff, crushrate;
int mode = REV;

float delt;

int crushmod, crushcount;
float crushsl, crushsr;

static void AudioCallback(float *in, float *out, size_t size)
{
    float sigl, sigr, inl, inr, k1, k2, drywet, feedback, c, d;
    d = feedback = drywet = 0;
    
    pod.UpdateAnalogControls();
    pod.DebounceControls();
    
    mode = mode + pod.encoder.Increment();
    mode = (mode % 3 + 3) % 3;

    k1 = zero_one_l.Process();
    k2 = zero_one_r.Process(); 
    
    switch (mode)
    {
        case REV:
   	    drywet = k1;
	    rev.SetFeedback(k2);
	    break;
        case DEL:
	    d = deltime.Process();
	    feedback = k2;
	    break;
        case CRU:
	    c = cutoff.Process();
	    tone.SetFreq(c);
	    crushmod = (int) crushrate.Process();
    }	 

    //leds
    pod.led1.Set(k1 * (mode == 2), k1 * (mode == 1), k1 * (mode == 0 || mode == 2));
    pod.led2.Set(k2 * (mode == 2), k2 * (mode == 1), k2 * (mode == 0 || mode == 2));
    
    pod.UpdateLeds();

    //audio
    for (size_t i = 0; i < size; i += 2)
    {
        inl = in[i];
        inr = in[i + 1];
	
	switch (mode)
	{
	    case REV:
	        rev.Process(inl, inr, &sigl, &sigr);
		sigl = drywet * sigl + (1 - drywet) * inl;
		sigr = drywet * sigr + (1 - drywet) * inr;
		break;
	    case DEL:
	        fonepole(delt, d, .00007f);
 	        delr.SetDelay(delt);
		dell.SetDelay(delt);
		sigl = dell.Read();
		sigr = delr.Read();

		dell.Write((feedback * sigl) + inl);
		sigl = (feedback * sigl) + ((1.0f - feedback) * inl);

		delr.Write((feedback * sigr) + inr);
		sigr = (feedback * sigr) + ((1.0f - feedback) * inr);
		break;
	    case CRU:
	        crushcount++;
		crushcount %= crushmod;
		if (crushcount == 0)
		{
		    crushsr = inr;
		    crushsl = inl;
		}
		sigl = tone.Process(crushsl);
		sigr = tone.Process(crushsr);
	}
	
	// left out
	out[i] = sigl;
	
	// right out
	out[i+1] = sigr;
    }
}

int main(void)
{
    // initialize pod hardware and oscillator daisysp module
    float sample_rate;
    
    //Inits and sample rate
    pod.Init();
    sample_rate = pod.AudioSampleRate();
    rev.Init(sample_rate);
    dell.Init();
    delr.Init();
    tone.Init(sample_rate);
    
    //set parameters
    zero_one_l.Init(pod.knob1, 0, 1, zero_one_l.LINEAR);
    zero_one_r.Init(pod.knob2, 0, 1, zero_one_r.LINEAR);
    deltime.Init(pod.knob1, sample_rate * .05, MAX_DELAY, deltime.LOGARITHMIC);
    cutoff.Init(pod.knob1, 500, 20000, cutoff.LOGARITHMIC);
    crushrate.Init(pod.knob2, 1, 50, crushrate.LOGARITHMIC);
    
    //reverb parameters
    rev.SetLpFreq(18000.0f);
    rev.SetFeedback(0.85f);

    //delay parameters
    // Set Delay time to 0.75 seconds
    delt = sample_rate * 0.75f;
    dell.SetDelay(delt);
    delr.SetDelay(delt);
    
    // start callback
    pod.StartAdc();
    pod.StartAudio(AudioCallback);

    while(1) {}
}
