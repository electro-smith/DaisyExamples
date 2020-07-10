#include "daisy_field.h"
#include "daisysp.h"

#define NUM_VOICES 16

using namespace daisy;

DaisyField hw;


struct voice
{
    void Init()
    {
        osc_.Init(DSY_AUDIO_SAMPLE_RATE);
        amp_ = 0.0f;
        osc_.SetAmp(1.0f);
        osc_.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
        on_ = false;
    }
    float Process()
    {
        float sig;
        amp_ += 0.0025f * ((on_ ? 1.0f : 0.0f) - amp_);
        sig = osc_.Process() * amp_;
        return sig;
    }
    void set_note(float nn) { osc_.SetFreq(daisysp::mtof(nn)); }

    daisysp::Oscillator osc_;
    float               amp_, midibase_;
    bool                on_;
};

voice   v[NUM_VOICES];
uint8_t buttons[16];
// Use bottom row to set major scale
// Top row chromatic notes, and the inbetween notes are just the octave.
float scale[16]   = {0.f,
                   2.f,
                   4.f,
                   5.f,
                   7.f,
                   9.f,
                   11.f,
                   12.f,
                   1.f,
                   3.f,
                   0.f,
                   6.f,
                   8.f,
                   10.f,
                   0.0f};
float active_note = scale[0];

int8_t octaves = 0;

static daisysp::ReverbSc verb;
// Use two side buttons to change octaves.
float kvals[8];
float cvvals[4];

size_t knob_idx[] = {DaisyField::KNOB_1,
                     DaisyField::KNOB_2,
                     DaisyField::KNOB_3,
                     DaisyField::KNOB_4,
                     DaisyField::KNOB_5,
                     DaisyField::KNOB_6,
                     DaisyField::KNOB_7,
                     DaisyField::KNOB_8};

void AudioCallback(float *in, float *out, size_t size)
{
    bool trig, use_verb;
    hw.ProcessAnalogControls();
    hw.UpdateDigitalControls();
    if(hw.GetSwitch(DaisyField::SW_1)->RisingEdge())
    {
        octaves -= 1;
        trig = true;
    }
    if(hw.GetSwitch(DaisyField::SW_2)->RisingEdge())
    {
        octaves += 1;
        trig = true;
    }
    use_verb = true;

    for(int i = 0; i < 8; i++)
    {
        kvals[knob_idx[i]] = hw.GetKnobValue(i);
        if(i < 4)
        {
            cvvals[i] = hw.GetCvValue(i);
        }
    }

    if(octaves < 0)
        octaves = 0;
    if(octaves > 4)
        octaves = 4;

    if(trig)
    {
        for(int i = 0; i < NUM_VOICES; i++)
        {
            v[i].set_note((12.0f * octaves) + 24.0f + scale[i]);
        }
    }
    for(size_t i = 0; i < 16; i++)
    {
        v[i].on_ = hw.KeyboardState(i);
    }
    float sig, send;
    float wetl, wetr;
    for(size_t i = 0; i < size; i += 2)
    {
        sig = 0.0f;
        for(int i = 0; i < NUM_VOICES; i++)
        {
            if(i != 10 && i != 14 && i != 15)
                sig += v[i].Process();
        }
        send = sig * 0.35f;
        verb.Process(send, send, &wetl, &wetr);
        //        wetl = wetr = sig;
        if(!use_verb)
            wetl = wetr = 0.0f;
        out[i]     = (sig + wetl) * 0.5f;
        out[i + 1] = (sig + wetr) * 0.5f;
    }
}

void AudioInputTest(float *in, float *out, size_t size)
{
    float sendL, sendR, wetL, wetR;
    for(size_t i = 0; i < size; i++)
    {
        sendL = in[i] * 0.7f;
        sendR = in[i + 1] * 0.7f;
        verb.Process(sendL, sendR, &wetL, &wetR);
        out[i]     = in[i] * 0.8f + wetL;
        out[i + 1] = in[i + 1] * 0.8f + wetR;
    }
}


int main(void)
{
    hw.Init();
    // Initialize controls.
    octaves = 2;
    for(int i = 0; i < NUM_VOICES; i++)
    {
        v[i].Init();
        v[i].set_note((12.0f * octaves) + 24.0f + scale[i]);
    }

    verb.Init(hw.SampleRate());
    verb.SetFeedback(0.94f);
    verb.SetLpFreq(8000.0f);

    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    for(;;)
    {
        hw.VegasMode();
        dsy_system_delay(1);
        dsy_dac_write(DSY_DAC_CHN1, hw.GetKnobValue(0) * 4095);
        dsy_dac_write(DSY_DAC_CHN2, hw.GetKnobValue(1) * 4095);
        dsy_gpio_toggle(&hw.gate_out_);
    }
}
