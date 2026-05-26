// Version 1.1.0

#include "daisy_petal.h"
#include "daisysp.h"
#include "terrarium.h"
#include "DelayUnit.h"

using namespace daisy;
using namespace daisysp;
using namespace terrarium;

DaisyPetal petal;

DSY_SDRAM_BSS DelayLine<float, MAX_DELAY> delayBuffer1;
DSY_SDRAM_BSS DelayLine<float, MAX_DELAY> delayBuffer2;

DelayUnit delay1;
DelayUnit delay2;

Parameter param_feedback_1, param_feedback_2;
Parameter param_mix_1, param_mix_2;
Parameter param_time_1, param_time_2;

bool bypassDelay1 = false;
bool bypassDelay2 = false;

Led led1, led2;

float samplerate;
uint32_t last_flash = 0;
bool flash_state = false;

Tone hpf;


void InitControls()
{
    param_time_1.Init(petal.knob[Terrarium::KNOB_1], 0.01f, 5.0f, Parameter::EXPONENTIAL);
    param_feedback_1.Init(petal.knob[Terrarium::KNOB_2], 0.0f, 1.0f, Parameter::LINEAR);
    param_mix_1.Init(petal.knob[Terrarium::KNOB_3], 0.0f, 1.0f, Parameter::LINEAR);

    param_time_2.Init(petal.knob[Terrarium::KNOB_4], 0.01f, 5.0f, Parameter::EXPONENTIAL);
    param_feedback_2.Init(petal.knob[Terrarium::KNOB_5], 0.0f, 1.0f, Parameter::LINEAR);
    param_mix_2.Init(petal.knob[Terrarium::KNOB_6], 0.0f, 1.0f, Parameter::LINEAR);

    led1.Init(petal.seed.GetPin(terrarium::Terrarium::LED_1), false);
    led2.Init(petal.seed.GetPin(terrarium::Terrarium::LED_2), false);
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    float delay_time_1 = param_time_1.Process();
    float feedback_1 = param_feedback_1.Process();
    float mix_1 = param_mix_1.Process();

    float ratio_knob = param_time_2.Process();
    float delay_time_2 = delay2.SnapToRatio(delay_time_1, ratio_knob);
    float feedback_2 = param_feedback_2.Process();
    float mix_2 = param_mix_2.Process();
    

    delay1.delayTarget = delay_time_1 * samplerate;
    delay1.feedback = feedback_1;

    delay2.delayTarget = delay_time_2 * samplerate;
    delay2.feedback = feedback_2;

    for (size_t i = 0; i < size; i++)
    {
        float dry = in[0][i];
        float delayed1 = delay1.Process(dry, delay_time_1 * samplerate);
        float delayed2 = delay2.Process(dry, delay_time_2 * samplerate);

        float mixed1 = (1.0f - mix_1) * dry + mix_1 * delayed1;
        float mixed2 = (1.0f - mix_2) * dry + mix_2 * delayed2;

        float output = dry;
        if (!bypassDelay1)
            output = mixed1;
        if (!bypassDelay2)
            output = (output + mixed2) * 0.5f;

        out[0][i] = out[1][i] = output;
    }

    // LED Flash Timing based on Delay 1 Time
    uint32_t now = System::GetNow();
    if (!bypassDelay1 && (now - last_flash) >= static_cast<uint32_t>(delay_time_1 * 1000))
    {
        flash_state = !flash_state;
        led1.Set(flash_state ? 1.0f : 0.0f);
        last_flash = now;
    }
    else if (bypassDelay1)
    {
        led1.Set(0.0f);
    }

    led1.Update();
    led2.Set(!bypassDelay2 ? 1.0f : 0.0f);
    led2.Update();
}

int main(void)
{
    petal.Init();
    samplerate = petal.AudioSampleRate();
    hpf.Init(samplerate);
    hpf.SetFreq(80.0f); // Cut off sub-bass

    delay1.Init(samplerate, delayBuffer1);
    delay2.Init(samplerate, delayBuffer2);
    InitControls();

    petal.StartAdc();
    petal.StartAudio(AudioCallback);

    while (1)
    {
        petal.ProcessAnalogControls();
        petal.ProcessDigitalControls();

        if (petal.switches[Terrarium::FOOTSWITCH_1].RisingEdge())
            bypassDelay1 = !bypassDelay1;

        if (petal.switches[Terrarium::FOOTSWITCH_2].RisingEdge())
            bypassDelay2 = !bypassDelay2;

        System::Delay(10);
    }
}
