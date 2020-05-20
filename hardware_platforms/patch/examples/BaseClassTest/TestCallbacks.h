#pragma once

#include "daisy_patch.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

#define NUM_OSC 25

DaisyPatch          hw;

ReverbSc verb;
DelayLine<float, 300000> DSY_SDRAM_BSS delay_left, delay_right;
Oscillator                             osc[NUM_OSC];


bool bypass_state;

float delaytime;

void AudioCallback(float *in, float *out, size_t size)
{
    float dryl, dryr, sendl, sendr, wetl, wetr;
    float verbsend, verbtime, newdelaytime, delayamt;
    // meta params
    float dfb, dmix;
    hw.UpdateAnalogControls();
    hw.DebounceControls();
    verbsend = (1.f + hw.GetCtrlValue(DaisyPatch::CTRL_1));
    verbtime = (1.f + hw.GetCtrlValue(DaisyPatch::CTRL_2));
    newdelaytime = (1.f + hw.GetCtrlValue(DaisyPatch::CTRL_3)) * 5.0f;
    delayamt = (1.f + hw.GetCtrlValue(DaisyPatch::CTRL_4));
    dmix      = delayamt * 0.75f;
    dfb       = delayamt * delayamt * 0.8f;
    delaytime += 0.002f * (newdelaytime - delaytime);
    delay_left.SetDelay(hw.AudioSampleRate() * delaytime);
    delay_right.SetDelay(hw.AudioSampleRate() * delaytime);
    if(hw.encoder.FallingEdge()) {
        bypass_state = !bypass_state;
    }
    float somesig;
    for(size_t i = 0; i < size; i += 2)
    {
		somesig = 0.0f;
        for(size_t j = 0; j < NUM_OSC; j++) {
            somesig += osc[j].Process();
		}
        float dinl, dinr;
        float verboutl, verboutr;
        dryl       = in[i];
        dryr       = in[i+1];
//        dryr       = somesig;
        sendl      = verbsend * dryl;
        sendr      = verbsend * dryr;
        verb.SetFeedback(verbtime * verbtime);
        dinl = delay_left.Read();
        dinr = delay_right.Read();
        delay_left.Write((dinl * dfb) + dryl);
        delay_right.Write((dinr * dfb) + dryr);
        verb.Process(sendl, sendr, &verboutl, &verboutr);
        wetl       = verboutl + (dinl * dmix) + (dryl * (1.0f - dmix));
        wetr       = verboutr + (dinr * dmix) + (dryr * (1.0f - dmix));
        if(bypass_state)
        {
            out[i] = in[i];
            out[i+1] = in[i+1];
        }
        else
        {
			out[i]     = wetl;
			out[i + 1] = somesig;
        }
    }
}
void SecondaryCallback(float *in, float *out, size_t size)
{
    for(size_t i = 0; i < size; i += 2)
    {
		float somesig = 0.0f;
        for(size_t j = 0; j < NUM_OSC; j++) {
            somesig += osc[j].Process();
		}
        if(bypass_state)
        {
            out[i]     = in[i];
            out[i + 1] = in[i + 1];
        }
        else
        {
			out[i] = somesig;
			out[i + 1] = somesig;
        }
    }
}

void SimplerCallback(float *in, float *out, size_t size) 
{
    float dryl, dryr, sendl, sendr, wetl, wetr;
    float verbsend, verbtime, newdelaytime, delayamt;
    // meta params
    float dfb, dmix;
    hw.UpdateAnalogControls();
    hw.DebounceControls();
    verbsend = (1.f + hw.GetCtrlValue(DaisyPatch::CTRL_1));
    verbtime = (DSY_CLAMP((1.f + hw.GetCtrlValue(DaisyPatch::CTRL_2)), 0.0f, 1.0f) * 0.25f) + 0.74f;
    newdelaytime = (1.f + hw.GetCtrlValue(DaisyPatch::CTRL_3)) * 5.0f;
    delayamt = (1.f + hw.GetCtrlValue(DaisyPatch::CTRL_4));
    dmix      = delayamt * 0.75f;
    dfb       = delayamt * delayamt * 0.8f;
    //    delaytime += 0.002f * (newdelaytime - delaytime);
    delaytime = hw.AudioSampleRate() * 0.500f;
    delay_left.SetDelay(hw.AudioSampleRate() * delaytime);
    delay_right.SetDelay(hw.AudioSampleRate() * delaytime);
    if(hw.encoder.FallingEdge()) {
        bypass_state = !bypass_state;
    }
    for(size_t i = 0; i < size; i += 2)
    {
        float dinl, dinr;
        float verboutl, verboutr;
        dryl       = in[i];
        dryr       = in[i+1];
        sendl      = verbsend * dryl;
        sendr      = verbsend * dryr;
        verb.SetFeedback(verbtime * verbtime);
        dinl = delay_left.Read();
        dinr = delay_right.Read();
        delay_left.Write((dinl * dfb) + dryl);
        delay_right.Write((dinr * dfb) + dryr);
        verb.Process(sendl, sendr, &verboutl, &verboutr);
        wetl       = verboutl + (dinl * dmix) + (dryl * (1.0f - dmix));
        wetr       = verboutr + (dinr * dmix) + (dryr * (1.0f - dmix));
        if(bypass_state)
        {
            out[i] = in[i];
            out[i+1] = in[i+1];
        }
        else
        {
			out[i]     = wetl;
			out[i + 1] = wetr;
        }
    }
}

void RunTestCallbacksMain() 
{
    float samplerate;
    // Init
    hw.Init();
    samplerate = hw.AudioSampleRate();
    verb.Init(samplerate);
    verb.SetLpFreq(12000.0f);
    delay_left.Init();
    delay_right.Init();
    for(size_t i = 0; i < NUM_OSC; i++)
    {
        osc[i].Init(samplerate);
        osc[i].SetAmp(0.5f);
        osc[i].SetFreq(200.0f + (i * 0.5f));
        osc[i].SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    }
    size_t initial_delay;
    initial_delay = 48000;
    delay_left.SetDelay(initial_delay);
    delay_right.SetDelay(initial_delay);
    // Display Test
    hw.StartAdc();
//    dsy_audio_set_callback(DSY_AUDIO_EXTERNAL, SecondaryCallback);
//    dsy_audio_start(DSY_AUDIO_EXTERNAL);

    hw.StartAudio(SimplerCallback);
    for(;;)
    {
        // Handle MIDI Events
        hw.DisplayControls();
    }
}
