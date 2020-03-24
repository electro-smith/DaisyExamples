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
parameter p_xf, p_vamt, p_dec, p_vtime;

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
static void  audio(float *in, float *out, size_t size)
{
    hw.DebounceControls();
    if(hw.button1.RisingEdge())
    {
        freq = mtof(48.0f + get_new_note());
        osc.SetFreq(freq);
//        env.SetTime(ADENV_SEG_DECAY, dec);
        env.Trigger();
    }

    hw.UpdateKnobs();
    float val = hw.GetKnobValue(DaisyPod::KNOB_1);
    hw.led2.Set(val, val, val);
//	hw.led2.Update(); // Handles the PWM

    // Audio Loop
    for(size_t i = 0; i < size; i += 2)
    {
        // Get Parameters
//        xf   = p_xf.process();
//        vamt = p_vamt.process();
//        dec  = p_dec.process();

        // Process
        rawsig = osc.Process();
        sig    = rawsig * env.Process();
        filt.Process(sig);
        filtsig = filt.Low();
        sendsig = filtsig * vamt;
        //        verb.Process(sendsig, sendsig, &wetvl, &wetvr);
        wetvl      = 0.0f;
        wetvr      = 0.0f;
        out[i]     = (filtsig + (wetvl)) * 0.707f;
        out[i + 1] = (filtsig + (wetvr)) * 0.707f;
    }
}

void InitSynth(float samplerate)
{
    // Synth Parameters.
//    p_xf.init(hw.knob1, 10.0f, 12000.0f, parameter::LOG);
//    p_dec.init(hw.knob1, 0.2f, 5.0f, parameter::EXP);
//    p_vamt.init(hw.knob2, 0.0f, 1.0f, parameter::LINEAR);
//    p_vtime.init(hw.knob2, 0.4f, 0.95f, parameter::LINEAR);
    dec = 0.62;
    // Init Osc and Nse
    osc.Init(samplerate);
    osc.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    osc.SetFreq(100.0f);
    osc.SetAmp(0.5f);
    env.Init(samplerate);
    env.SetCurve(-15.0f);
    env.SetTime(ADENV_SEG_ATTACK, 0.002f);
    env.SetTime(ADENV_SEG_DECAY, 2.6f);
    filt.Init(samplerate);
    filt.SetRes(0.5f);
    filt.SetDrive(0.8f);
    filt.SetFreq(2400.0f);
//    verb.Init(samplerate);
//    verb.SetFeedback(0.87);
//    verb.SetLpFreq(10000.0f);
}

int main(void)
{
	float samplerate;
	// Init
    hw.Init();
	samplerate = hw.AudioSampleRate();
    InitSynth(samplerate);
    // Start Callbacks
	hw.StartAdc();
	hw.StartAudio(audio);


    while(1)
    {
        //  Blink the Seed's on board LED.
//        hw.DelayMs(250);
//        dsy_gpio_toggle(&hw.seed.led);
    }
}
