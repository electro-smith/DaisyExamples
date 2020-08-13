#include "daisysp.h"
#include "daisy_petal.h"

// Set max delay time to 0.75 of samplerate.
#define MAX_DELAY static_cast<size_t>(48000 * 2.5f)
enum 
{
    REV,
    DEL,
    CRU,
    WAH,
    FX_LAST,
};


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
bool effectOn[4];

//Helper functions
void Controls();

void GetReverbSample(float &inl, float &inr);
void GetDelaySample(float &inl, float &inr);
void GetCrushSample(float &inl, float &inr);
void GetWahSample(float &inl, float &inr);
void UpdateLeds(float k1, float k2);
  
void AudioCallback(float *in, float *out, size_t size)
{
    float inl, inr;

    Controls();
    
    //audio
    for (size_t i = 0; i < size; i += 2)
    {
        inl = in[i];
        inr = in[i + 1];
	
        GetCrushSample(inl, inr);
        GetWahSample(inl, inr);
		GetDelaySample(inl, inr);		
        GetReverbSample(inl, inr);
	
		// left out
		out[i] = inl;
	
		// right out
		out[i+1] = inr;
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
    deltime.Init(petal.knob[2], sample_rate * .05, MAX_DELAY, deltime.LOGARITHMIC);
    cutoffParam.Init(petal.knob[2], 500, 20000, cutoffParam.LOGARITHMIC);
    crushrate.Init(petal.knob[3], 1, 50, crushrate.LOGARITHMIC);
    
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
	
	effectOn[0] = effectOn[1] = effectOn[2] = effectOn[3] = false;
	
    // start callback
    petal.StartAdc();
    petal.StartAudio(AudioCallback);

    while(1) 
	{
		float a, b;
        a = petal.GetKnobValue(petal.KNOB_1);
        b = petal.GetKnobValue(petal.KNOB_2);
        UpdateLeds(a,b);
        dsy_system_delay(6);
	}
}

void UpdateKnobs(float &k1, float &k2)
{
    k1 = petal.knob[2].Process();
    k2 = petal.knob[3].Process(); 
    
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
    mode = (mode % FX_LAST + FX_LAST) % FX_LAST;
}

void UpdateLeds(float k1, float k2)
{
	petal.ClearLeds();
	
	for (int i = 0; i < 4; i++)
	{
		petal.SetFootswitchLed((DaisyPetal::FootswitchLed)i, effectOn[i]);
	}
	
	petal.SetRingLed(static_cast<DaisyPetal::RingLed>(mode * 2), k1, 0.f, k1);
    petal.SetRingLed(static_cast<DaisyPetal::RingLed>((mode * 2) + 1), k2, 0.f, k2);
    
    petal.UpdateLeds();
}

void UpdateSwitches()
{
	//turn the effect on or off if a footswitch is pressed
	for (int i = 0; i < 4; i++)
	{
		effectOn[i] = petal.switches[i].RisingEdge() ? !effectOn[i] : effectOn[i];
	}
}

void Controls()
{
    float k1, k2;
    delayTarget = feedback = drywet = 0;
    
    petal.UpdateAnalogControls();
    petal.DebounceControls();

    UpdateKnobs(k1, k2);

    UpdateEncoder();
	
	UpdateSwitches();
}

void GetReverbSample(float &inl, float &inr)
{
	float outl, outr;
    rev.Process(inl, inr, &outl, &outr);
    inl = drywet * outl + (1 - drywet) * inl;
    inr = drywet * outr + (1 - drywet) * inr;
}

void GetDelaySample(float &inl, float &inr)
{
    fonepole(currentDelay, delayTarget, .00007f);
    delr.SetDelay(currentDelay);
    dell.SetDelay(currentDelay);
    float outl = dell.Read();
    float outr = delr.Read();
    
    dell.Write((feedback * outl) + inl);
    inl = (feedback * outl) + ((1.0f - feedback) * inl);
    
    delr.Write((feedback * outr) + inr);
    inr = (feedback * outr) + ((1.0f - feedback) * inr);
}

void GetCrushSample(float &inl, float &inr)
{
    crushcount++;
    crushcount %= crushmod;
    if (crushcount == 0)
    {
		crushsr = inr;
		crushsl = inl;
    }
    inl = tone.Process(crushsl);
    inr = tone.Process(crushsr);
}

void GetWahSample(float &inl, float &inr)
{
	inl = wah[0].Process(inl);
	inr = wah[1].Process(inr);
}
