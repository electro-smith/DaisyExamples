#include "daisy_field.h"
#include "daisysp.h"

#define NUM_VOICES 16
daisy_field hw;


struct voice
{
    void init()
    {
        osc_.Init(DSY_AUDIO_SAMPLE_RATE);
        amp_ = 0.0f;
        osc_.SetAmp(0.5f);
        osc_.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
        on_ = false;
    }
    float process()
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

voice v[NUM_VOICES];
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
daisy::AnalogControl knobs[8];
daisy::AnalogControl cvs[4];
float                kvals[8];
float                cvvals[4];

void AudioCallback(float *in, float *out, size_t size)
{
    bool trig, use_verb;
    dsy_sr_4021_update(&hw.keyboard_sr);
    hw.switches[SW_1].Debounce();
    hw.switches[SW_2].Debounce();
    hw.switches[SW_3].Debounce();
    if(hw.switches[SW_1].RisingEdge())
    {
        octaves -= 1;
        trig = true;
    }
    if(hw.switches[SW_2].RisingEdge())
    {
        octaves += 1;
        trig = true;
    }
    if(hw.switches[SW_3].Pressed())
    {
        use_verb = true;
    }
    else
    {
        use_verb = false;
    }
    for(int i = 0; i < 8; i++)
    {
        //kvals[i] = knobs[i].Process();
        kvals[i] = dsy_adc_get_mux_float(0, i);
        if(i < 4)
        {
            //cvvals[i] = cvs[i].Process();
            cvvals[i] = dsy_adc_get_float(i + 1);
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
        buttons[i] = dsy_sr_4021_state(&hw.keyboard_sr, i) | (buttons[i] << 1);
        {
            if(buttons[i] == 0xFF)
            {
                v[i].on_ = false;
            }
            else if(buttons[i] == 0x00)
            {
                v[i].on_ = true;
            }
        }
    }
    float sig, send;
    float wetl, wetr;
    for(size_t i = 0; i < size; i += 2)
    {
        sig = 0.0f;
        for(int i = 0; i < NUM_VOICES; i++)
        {
            if(i != 10 && i != 14 && i != 15)
                sig += v[i].process();
        }
        send = sig * 0.35f;
        verb.Process(send, send, &wetl, &wetr);
        //        wetl = wetr = sig;
        if(use_verb)
            wetl = wetr = 0.0f;
        out[i]     = (sig + wetl) * 0.5f;
        out[i + 1] = (sig + wetr) * 0.5f;
    }
}

void LightCallback(float *in, float *out, size_t size)
{
    for(int i = 0; i < 8; i++)
    {
        kvals[i] = knobs[i].Process();
        //        kvals[i] = dsy_adc_get_mux_float(0, i);
        if(i < 4)
        {
            cvvals[i] = cvs[i].Process();
            //            cvvals[i] = dsy_adc_get_float(i + 1);
        }
    }
}

int main(void)
{
    size_t blocksize = 48;
    daisy_field_init(&hw);
    // initialize controls.
    int potmap[8]
        = {KNOB_1, KNOB_2, KNOB_3, KNOB_4, KNOB_5, KNOB_6, KNOB_7, KNOB_8};
    for(int i = 0; i < 8; i++)
    {
        knobs[i].Init(dsy_adc_get_mux_rawptr(0, potmap[i]),
                      DSY_AUDIO_SAMPLE_RATE / blocksize);
    }
    cvs[0].InitBipolarCv(dsy_adc_get_rawptr(1),
                         DSY_AUDIO_SAMPLE_RATE / blocksize);
    cvs[1].InitBipolarCv(dsy_adc_get_rawptr(2),
                         DSY_AUDIO_SAMPLE_RATE / blocksize);
    cvs[2].InitBipolarCv(dsy_adc_get_rawptr(3),
                         DSY_AUDIO_SAMPLE_RATE / blocksize);
    cvs[3].InitBipolarCv(dsy_adc_get_rawptr(4),
                         DSY_AUDIO_SAMPLE_RATE / blocksize);
    octaves = 2;
    for(int i = 0; i < NUM_VOICES; i++)
    {
        v[i].init();
        v[i].set_note((12.0f * octaves) + 24.0f + scale[i]);
    }
    verb.Init(DSY_AUDIO_SAMPLE_RATE);
    verb.SetFeedback(0.94f);
    verb.SetLpFreq(8000.0f);
    // Init muxsel1
    dsy_gpio foo;
    foo.mode = DSY_GPIO_MODE_OUTPUT_PP;
    foo.pin  = {DSY_GPIOA, 7};
    foo.pull = DSY_GPIO_NOPULL;
    dsy_gpio_init(&foo);
    dsy_adc_start();
    dsy_audio_set_callback(DSY_AUDIO_INTERNAL, AudioCallback);
    dsy_audio_set_blocksize(DSY_AUDIO_INTERNAL, blocksize);
    dsy_audio_start(DSY_AUDIO_INTERNAL);
    bool ledstate;
    ledstate = true;
    for(;;)
    {
        dsy_tim_delay_ms(250);
        hw.seed.SetLed(ledstate);
        ledstate = !ledstate;
    }
}
