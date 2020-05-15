#include <stdio.h>
#include "daisy_board.h"

#define ADC_CHN_CTRL_1 DSY_ADC_PIN_CHN10
#define ADC_CHN_CTRL_2 DSY_ADC_PIN_CHN15
#define ADC_CHN_CTRL_3 DSY_ADC_PIN_CHN4
#define ADC_CHN_CTRL_4 DSY_ADC_PIN_CHN7
#define PIN_ENC_CLICK 0
#define PIN_ENC_B 11
#define PIN_ENC_A 12

using namespace daisy;

class JustSeed : public DaisyBoard
{
};

class PatchKnobs : public DaisyBoard
{
  public:
  private:
    void InitControls()
    {
        const size_t num_controls = 4;
        // Some of this stuff can move to base class if this works out.
        c_data_.num_controls      = num_controls;
        // Set order of ADCs based on CHANNEL NUMBER
        uint8_t channel_order[num_controls] = {
            ADC_CHN_CTRL_1,
            ADC_CHN_CTRL_2,
            ADC_CHN_CTRL_3,
            ADC_CHN_CTRL_4,
        };
        // NUMBER OF CHANNELS
        seed.adc_handle.channels = num_controls;
        // Fill the ADCs active channel array.
        for(uint8_t i = 0; i < num_controls; i++)
        {
            seed.adc_handle.active_channels[i] = channel_order[i];
        }
        // Set Oversampling to 32x
        seed.adc_handle.oversampling = DSY_ADC_OVS_32;
        // Init ADC
        dsy_adc_init(&seed.adc_handle);

        for(size_t i = 0; i < num_controls; i++)
        {
            control_[i].Init(dsy_adc_get_rawptr(i), AudioCallbackRate(), true);
        }
    }
};

class JustEncoder : public DaisyBoard
{
  public:
    void InitControls() {
        enc.Init(seed.GetPin(PIN_ENC_A),
                 seed.GetPin(PIN_ENC_B),
                 seed.GetPin(PIN_ENC_CLICK),
                 AudioCallbackRate());
    }
    void DebounceControls() { enc.Debounce(); }
    int32_t Increment() { return enc.Increment(); }
    bool    Click() { return enc.FallingEdge(); }

  private:
    Encoder enc;
};

//JustSeed hw;
PatchKnobs hw;
float      test;

// If we use DerivedOne here, test will always be 0.0f (+1)
// If we use DerivedTwo we'll be reading from the first knob of the Daisy Patch
void TestCallback(float *in, float *out, size_t size)
{
    hw.UpdateAnalogControls();
    hw.DebounceControls();
    test = hw.GetControlValue(0);
    test += 1.0f; // temp offset for bad AnalogControl code.
    for(size_t i = 0; i < size; i += 2)
    {
        out[i]     = in[i] * test;
        out[i + 1] = in[i + 1] * test;
    }
}

int main(void)
{
    hw.Init();
    test = 0.0f;
    hw.StartAudio(TestCallback);
    hw.StartAdc();
    for(;;) {}
}
