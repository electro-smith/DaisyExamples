// Dual Delay Pedal - Version 1.1.5
// Rerverse delay is working, controls are not working
// Two 5 second delays with almost infinite feedback. 

#include "daisysp.h"
#include "daisy_petal.h"
#include "terrarium.h"
#include "DelayUnitReverse.h"

using namespace daisy;
using namespace daisysp;
using namespace terrarium;

DaisyPetal petal;
Parameter param_feedback_1, param_mix_1, param_feedback_2, param_mix_2;
Led led1, led2;
DelayUnitReverse delay1_rev, delay2_rev;
CrossFade cfade;

DSY_SDRAM_BSS daisysp::DelayLineReverse<float, MAX_DELAY> reverseDelayBuffer1;
DSY_SDRAM_BSS daisysp::DelayLineReverse<float, MAX_DELAY> reverseDelayBuffer2;

float samplerate;
bool bypassDelay1 = false;
bool bypassDelay2 = false;

///////////////////////////////////////////////////////////////
// Init Params

void InitParams()
{
    // Initialize parameters for Delay1 -----------------------
    param_feedback_1.Init(petal.knob[Terrarium::KNOB_2], 0.0f, 1.0f, Parameter::LINEAR);
    param_mix_1.Init(petal.knob[Terrarium::KNOB_3], 0.0f, 1.0f, Parameter::LINEAR);
    // Initialize parameters for Delay2 -----------------------
    param_feedback_2.Init(petal.knob[Terrarium::KNOB_5], 0.0f, 1.0f, Parameter::LINEAR);
    param_mix_2.Init(petal.knob[Terrarium::KNOB_6], 0.0f, 1.0f, Parameter::LINEAR);
}

///////////////////////////////////////////////////////////////
// Init LEDs 

void InitLeds()
{
    led1.Init(petal.seed.GetPin(Terrarium::LED_1), false);
    led2.Init(petal.seed.GetPin(Terrarium::LED_2), false);
}

///////////////////////////////////////////////////////////////
// AudioCallback

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    // Delay 1 ------------------------------------------------
    float knob1 = 1.0f - petal.knob[Terrarium::KNOB_1].Process();
    // 0 - 0.5 -> 0 - 1s delay time, 0.5 - 1.0 -> 1.0 - 5.0s delay time
    float delay1_time = knob1 < 0.5f ? fmap(knob1 * 2.0f, 0.1f, 1.0f)
                                 : fmap((knob1 - 0.5f) * 2.0f, 1.0f, 5.0f);  

    float delay1_samples = delay1_time * samplerate;
    float delay1_mix = param_mix_1.Process();

    delay1_rev.delayTarget = delay1_samples;
    delay1_rev.feedback = param_feedback_1.Process();

    // Delay 2 ------------------------------------------------
    float knob4 = 1.0f - petal.knob[Terrarium::KNOB_4].Process();
    // 0 - 0.5 -> 0 - 1s delay time, 0.5 - 1.0 -> 1.0 - 5.0s delay time
    float delay2_time = knob4 < 0.5f ? fmap(knob4 * 2.0f, 0.1f, 1.0f)
                                     : fmap((knob4 - 0.5f) * 2.0f, 1.0f, 5.0f);
    
    float delay2_samples = delay2_time * samplerate;
    float delay2_mix = param_mix_2.Process();
    
    delay2_rev.delayTarget = delay2_samples;
    delay2_rev.feedback = param_feedback_2.Process();

    // ffective wet signal from both delays:
    float wetScale1 = delay1_mix; // or any mapping you prefer
    float wetScale2 = delay2_mix; // or combine them differently

    // In the processing loop:
    for (size_t i = 0; i < size; i++)
    {
        float dry = in[0][i];

        float d1 = bypassDelay1 ? delay1_rev.Process(0.0f) : delay1_rev.Process(dry);
        float d2 = bypassDelay2 ? delay2_rev.Process(0.0f) : delay2_rev.Process(dry);
        
        // Sum the wet outputs using the mix parameters:
        float wet = d1 * wetScale1 + d2 * wetScale2;
        
        // Use the CrossFade object to blend dry and wet signals.
        // This ensures constant power blending based on effectiveMix.
        float finalMix = cfade.Process(dry, wet);
        
        out[0][i] = out[1][i] = finalMix;
    }


}

///////////////////////////////////////////////////////////////
// Main 

int main(void)
{
    // Initialize patch ---------------------------------------
    petal.Init();
    samplerate = petal.AudioSampleRate();

    // Initialize Params --------------------------------------
    InitParams();
    InitLeds();

    // Initialize CrossFade -----------------------------------
    cfade.Init();
    cfade.SetCurve(CROSSFADE_CPOW);

    // Initialize Delay units ---------------------------------
    delay1_rev.Init(samplerate, reverseDelayBuffer1);
    delay2_rev.Init(samplerate, reverseDelayBuffer2);

    // Start Adc and start AudioCallback ----------------------
    petal.StartAdc();
    petal.StartAudio(AudioCallback);

    while (1)
    {
        petal.ProcessAnalogControls();
        petal.ProcessDigitalControls();

        // Use SWITCH_1 to control reverse mode for delay1:
        delay1_rev.reverse = petal.switches[Terrarium::SWITCH_1].Pressed();
        // And similarly for delay2 if desired:
        delay2_rev.reverse = petal.switches[Terrarium::SWITCH_2].Pressed();

        // 
        if (petal.switches[Terrarium::FOOTSWITCH_1].RisingEdge())
        {
            bypassDelay1 = !bypassDelay1;
        }

        if (petal.switches[Terrarium::FOOTSWITCH_2].RisingEdge())
        {
            bypassDelay2 = !bypassDelay2;
        }

        // LED1 indicates if effect is enabled
        led1.Set(!bypassDelay1 ? 1.0f : 0.0f);
        led2.Set(!bypassDelay2 ? 1.0f : 0.0f);
        
        led1.Update();
        led2.Update();

        System::Delay(10);
    }
}
