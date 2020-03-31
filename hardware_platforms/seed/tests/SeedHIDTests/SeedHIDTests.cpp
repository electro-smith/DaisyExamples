// HID Tests:
// Reworking all hid_* modules to be C++ for better user-interface.
//
// I'll be using the daisy pod to test most of the hardware,
// but won't be using the Pod BSP. Just to make sure the
// initialization for each component is nice and clean.
//
// Once everything's ironed out. I'll back annotate the
// init changes, etc. to the bsp files.
#include <string.h>
#include "daisy_seed.h"
#include "daisysp.h"
#include <math.h>

using namespace daisy;

static void init_led();
static void init_pod_adcs();

class BasicRgb
{
  public:
    enum Color
    {
        OFF,
        RED,
        GREEN,
        BLUE,
        WHITE,
    };
    BasicRgb() {}
    ~BasicRgb() {}
    void Init(dsy_gpio_pin r, dsy_gpio_pin g, dsy_gpio_pin b) 
    {
        hw_r_.pin = r;
        hw_g_.pin = g;
        hw_b_.pin = b;
        hw_r_.mode = hw_g_.mode = hw_b_.mode = DSY_GPIO_MODE_OUTPUT_PP;
        hw_r_.pull = hw_g_.pull = hw_b_.pull = DSY_GPIO_NOPULL;
        dsy_gpio_init(&hw_r_);
        dsy_gpio_init(&hw_g_);
        dsy_gpio_init(&hw_b_);
    }

    void Set(Color c) 
    {
        switch(c)
        {
            case OFF:
                dsy_gpio_write(&hw_r_, 1);
                dsy_gpio_write(&hw_g_, 1);
                dsy_gpio_write(&hw_b_, 1);
                break;
            case RED: 
                dsy_gpio_write(&hw_r_, 0); 
                dsy_gpio_write(&hw_g_, 1); 
                dsy_gpio_write(&hw_b_, 1); 
                break;
            case GREEN: 
                dsy_gpio_write(&hw_r_, 1); 
                dsy_gpio_write(&hw_g_, 0); 
                dsy_gpio_write(&hw_b_, 1); 
                break;
            case BLUE: 
                dsy_gpio_write(&hw_r_, 1); 
                dsy_gpio_write(&hw_g_, 1); 
                dsy_gpio_write(&hw_b_, 0); 
                break;
            case WHITE: 
                dsy_gpio_write(&hw_r_, 0); 
                dsy_gpio_write(&hw_g_, 0); 
                dsy_gpio_write(&hw_b_, 0); 
                break;
            default: break;
        }
    }


  private:
    bool     r_, g_, b_;
    dsy_gpio hw_r_, hw_g_, hw_b_;
};

enum class LedState
{
    ON  = 0x00,
    OFF = 0x01,
};

daisysp::Oscillator osc;

// Functional Modules
DaisySeed hw;
dsy_gpio led_blue;
Switch sw_1;
Encoder enc;
BasicRgb led2;
AnalogControl pot1;
parameter     p;

// Module in progress.

uint8_t c;

void AudioCallback(float *in, float *out, size_t size)
{
    // Ticks at FS/size (default 2kHz)
    int32_t inc;
    sw_1.Debounce();
	if(sw_1.RisingEdge())
        dsy_gpio_write(&led_blue, static_cast<uint8_t>(LedState::ON));
    if(sw_1.TimeHeldMs() > 1000 || sw_1.FallingEdge())
        dsy_gpio_write(&led_blue, static_cast<uint8_t>(LedState::OFF));


    enc.Debounce();
    inc = enc.Increment();
    if(inc > 0)
    {
        c = (c + 1) % 5;
    }
    else if(inc < 0)
    {
        if(c == 0)
            c = 4;
        else
            c -= 1;
    }

    if(enc.RisingEdge())
        c = BasicRgb::Color::WHITE;

    if(enc.FallingEdge())
        c = BasicRgb::Color::OFF;

    if(enc.TimeHeldMs() > 1000)
        c = (dsy_system_getnow() & 255) > 127 ? BasicRgb::Color::WHITE
                                              : BasicRgb::Color::OFF;

    led2.Set(static_cast<BasicRgb::Color>(c));


    for(size_t i = 0; i < size; i += 2)
    {
        // Ticks at FS
        //osc.set_freq(daisysp::mtof(12.0f + (pot1.Process() * 96.0f)));
        osc.SetFreq(daisysp::mtof(p.process()));
        out[i] = osc.Process();
        out[i + 1] = out[i];
    }
}


int main(void)
{
    // Initialize Hardware
    float sample_rate;
    hw.Init();
    sample_rate = hw.AudioSampleRate();
    init_pod_adcs();
    osc.Init(sample_rate);
    osc.SetAmp(1.00f);
    osc.SetWaveform(daisysp::Oscillator::WAVE_TRI);
    dsy_tim_start();
    init_led();

    led2.Init(hw.GetPin(0), hw.GetPin(25), hw.GetPin(24));

    // Testing New Switch
    sw_1.Init(hw.GetPin(28),
              sample_rate / 24,
              Switch::TYPE_MOMENTARY,
              Switch::POLARITY_INVERTED,
              Switch::PULL_UP);

    // Testing New Encoder
    enc.Init(hw.GetPin(27), hw.GetPin(26), hw.GetPin(1),
             sample_rate / 24);
    
    pot1.Init(dsy_adc_get_rawptr(0), sample_rate);
    p.init(pot1, 12.0f, 96.0f, parameter::LINEAR);

    dsy_adc_start();
    hw.StartAudio(AudioCallback);

    while(1) {}
}

void init_led()
{
    // Still old style so as not to break
    // pure-c modules..
    led_blue.pin = hw.GetPin(19);
    led_blue.mode     = DSY_GPIO_MODE_OUTPUT_PP;
    led_blue.pull     = DSY_GPIO_NOPULL;
    dsy_gpio_init(&led_blue);
}

void init_pod_adcs()  
{
    uint8_t channel_order[2] = {DSY_ADC_PIN_CHN11, DSY_ADC_PIN_CHN10};
    hw.adc_handle.channels      = 2;
    for(uint8_t i = 0; i < 2; i++)
    {
        hw.adc_handle.active_channels[i] = channel_order[i];
    }
    dsy_adc_init(&hw.adc_handle);
}
