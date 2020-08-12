#include "daisysp.h"
#include "daisy_petal.h"

// Set max delay time to 0.75 of samplerate.
#define MAX_DELAY static_cast<size_t>(48000 * 2.5f)
#define REV 0
#define DEL 1
#define CRU 2
#define WAH 3


using namespace daisysp;
using namespace daisy;

static DaisyPetal  petal;

static ReverbSc  rev;
static DelayLine <float, MAX_DELAY>  DSY_SDRAM_BSS dell;
static DelayLine <float, MAX_DELAY>  DSY_SDRAM_BSS delr;
static Tone tone;
static Autowah wah[2];
static Parameter deltime, cutoffParam, crushrate;
int mode = REV;

float currentDelay, feedback, delayTarget, cutoff;

int crushmod, crushcount;
float crushsl, crushsr, drywet;


//Helper functions
void Controls();

void GetReverbSample(float &outl, float &outr, float inl, float inr);
void GetDelaySample(float &outl, float &outr, float inl, float inr);
void GetCrushSample(float &outl, float &outr, float inl, float inr);
void GetWahSample(float &outl, float &outr, float inl, float inr);

  
void AudioCallback(float *in, float *out, size_t size)
{
    float outl, outr, inl, inr;

    Controls();
    
    //audio
    for (size_t i = 0; i < size; i += 2)
    {
        inl = in[i];
        inr = in[i + 1];
	
        switch (mode)
	{
            case REV:
                GetReverbSample(outl, outr, inl, inr);
                break;
            case DEL:
                GetDelaySample(outl, outr, inl, inr);
                break;
            case CRU:
	            GetCrushSample(outl, outr, inl, inr);
                break;
            case WAH:
                GetWahSample(outl, outr, inl, inr);
				break;
			default:
	        outl = outr = 0;
	}
	
	// left out
	out[i] = outl;
	
	// right out
	out[i+1] = outr;
    }
}

int main(void)
{
    // initialize petal hardware and oscillator daisysp module
    float sample_rate;
    
    //Inits and sample rate
    petal.Init();
    sample_rate = petal.AudioSampleRate();
    rev.Init(sample_rate);
    dell.Init();
    delr.Init();
    tone.Init(sample_rate);
    
    //set parameters
    deltime.Init(petal.knob[0], sample_rate * .05, MAX_DELAY, deltime.LOGARITHMIC);
    cutoffParam.Init(petal.knob[0], 500, 20000, cutoffParam.LOGARITHMIC);
    crushrate.Init(petal.knob[1], 1, 50, crushrate.LOGARITHMIC);
    
    //reverb parameters
    rev.SetLpFreq(18000.0f);
    rev.SetFeedback(0.85f);

    //delay parameters
    currentDelay = delayTarget = sample_rate * 0.75f;
    dell.SetDelay(currentDelay);
    delr.SetDelay(currentDelay);
    
	wah[0].Init(sample_rate);
	wah[1].Init(sample_rate);
	wah[0].SetLevel(.8f);
	wah[1].SetLevel(.8f);
	
    // start callback
    petal.StartAdc();
    petal.StartAudio(AudioCallback);

    while(1) {}
}

void UpdateKnobs(float &k1, float &k2)
{
    k1 = petal.knob[0].Process();
    k2 = petal.knob[1].Process(); 
    
    switch (mode)
    {
        case REV:
			drywet = k1;
			rev.SetFeedback(k2);
			break;
        case DEL:
			delayTarget = deltime.Process();
			feedback = k2;
			break;
        case CRU:
			cutoff = cutoffParam.Process();
			tone.SetFreq(cutoff);
			crushmod = (int) crushrate.Process();
			break;
		case WAH:
			wah[0].SetDryWet(petal.knob[2].Process() * 100);
			wah[0].SetWah(petal.knob[3].Process());
			wah[1].SetDryWet(petal.knob[2].Process() * 100);
			wah[1].SetWah(petal.knob[3].Process());
			break;
    }
}

void UpdateEncoder()
{
    mode = mode + petal.encoder.Increment();
    mode = (mode % 3 + 3) % 3;
}

void UpdateLeds(float k1, float k2)
{
	for(int i = 0; i < 8; i++)
	{
	    petal.ring_led[i].Set(0.f, 0.f, 0.f);
	}
	
    petal.ring_led[mode * 2].Set(k1, 0.f, k1);
    petal.ring_led[(mode * 2) + 1].Set(k2, 0.f, k2);
    
    petal.UpdateLeds();
}

void Controls()
{
    float k1, k2;
    delayTarget = feedback = drywet = 0;
    
    petal.UpdateAnalogControls();
    petal.DebounceControls();

    UpdateKnobs(k1, k2);

    UpdateEncoder();

    UpdateLeds(k1, k2);
}

void GetReverbSample(float &outl, float &outr, float inl, float inr)
{
    rev.Process(inl, inr, &outl, &outr);
    outl = drywet * outl + (1 - drywet) * inl;
    outr = drywet * outr + (1 - drywet) * inr;
}

void GetDelaySample(float &outl, float &outr, float inl, float inr)
{
    fonepole(currentDelay, delayTarget, .00007f);
    delr.SetDelay(currentDelay);
    dell.SetDelay(currentDelay);
    outl = dell.Read();
    outr = delr.Read();
    
    dell.Write((feedback * outl) + inl);
    outl = (feedback * outl) + ((1.0f - feedback) * inl);
    
    delr.Write((feedback * outr) + inr);
    outr = (feedback * outr) + ((1.0f - feedback) * inr);
}

void GetCrushSample(float &outl, float &outr, float inl, float inr)
{
    crushcount++;
    crushcount %= crushmod;
    if (crushcount == 0)
    {
	crushsr = inr;
	crushsl = inl;
    }
    outl = tone.Process(crushsl);
    outr = tone.Process(crushsr);
}

void GetWahSample(float &outl, float &outr, float inl, float inr)
{
	outl = wah[0].Process(inl);
	outr = wah[1].Process(inr);
}
