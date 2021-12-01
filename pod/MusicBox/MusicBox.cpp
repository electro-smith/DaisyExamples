#include "daisy_pod.h"
#include "daisysp.h"
#include <math.h>

using namespace daisy;
using namespace daisysp;

DaisyPod   hw;
Oscillator osc;
Svf        filt;
ReverbSc   verb;
AdEnv      env;
Parameter  p_xf, p_vamt, p_dec, p_vtime;

const static float scale[7] = {0, 2, 4, 5, 7, 9, 11};

static float get_new_note()
{
    int32_t val, oct, idx;
    val = myrand() % 14;
    oct = val / 7;
    idx = val % 7;
    return scale[idx] + (12 * oct);
}

static float freq;
float        sig, rawsig, filtsig, sendsig, wetvl, wetvr;
float        xf, vamt, dec, time;
static void  audio(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
    hw.ProcessDigitalControls();
    if(hw.button1.RisingEdge())
    {
        freq = mtof(48.0f + get_new_note());
        osc.SetFreq(freq);
        env.SetTime(ADENV_SEG_DECAY, dec);
        env.Trigger();
    }

    // Get Parameters
    xf   = p_xf.Process();
    vamt = p_vamt.Process();
    dec  = p_dec.Process();

    // Audio Loop
    for(size_t i = 0; i < size; i += 2)
    {
        // Process
        rawsig = osc.Process();
        sig    = rawsig * env.Process();
        filt.Process(sig);
        filtsig = filt.Low();
        sendsig = filtsig * vamt;
        verb.Process(sendsig, sendsig, &wetvl, &wetvr);
        out[i]     = (filtsig + (wetvl)) * 0.707f;
        out[i + 1] = (filtsig + (wetvr)) * 0.707f;
    }
}

void InitSynth(float samplerate)
{
    // Synth Parameters.
    p_xf.Init(hw.knob1, 10.0f, 12000.0f, Parameter::LOGARITHMIC);
    p_dec.Init(hw.knob1, 0.2f, 5.0f, Parameter::EXPONENTIAL);
    p_vamt.Init(hw.knob2, 0.0f, 1.0f, Parameter::LINEAR);
    p_vtime.Init(hw.knob2, 0.4f, 0.95f, Parameter::LINEAR);
    dec = 0.62;
    // Init Osc and Nse
    osc.Init(samplerate);
    osc.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    osc.SetFreq(100.0f);
    osc.SetAmp(.5f);
    env.Init(samplerate);
    env.SetCurve(-15.0f);
    env.SetTime(ADENV_SEG_ATTACK, 0.002f);
    env.SetTime(ADENV_SEG_DECAY, 2.6f);
    env.SetMax(1.f);
    env.SetMin(0.f);
    filt.Init(samplerate);
    filt.SetRes(0.5f);
    filt.SetDrive(0.8f);
    filt.SetFreq(2400.0f);
    verb.Init(samplerate);
    verb.SetFeedback(0.87);
    verb.SetLpFreq(10000.0f);
}

int main(void)
{
    float samplerate;
    // Init
    hw.Init();
    hw.SetAudioBlockSize(4);
    samplerate = hw.AudioSampleRate();
    InitSynth(samplerate);
    // Start Callbacks
    hw.StartAdc();
    hw.StartAudio(audio);
    while(1) {}
}
