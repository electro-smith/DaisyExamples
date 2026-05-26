// Dual Delay Pedal - Version 1.0.3



#include "daisysp.h"
#include "daisy_petal.h"
#include "terrarium.h"

using namespace daisy;
using namespace daisysp;
using namespace terrarium;

DaisyPetal petal;
Parameter param_feedback, param_mix;
Led led1, led2;

Tone lp_filter;

float samplerate;
bool effect_enabled = true;

constexpr size_t MAX_DELAY = 48000 * 5; // 5 seconds max delay

DSY_SDRAM_BSS DelayLine<float, MAX_DELAY> delayBuffer;

struct DelayUnit {
    DelayLine<float, MAX_DELAY>* buffer;
    float currentDelay = 0.5f;
    float delayTarget = 0.5f;
    float feedback = 0.5f;
    Tone lp_filter;

    void Init(float samplerate, DelayLine<float, MAX_DELAY>& buf)
    {
        buffer = &buf;
        buffer->Init();
        lp_filter.Init(samplerate);
        lp_filter.SetFreq(4000.0f);
    }

    float Process(float in, float delay_samples)
    {
        fonepole(currentDelay, delayTarget, 0.0002f);
        buffer->SetDelay(currentDelay);

        float delayed = buffer->Read();
        float input = in + delayed * feedback;
        float filtered = lp_filter.Process(input);
        filtered = fminf(fmaxf(filtered, -1.0f), 1.0f);
        buffer->Write(filtered);

        return delayed;
    }
};

DelayUnit delay;

void InitParams()
{
    param_feedback.Init(petal.knob[Terrarium::KNOB_2], 0.0f, 1.0f, Parameter::LINEAR);
    param_mix.Init(petal.knob[Terrarium::KNOB_3], 0.0f, 1.0f, Parameter::LINEAR);
}

void InitLeds()
{
    led1.Init(petal.seed.GetPin(Terrarium::LED_1), false);
    led2.Init(petal.seed.GetPin(Terrarium::LED_2), false);
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    float knob = petal.knob[Terrarium::KNOB_1].Process();
    float delay_time = knob < 0.5f
        ? fmap(knob * 2.0f, 0.01f, 1.0f)
        : fmap((knob - 0.5f) * 2.0f, 1.0f, 5.0f);
    
    float mix = param_mix.Process();
    float delay_samples = delay_time * samplerate;
    
    delay.delayTarget = delay_samples;
    delay.feedback = param_feedback.Process();

    for (size_t i = 0; i < size; i++)
    {
        float dry = in[0][i];
        
        float delayed = delay.Process(dry, delay_samples);

        float wet = delayed * 0.3f;
        float mixed = (1.0f - mix) * dry + mix * wet;

        float output = effect_enabled ? mixed : dry;
        out[0][i] = out[1][i] = output;
    }
}

int main(void)
{
    petal.Init();
    lp_filter.Init(petal.AudioSampleRate());
    lp_filter.SetFreq(4000.0f);
    samplerate = petal.AudioSampleRate();

    InitParams();
    InitLeds();

    delay.Init(samplerate, delayBuffer);
    petal.StartAdc();
    petal.StartAudio(AudioCallback);

    while (1)
    {
        petal.ProcessAnalogControls();
        petal.ProcessDigitalControls();

        if (petal.switches[Terrarium::FOOTSWITCH_1].RisingEdge())
        {
            effect_enabled = !effect_enabled;
        }

        // LED1 indicates if effect is enabled
        led1.Set(effect_enabled ? 1.0f : 0.0f);
        led1.Update();

        // LED2 shows mix level from KNOB_3
        led2.Set(param_mix.Process());
        led2.Update();

        System::Delay(10);
    }
}
