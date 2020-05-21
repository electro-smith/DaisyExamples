#pragma once

#include "daisy_patch.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

#define NUM_OSC 25

class NewClass
{
  public:
    NewClass() {}
    ~NewClass() {}

  private:
    float sr_, fc_, res_, drive_, freq_, damp_;
    float notch_, low_, high_, band_, peak_;
    float input_;
    float out_low_, out_high_, out_band_, out_peak_, out_notch_;
};


DaisyPatch hw;

parameter p_xf, p_res;

ReverbSc verb;
//Svf DSY_SDRAM_BSS filter;
//newclass newclass;
Svf filter;

#define NUM_BYTES 1 * 1024 // 41 is threshold to break everything.
uint8_t                  bytes[NUM_BYTES];
Oscillator               osc[NUM_OSC];
DelayLine<float, 300000> DSY_SDRAM_BSS delay[4];
bool                                   bypass_state;
float                                  delaytime;

// All floats from making one filter in normal RAM -- is the problem?
//            float sr_, fc_, res_, drive_, freq_, damp_;
//            float notch_, low_, high_, band_, peak_;
//            float input_;
//            float out_low_, out_high_, out_band_, out_peak_, out_notch_;

float major_chord[4] = {0, 4, 7, 11};
float minor_chord[4] = {0, 3, 7, 10};

void QuadBypass(float **in, float **out, size_t size) 
{
    
    for(size_t i = 0; i < size; i++) {
        for(size_t chn = 0; chn < 4; chn++) {
            out[chn][i] = in[chn][i];
        }
    }
}

void ChordCallback(float **in, float **out, size_t size)
{
    size_t num_channels = 4;
    float  root, third, fifth, seventh;
    float  rootnote;
    hw.UpdateAnalogControls();
    rootnote = 24.f + ((hw.GetCtrlValue(DaisyPatch::CTRL_1)) * 60.0f);
    for(size_t i = 0; i < size; i++)
    {
        for(size_t chn = 0; chn < num_channels; chn++)
        {
            float *chord;
            chord = (hw.GetCtrlValue(DaisyPatch::CTRL_2)) > 0.5f ? major_chord
                                                                 : minor_chord;
            osc[chn].SetFreq(mtof(rootnote + chord[chn]));
            out[chn][i] = osc[chn].Process();
        }
    }
}
void MultiOutputFilter(float **in, float **out, size_t size)
{
    size_t num_channels = 4;
    hw.UpdateAnalogControls();
    for(size_t i = 0; i < size; i++)
    {
        filter.SetFreq(p_xf.process());
        filter.SetRes(p_res.process());
        filter.Process(in[0][i]);
        out[0][i] = filter.Low();
        out[1][i] = filter.Band();
        out[2][i] = filter.High();
        out[3][i] = filter.Notch();
    }
}


void SimplerCallback(float **in, float **out, size_t size)
{
    float dryl, dryr, sendl, sendr, wetl, wetr;
    float verbsend, verbtime, newdelaytime, delayamt;
    // meta params
    float dfb, dmix;
    hw.UpdateAnalogControls();
    hw.DebounceControls();
    verbsend = (1.f + hw.GetCtrlValue(DaisyPatch::CTRL_1));
    verbtime
        = (DSY_CLAMP((hw.GetCtrlValue(DaisyPatch::CTRL_2)), 0.0f, 1.0f) * 0.25f)
          + 0.74f;
    newdelaytime = 0.0015f + (hw.GetCtrlValue(DaisyPatch::CTRL_3)) * 0.6f;
    delayamt     = (hw.GetCtrlValue(DaisyPatch::CTRL_4));
    dmix         = delayamt * 0.75f;
    dfb          = delayamt * delayamt * 0.9f;
    delaytime += 0.001f * (newdelaytime - delaytime);
    if(hw.encoder.FallingEdge())
    {
        bypass_state = !bypass_state;
    }
    for(size_t i = 0; i < 4; i++)
    {
        delay[i].SetDelay(hw.AudioSampleRate()
                          * (delaytime + (i * (delaytime * 0.2f))));
        if(bypass_state)
        {
            for(size_t j = 0; j < size; j++)
            {
                out[i][j] = in[i][j];
            }
        }
        else
        {
            float dry, wet, del;
            for(size_t j = 0; j < size; j++)
            {
                dry = in[i][j];
                del = delay[i].Read();
                delay[i].Write(dry + (del * dfb));
                wet = (dry * sinf((1.0f - verbsend))) + (del * sinf(verbsend));
                out[i][j] = wet;
            }
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
        for(size_t i = 0; i < NUM_BYTES; i++)
        {
            bytes[i] = 0;
    	}
    for(size_t i = 0; i < NUM_OSC; i++)
    {
        osc[i].Init(samplerate);
        osc[i].SetAmp(0.5f);
        osc[i].SetFreq(200.0f + (i * 0.5f));
        osc[i].SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    }
    size_t initial_delay;
    initial_delay = 48000;
    for(size_t i = 0; i < 4; i++)
    {
        delay[i].Init();
        delay[i].SetDelay(initial_delay);
    }
    filter.Init(samplerate);
    p_xf.init(hw.controls[DaisyPatch::CTRL_1], 20.f, 16000.f, parameter::LOG);
    p_res.init(hw.controls[DaisyPatch::CTRL_2], 0.f, 1.f, parameter::LINEAR);
    // Display Test
    hw.StartAdc();
    hw.StartAudio(QuadBypass);
    //    hw.StartAudio(MultiOutputFilter);
    for(;;)
    {
        hw.DisplayControls();
    }
}
