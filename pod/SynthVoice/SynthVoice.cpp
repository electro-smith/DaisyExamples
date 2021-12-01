#include "daisysp.h"
#include "daisy_pod.h"

using namespace daisysp;
using namespace daisy;

static DaisyPod   pod;
static Oscillator osc, lfo;
static MoogLadder flt;
static AdEnv      ad;
static Parameter  pitchParam, cutoffParam, lfoParam;

int   wave, mode;
float vibrato, oscFreq, lfoFreq, lfoAmp, attack, release, cutoff;
float oldk1, oldk2, k1, k2;
bool  selfCycle;

void ConditionalParameter(float  oldVal,
                          float  newVal,
                          float &param,
                          float  update);

void Controls();

void NextSamples(float &sig)
{
    float ad_out = ad.Process();
    vibrato      = lfo.Process();

    osc.SetFreq(oscFreq + vibrato);

    sig = osc.Process();
    sig = flt.Process(sig);
    sig *= ad_out;
}

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    Controls();

    for(size_t i = 0; i < size; i += 2)
    {
        float sig;
        NextSamples(sig);

        // left out
        out[i] = sig;

        // right out
        out[i + 1] = sig;
    }
}

int main(void)
{
    // Set global variables
    float sample_rate;
    mode    = 0;
    vibrato = 0.0f;
    oscFreq = 1000.0f;
    oldk1 = oldk2 = 0;
    k1 = k2   = 0;
    attack    = .01f;
    release   = .2f;
    cutoff    = 10000;
    lfoAmp    = 1.0f;
    lfoFreq   = 0.1f;
    selfCycle = false;

    //Init everything
    pod.Init();
    pod.SetAudioBlockSize(4);
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
    ad.SetTime(ADENV_SEG_ATTACK, 0.01);
    ad.SetTime(ADENV_SEG_DECAY, .2);
    ad.SetMax(1);
    ad.SetMin(0);
    ad.SetCurve(0.5);

    //set parameter parameters
    cutoffParam.Init(pod.knob1, 100, 20000, cutoffParam.LOGARITHMIC);
    pitchParam.Init(pod.knob2, 50, 5000, pitchParam.LOGARITHMIC);
    lfoParam.Init(pod.knob1, 0.25, 1000, lfoParam.LOGARITHMIC);

    // start callback
    pod.StartAdc();
    pod.StartAudio(AudioCallback);

    while(1) {}
}

//Updates values if knob had changed
void ConditionalParameter(float  oldVal,
                          float  newVal,
                          float &param,
                          float  update)
{
    if(abs(oldVal - newVal) > 0.00005)
    {
        param = update;
    }
}


//Controls Helpers
void UpdateEncoder()
{
    wave += pod.encoder.RisingEdge();
    wave %= osc.WAVE_POLYBLEP_TRI;

    //skip ramp since it sounds like saw
    if(wave == 3)
    {
        wave = 4;
    }

    osc.SetWaveform(wave);

    mode += pod.encoder.Increment();
    mode = (mode % 3 + 3) % 3;
}

void UpdateKnobs()
{
    k1 = pod.knob1.Process();
    k2 = pod.knob2.Process();

    switch(mode)
    {
        case 0:
            ConditionalParameter(oldk1, k1, cutoff, cutoffParam.Process());
            ConditionalParameter(oldk2, k2, oscFreq, pitchParam.Process());
            flt.SetFreq(cutoff);
            break;
        case 1:
            ConditionalParameter(oldk1, k1, attack, pod.knob1.Process());
            ConditionalParameter(oldk2, k2, release, pod.knob2.Process());
            ad.SetTime(ADENV_SEG_ATTACK, attack);
            ad.SetTime(ADENV_SEG_DECAY, release);
            break;
        case 2:
            ConditionalParameter(oldk1, k1, lfoFreq, lfoParam.Process());
            ConditionalParameter(oldk2, k2, lfoAmp, pod.knob2.Process());
            lfo.SetFreq(lfoFreq);
            lfo.SetAmp(lfoAmp * 100);
        default: break;
    }
}

void UpdateLeds()
{
    pod.led1.Set(mode == 2, mode == 1, mode == 0);
    pod.led2.Set(0, selfCycle, selfCycle);

    oldk1 = k1;
    oldk2 = k2;

    pod.UpdateLeds();
}

void UpdateButtons()
{
    if(pod.button1.RisingEdge() || (selfCycle && !ad.IsRunning()))
    {
        ad.Trigger();
    }

    if(pod.button2.RisingEdge())
    {
        selfCycle = !selfCycle;
    }
}

void Controls()
{
    pod.ProcessAnalogControls();
    pod.ProcessDigitalControls();

    UpdateEncoder();

    UpdateKnobs();

    UpdateLeds();

    UpdateButtons();
}
