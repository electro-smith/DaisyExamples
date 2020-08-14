#include "daisysp.h"
#include "daisy_petal.h"

// Set max delay time to 0.75 of samplerate.
#define MAX_DELAY static_cast<size_t>(48000 * 2.5f)

using namespace daisysp;
using namespace daisy;

static DaisyPetal  petal;

static ReverbSc  rev;
static DelayLine <float, MAX_DELAY>  DSY_SDRAM_BSS dell;
static DelayLine <float, MAX_DELAY>  DSY_SDRAM_BSS delr;
static Autowah wah[2];
static Parameter deltime, reverbLpParam, crushrate;

float currentDelay, feedback, delayTarget, cutoff, dryWet;

int crushmod, crushcount;
float crushsl, crushsr;
bool effectOn[4];

//Helper functions
void Controls();

void GetReverbSample(float &inl, float &inr);
void GetDelaySample(float &inl, float &inr);
void GetCrushSample(float &inl, float &inr);
void GetWahSample(float &inl, float &inr);
void UpdateLeds();
  
void AudioCallback(float *in, float *out, size_t size)
{
    Controls();
    
    //audio
    for (size_t i = 0; i < size; i += 2)
    {
        float sigl = in[i];
        float sigr = in[i + 1];
		
		if(effectOn[0])
			GetCrushSample(sigl, sigr);
		if(effectOn[1])
			GetWahSample(sigl, sigr);
		if(effectOn[2])
			GetDelaySample(sigl, sigr);		
		if(effectOn[3])
			GetReverbSample(sigl, sigr);
	
		out[i]   = sigl * dryWet + in[i] * (1 - dryWet);
		out[i+1] = sigr * dryWet + in[i + 1] * (1 - dryWet);
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
    
    //set parameters
	reverbLpParam.Init(petal.knob[0], 20, 10000, Parameter::LOGARITHMIC);
    deltime.Init(petal.knob[2], sample_rate * .05, MAX_DELAY, deltime.LOGARITHMIC);
    crushrate.Init(petal.knob[4], 1, 50, crushrate.LOGARITHMIC);
    
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
        UpdateLeds();
        dsy_system_delay(6);
	}
}

void UpdateKnobs()
{
	rev.SetLpFreq(reverbLpParam.Process());
	rev.SetFeedback(petal.knob[1].Process());

	delayTarget = deltime.Process();
	feedback = petal.knob[3].Process();

	crushmod = (int) crushrate.Process();

	wah[0].SetWah(petal.knob[5].Process());
	wah[1].SetWah(petal.knob[5].Process());
}

void UpdateEncoder()
{
	dryWet += petal.encoder.Increment() * .05f;
	dryWet = dryWet > 1.0f ? 1.0f : dryWet;
	dryWet = dryWet < 0.0f ? 0.0f : dryWet;
}

void UpdateLeds()
{
	petal.ClearLeds();
	
	for (int i = 0; i < 4; i++)
	{
		petal.SetFootswitchLed((DaisyPetal::FootswitchLed)i, effectOn[i]);
	}
	    
	for (int i = 0; i < 8; i++)
	{
		petal.SetRingLed((DaisyPetal::RingLed)i, dryWet, dryWet, 0.f);
	}
		
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
    petal.UpdateAnalogControls();
    petal.DebounceControls();

    UpdateKnobs();

    UpdateEncoder();
	
	UpdateSwitches();
}

void GetReverbSample(float &inl, float &inr)
{
    rev.Process(inl, inr, &inl, &inr);
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
}

void GetWahSample(float &inl, float &inr)
{
	inl = wah[0].Process(inl);
	inr = wah[1].Process(inr);
}