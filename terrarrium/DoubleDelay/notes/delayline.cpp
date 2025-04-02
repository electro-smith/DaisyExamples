#include "daisysp.h"
#include "daisy_seed.h"

// Interleaved audio definitions
#define LEFT (i)
#define RIGHT (i + 1)

// Set delay time range in samples.
#define MIN_DELAY 24
#define MAX_DELAY 63962

using namespace daisysp;
using namespace daisy;

// Set up Daisy hardware seed
static DaisySeed hw;

// Declare the button
Switch button1;

// Declare DelayLines of MAX_DELAY number of floats.
static DelayLine<float, static_cast<size_t>(MAX_DELAY)> delL;
static DelayLine<float, static_cast<size_t>(MAX_DELAY)> delR;

// Setup..
const int num_adc_channels = 3;
const int blockSize = 1;

// Scale the feedback signal
const float feedbackScalar=1.0;


//
// Audio Callback function  (the audio loop)
//
static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float feedbackL, feedbackR, delL_out, delR_out, sigL_out, sigR_out, del;
    
    for(size_t i = 0; i < size; i += 2)
    {
        
        float knob = hw.adc.GetFloat(0);
        float knob2 = hw.adc.GetFloat(1);
        float knob3 = hw.adc.GetFloat(2);

        knob *= knob;
        del = ((MAX_DELAY - MIN_DELAY) * knob)+(MIN_DELAY);
        delL.SetDelay(del);
        delR.SetDelay(del);

        button1.Debounce();
        float btn = button1.Pressed();

        // Read from delay line
        delL_out = delL.Read();
        delR_out = delR.Read();

        // Calculate output mix
        if(btn){
            if (knob3 <= .5){
                sigL_out  = ((in[LEFT]) + (delL_out * (knob3*2.0f)));
                sigR_out  = ((in[RIGHT]) + (delR_out * (knob3*2.0f)));
            }else{
                sigL_out  = (((in[LEFT])*(1-((knob3-0.5f)*2.0f)))) + (delL_out);
                sigR_out  = (((in[RIGHT])*(1-((knob3-0.5f)*2.0f)))) + (delR_out);
            }
        }else{
            sigL_out = in[LEFT];
            sigR_out = in[RIGHT];
        }

        // Calculate feedback
        feedbackL = (delL_out * knob2 * feedbackScalar) + in[LEFT];
        feedbackR = (delR_out * knob2 * feedbackScalar) + in[RIGHT];

        // Write to the delay
        delL.Write(feedbackL * btn);
        delR.Write(feedbackR * btn);
        
        // Output
        out[LEFT]  = sigL_out;
        out[RIGHT] = sigR_out;
    }
}

int main(void)
{
    // initialize seed hardware and daisysp modules
    float sample_rate;
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(blockSize);
    sample_rate = hw.AudioSampleRate();
    delL.Init();
    delR.Init();

    //This is our ADC configuration
    AdcChannelConfig adcConfig[num_adc_channels];
    
    //Configure pin 21 as an ADC input. This is where we'll read the knob.
    adcConfig[0].InitSingle(hw.GetPin(21));
    adcConfig[1].InitSingle(hw.GetPin(22));
    adcConfig[2].InitSingle(hw.GetPin(23));

    //Initialize the adc with the config we just made
    hw.adc.Init(adcConfig, num_adc_channels);

    //Initialize the button
    button1.Init(hw.GetPin(28),1000);

    // Set Default Delay time
    delL.SetDelay(sample_rate * 0.75f);
    delR.SetDelay(sample_rate * 0.75f);

    // Start reading ADC values
    hw.adc.Start();

    // Start audio loop    
    hw.StartAudio(AudioCallback);

    while(1) {}
}