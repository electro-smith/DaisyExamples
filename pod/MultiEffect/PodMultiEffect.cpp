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
//      *Knob two sets feedback level
// ~~
// ## Mode three (purple) : bitcrush 
//      *Knob one sets lowpass filter cutoff
//      *Knob two sets crush rate

#include "daisysp.h"
#include "daisy_pod.h"

// Set max delay time to 0.75 of samplerate.
#define MAX_DELAY static_cast<size_t>(48000 * 0.5f)

using namespace daisysp;
using namespace daisy;

static DaisyPod  pod;

static ReverbSc  rev;
static DelayLine <float, MAX_DELAY>  del;
static Bitcrush  crush;
static Tone tone;
static Parameter zero_one_l, zero_one_r,  deltime, cutoff, crushrate;

int mode;

static void AudioCallback(float *in, float *out, size_t size)
{
  float sigl, sigr, inl, inr, k1, k2, drywet, feedback, c;
    
    pod.UpdateAnalogControls();
    pod.DebounceControls();
    
    mode += pod.encoder.Increment();
    mode = (mode % 3 + 3) % 3;

    k1 = zero_one_l.Process();
    k2 = zero_one_r.Process(); 
    
    switch (mode)
    {
        case 0:
   	    drywet = k1;
	    rev.SetFeedback(k2);
	    pod.led1.Set(k1,0,0);
	    pod.led2.Set(k2,0,0);
	    break;
        case 1:
 	    del.SetDelay(deltime.Process());
	    feedback = k2;
	    pod.led1.Set(0,k1,0);
	    pod.led2.Set(0,k2,0);
	    break;
        case 2:
	    c = cutoff.Process();
	    tone.SetFreq(c);
	    crush.SetCrushRate(crushrate.Process());
	    pod.led1.Set(k1,0,k1);
	    pod.led2.Set(k2,0,k2);
        default:
	    break;
    }	 

    pod.UpdateLeds();
    
    for (size_t i = 0; i < size; i += 2)
    {
        inl = in[i];
        inr = in[i + 1];
	
	switch (mode)
	{
	    case 0:
	        rev.Process(inl, inr, &sigl, &sigr);
		sigl = drywet * sigl + (1 - drywet) * inl;
		sigr = drywet * sigr + (1 - drywet) * inr;
		break;
	    case 1:
	        sigl = sigr = del.Read();
	        del.Write(feedback * sigl + (1 - feedback) * inl);
                break;
	    case 2:
	        inl = crush.Process(inl);
		sigl = sigr = tone.Process(inl);
	    default:
	        break;	      
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
    mode = 0;
    
    //Inits and sample rate
    pod.Init();
    sample_rate = pod.AudioSampleRate();
    rev.Init(sample_rate);
    del.Init();
    crush.Init(sample_rate);
    tone.Init(sample_rate);
    
    //set parameters
    zero_one_l.Init(pod.knob1, 0, 1, zero_one_l.LINEAR);
    zero_one_r.Init(pod.knob2, 0, 1, zero_one_r.LINEAR);
    deltime.Init(pod.knob1, 1, MAX_DELAY, deltime.LOGARITHMIC);
    cutoff.Init(pod.knob1, 500, 20000, cutoff.LOGARITHMIC);
    crushrate.Init(pod.knob2, 100 , 30000, crushrate.LOGARITHMIC);
    
    //reverb parameters
    rev.SetLpFreq(18000.0f);
    rev.SetFeedback(0.85f);

    //delay parameters
    // Set Delay time to 0.75 seconds
    del.SetDelay(sample_rate * 0.75f);

    //crush parameters
    crush.SetBitDepth(2);
    crush.SetCrushRate(10000);
    
    // start callback
    pod.StartAdc();
    pod.StartAudio(AudioCallback);

    while(1) {}
}
