#include "daisysp.h"
#include "daisy_petal.h"

// Set max delay time to 0.75 of samplerate.
#define MAX_DELAY static_cast<size_t>(48000 * 2.5f)

using namespace daisysp;
using namespace daisy;

static DaisyPetal petal;

static ReverbSc                                  rev;
static DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS dell;
static DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delr;
static Autowah                                   wah[2];
static Parameter deltime, reverbLpParam, crushrate;

float currentDelay, feedback, delayTarget, cutoff, dryWet[5];

int   crushmod, crushcount;
float crushl, crushr;
bool  effectOn[4];

//Helper functions
void Controls();
void UpdateLeds();

void GetCrushSample(float &inl, float &inr);
void GetWahSample(float &inl, float &inr);
void GetDelaySample(float &inl, float &inr);
void GetReverbSample(float &inl, float &inr);


enum effectTypes
{
    CRUSH,
    WAH,
    DEL,
    REV,
    ALL,
    LAST,
};
effectTypes dryWetMode;

void GetSample(float &inl, float &inr, effectTypes type)
{
    switch(type)
    {
        case CRUSH: GetCrushSample(inl, inr); break;
        case WAH: GetWahSample(inl, inr); break;
        case DEL: GetDelaySample(inl, inr); break;
        default: break;
    }
}

void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
    Controls();

    //audio
    for(size_t i = 0; i < size; i += 2)
    {
        float sigl = in[i];
        float sigr = in[i + 1];

        for(int eff = 0; eff < REV; eff++)
        {
            float oldSigL = sigl;
            float oldSigR = sigr;

            if(effectOn[eff])
            {
                GetSample(sigl, sigr, (effectTypes)eff);
            }

            sigl = sigl * dryWet[eff] + oldSigL * (1 - dryWet[eff]);
            sigr = sigr * dryWet[eff] + oldSigR * (1 - dryWet[eff]);
        }

        float verbl = sigl * dryWet[REV];
        float verbr = sigr * dryWet[REV];
        GetReverbSample(verbl, verbr);

        out[i]     = sigl * dryWet[ALL] + in[i] * (1 - dryWet[ALL]);
        out[i + 1] = sigr * dryWet[ALL] + in[i + 1] * (1 - dryWet[ALL]);

        if(effectOn[REV])
        {
            out[i] += verbl;
            out[i + 1] += verbr;
        }
    }
}

int main(void)
{
    // initialize petal hardware and oscillator daisysp module
    float sample_rate;

    //Inits and sample rate
    petal.Init();
    petal.SetAudioBlockSize(4);
    sample_rate = petal.AudioSampleRate();
    rev.Init(sample_rate);
    dell.Init();
    delr.Init();

    //set parameters
    reverbLpParam.Init(petal.knob[0], 400, 22000, Parameter::LOGARITHMIC);
    deltime.Init(
        petal.knob[2], sample_rate * .05, MAX_DELAY, deltime.LOGARITHMIC);
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

    for(int i = 0; i < LAST; i++)
    {
        dryWet[i] = 0.6f;
    }

    dryWetMode = CRUSH;

    // start callback
    petal.StartAdc();
    petal.StartAudio(AudioCallback);

    while(1)
    {
        UpdateLeds();
        System::Delay(6);
    }
}

void UpdateKnobs()
{
    rev.SetLpFreq(reverbLpParam.Process());
    rev.SetFeedback(petal.knob[1].Process());

    delayTarget = deltime.Process();
    feedback    = petal.knob[3].Process();

    crushmod = (int)crushrate.Process();

    wah[0].SetWah(petal.knob[5].Process());
    wah[1].SetWah(petal.knob[5].Process());
}

void UpdateEncoder()
{
    //press
    if(petal.encoder.RisingEdge())
    {
        dryWetMode = (effectTypes)(dryWetMode + 1);
        dryWetMode = (effectTypes)(dryWetMode % LAST);
    }

    //turn
    dryWet[dryWetMode] += petal.encoder.Increment() * .05f;
    dryWet[dryWetMode] = dryWet[dryWetMode] > 1.0f ? 1.0f : dryWet[dryWetMode];
    dryWet[dryWetMode] = dryWet[dryWetMode] < 0.0f ? 0.0f : dryWet[dryWetMode];
}

void UpdateLeds()
{
    petal.ClearLeds();

    //footswitch leds
    petal.SetFootswitchLed((DaisyPetal::FootswitchLed)0, effectOn[REV]);
    petal.SetFootswitchLed((DaisyPetal::FootswitchLed)1, effectOn[DEL]);
    petal.SetFootswitchLed((DaisyPetal::FootswitchLed)2, effectOn[CRUSH]);
    petal.SetFootswitchLed((DaisyPetal::FootswitchLed)3, effectOn[WAH]);


    //ring leds
    int32_t whole;
    float   frac;
    whole = (int32_t)(dryWet[dryWetMode] / .125f);
    frac  = dryWet[dryWetMode] / .125f - whole;

    // Set full bright
    for(int i = 0; i < whole; i++)
    {
        petal.SetRingLed(
            static_cast<DaisyPetal::RingLed>(i),
            (dryWetMode == CRUSH || dryWetMode == REV || dryWetMode == ALL)
                * 1.f,                                      //red
            (dryWetMode == WAH || dryWetMode == ALL) * 1.f, //green
            (dryWetMode == DEL || dryWetMode == REV || dryWetMode == ALL)
                * 1.f); //blue
    }

    // Set Frac
    if(whole < 7 && whole > 0)
    {
        petal.SetRingLed(
            static_cast<DaisyPetal::RingLed>(whole - 1),
            (dryWetMode == CRUSH || dryWetMode == REV || dryWetMode == ALL)
                * frac,                                      //red
            (dryWetMode == WAH || dryWetMode == ALL) * frac, //green
            (dryWetMode == DEL || dryWetMode == REV || dryWetMode == ALL)
                * frac); //blue
    }

    petal.UpdateLeds();
}

void UpdateSwitches()
{
    //turn the effect on or off if a footswitch is pressed

    effectOn[REV]
        = petal.switches[0].RisingEdge() ? !effectOn[REV] : effectOn[REV];
    effectOn[DEL]
        = petal.switches[1].RisingEdge() ? !effectOn[DEL] : effectOn[DEL];
    effectOn[CRUSH]
        = petal.switches[2].RisingEdge() ? !effectOn[CRUSH] : effectOn[CRUSH];
    effectOn[WAH]
        = petal.switches[3].RisingEdge() ? !effectOn[WAH] : effectOn[WAH];
}

void Controls()
{
    petal.ProcessAnalogControls();
    petal.ProcessDigitalControls();

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
    if(crushcount == 0)
    {
        crushr = inr;
        crushl = inl;
    }

    inr = crushr;
    inl = crushl;
}

void GetWahSample(float &inl, float &inr)
{
    inl = wah[0].Process(inl);
    inr = wah[1].Process(inr);
}
