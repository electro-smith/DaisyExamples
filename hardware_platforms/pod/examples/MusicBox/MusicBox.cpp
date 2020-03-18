#include "daisy_pod.h"
#include "daisysp.h"
#include <math.h>

using namespace daisy;
using namespace daisysp;

daisy_pod  hw;
Oscillator osc;
Svf        filt;
ReverbSc verb;
AdEnv      env;

parameter p_xf, p_vamt, p_dec, p_vtime;

//static float mtof(float m);

const static float scale[7] = {0, 2, 4, 5, 7, 9, 11};

static float get_new_note()
{
    int32_t val, oct, idx;
    val = rand() % 14;
    oct = val / 7;
    idx = val % 7;
    return scale[idx] + (12 * oct);
}

static void clear_leds()
{
    for (int i = 0; i < LED_LAST; i++)
    {
        dsy_gpio_write(&hw.leds[i], 1);
    }
}


static float freq;
float sig, rawsig, filtsig, sendsig, wetvl, wetvr;
float xf, vamt, dec, time;
static void  audio(float *in, float *out, size_t size)
{
    hw.switches[SW_1].Debounce();
    if(hw.switches[SW_1].RisingEdge())
    {
        freq = mtof(48.0f + get_new_note());
        osc.SetFreq(freq);
        env.SetTime(ADENV_SEG_DECAY, dec);
        env.Trigger();
    }
    clear_leds();
    if(hw.switches[SW_1].Pressed())
    {
        dsy_gpio_write(&hw.leds[LED_2_G], 0);
        dsy_gpio_write(&hw.leds[LED_2_B], 0);
    }
    // Audio Loop
    for(size_t i = 0; i < size; i += 2)
    {
        // Get Parameters
        xf = p_xf.process();
        vamt = p_vamt.process();
        dec = p_dec.process();

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


int main(void)
{
    // Initialize Hardware
    //hw.Init();
    AnalogControl knob1, knob2;
    daisy_pod_init(&hw);
    clear_leds();
    knob1.Init(dsy_adc_get_rawptr(KNOB_1), SAMPLE_RATE);
    knob2.Init(dsy_adc_get_rawptr(KNOB_2), SAMPLE_RATE);
    p_xf.init(knob1, 10.0f, 12000.0f, parameter::LOG);
    p_vamt.init(knob2, 0.0f, 1.0f, parameter::LINEAR);
    p_vtime.init(knob2, 0.4f, 0.95f, parameter::LINEAR);
    p_dec.init(knob1, 0.2f, 5.0f, parameter::EXP);
    dec = 0.02;
    // Init Osc and Nse
    dsy_tim_start();
    osc.Init(SAMPLE_RATE);
    osc.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    osc.SetAmp(0.5f);
    env.Init(SAMPLE_RATE);
    env.SetCurve(-15.0f);
    env.SetTime(ADENV_SEG_ATTACK, 0.002f);
    env.SetTime(ADENV_SEG_DECAY, 0.6f);
    filt.Init(SAMPLE_RATE);
    filt.SetRes(0.5f);
    filt.SetDrive(0.8f);
    filt.SetFreq(2400.0f);
    verb.Init(SAMPLE_RATE);
    verb.SetFeedback(0.87);
    verb.SetLpFreq(10000.0f);
    // Old style still
    dsy_audio_set_blocksize(DSY_AUDIO_INTERNAL, 128);
    dsy_audio_set_callback(DSY_AUDIO_INTERNAL, audio);
    dsy_audio_start(DSY_AUDIO_INTERNAL);
    dsy_adc_start();

//    for(uint16_t i = 0; i < daisy_patch::LED_LAST; i++)
//    {
//        dsy_led_driver_set_led(i, 0.0f);
//    }
    while(1)
    {
        dsy_tim_delay_ms(20);
//        for(uint16_t i = 0; i < daisy_patch::LED_LAST; i++)
//        {
//            dsy_led_driver_set_led(i, param_bright.value());
//        }
    }
}
