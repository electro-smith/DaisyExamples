// # Seed3Dev
// ## Description
// Dev Playground for Seed3
//
#include <stdio.h>
#include <string.h>
//#include "daisy_seed.h"
#include "daisy_pod.h"
#include "daisy_petal.h"
#include "daisysp.h"
#include "arm_math.h"
using namespace daisy;

#define USE_POD

// Globals
#ifdef USE_POD
#define DEV_NAMESPACE DaisyPod
#else
#define DEV_NAMESPACE DaisyPetal
#endif
DEV_NAMESPACE hw;
//daisysp::Oscillator DSY_SDRAM_BSS osc;
daisysp::DelayLine<float, 48000 * 4> DSY_SDRAM_BSS del;

daisysp::Oscillator osc;
//#define NUM_SWARM 70
#define NUM_SWARM 15
daisysp::Oscillator swarm[NUM_SWARM];
uint8_t             wave;
daisysp::ReverbSc   verb;
//SdmmcHandler        sdcard;
//WavPlayer           sampler;

arm_rfft_fast_instance_f32 fft;
float DSY_SDRAM_BSS fftinbuff[1024];
float DSY_SDRAM_BSS fftoutbuff[1024];

float amp, targetamp;

void AudioTest(float *in, float *out, size_t size)
{
    hw.UpdateAnalogControls();
    hw.DebounceControls();
    float sig, note;
    // One way to get value
    note = hw.GetKnobValue(DEV_NAMESPACE::KNOB_1) * 127.0f;
    osc.SetFreq(daisysp::mtof(note));

    // Handle Encoder for waveform switching test.
    int32_t inc;
    inc = hw.encoder.Increment();
    if(inc > 0)
    {
        wave = (wave + 1) % daisysp::Oscillator::WAVE_LAST;
        osc.SetWaveform(wave);
    }
    else if(inc < 0)
    {
        wave = (wave + daisysp::Oscillator::WAVE_LAST - 1)
               % daisysp::Oscillator::WAVE_LAST;
        osc.SetWaveform(wave);
    }

    // Turn amplitude on when pressing button.
    //    targetamp
    //        = (hw.encoder.Pressed() || hw.switches[DEV_NAMESPACE::SW_5].Pressed())
    //              ? 0.8f
    //              : 0.0f;
    targetamp = (hw.encoder.Pressed()) ? 0.8f : 0.0f;
    // fill inbuff with 128 samples
    for(size_t i = 0; i < size / 2; i++)
    {
        fftinbuff[i] = in[i * 2];
    }
    //    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    //    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    //    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    //    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    //    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    //    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    //    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    //    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    //    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    //    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    //    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    //    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    //    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    //    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    //    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    //    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    float delsig;

    for(size_t i = 0; i < size; i += 2)

    {
        daisysp::fonepole(amp, targetamp, 0.005f);
        osc.SetAmp(amp);
        //        sig    = osc.Process();
        //        out[i] = out[i + 1] = sig;
        //                if(hw.switches[DEV_NAMESPACE::SW_6].Pressed())
        //                {
        //                    sig    = osc.Process();
        //                    out[i] = out[i + 1] = sig;
        //                }
        //                else
        float tempo, temps;
        tempo = 0.0f;
        for(size_t i = 0; i < NUM_SWARM; i++)
        {
            tempo += swarm[i].Process() * 0.35f;
        }
        // FFT THAT ISH!

        //        temps = s162f(sampler.Stream());
        {
            float wetl, wetr, dryl, dryr, sendl, sendr;
            wetl = 0.0f;
            wetr = 0.0f;
            dryl = in[i] * 0.45f;
            dryr = in[i + 1] * 0.45f;
            //            dryl  = tempo;
            //            dryr  = temps;
            delsig = del.Read();
            sendl  = (dryl * 0.5f) + (delsig * 0.5f);
            sendr  = (dryr * 0.5f) + (delsig * 0.5f);
            del.Write(sendl);
            //            wetl = 0.0f;
            //            wetr = 0.0f;
            verb.Process(sendl, sendr, &wetl, &wetr);

            out[i]     = (dryr + wetl + delsig) * 0.6f;
            out[i + 1] = (dryl + wetl + delsig) * 0.6f;
            //                    out[i + 1] = dryr + wetr;
        }
    }
}

void verbthru(float *in, float *out, size_t size)
{
    for(size_t i = 0; i < size; i += 2)
    {
        float wetl, wetr, dryl, dryr, sendl, sendr;
        wetl  = 0.0f;
        wetr  = 0.0f;
        dryl  = in[i + 1] * 0.45f;
        dryr  = in[i] * 0.45f;
        sendl = dryl * 0.65f;
        sendr = dryr * 0.65f;
        verb.Process(sendl, sendr, &wetl, &wetr);
        out[i]     = (wetl + dryl) * 0.507f;
        out[i + 1] = (wetr + dryr) * 0.507f;
        //        out[i]     = in[i + 1];
        //        out[i + 1] = in[i];
    }
}
void passthru(float *in, float *out, size_t size)
{
    for(size_t i = 0; i < size; i += 2)
    {
        out[i]     = in[i + 1];
        out[i + 1] = in[i];
    }
}

uint8_t waveform;
void    r2d2(float *in, float *out, size_t size)
{
    float sig, freq;
    hw.DebounceControls();
    hw.UpdateAnalogControls();

    freq = daisysp::mtof(hw.GetKnobValue(DaisyPod::KNOB_1) * 127.0f);
    amp  = (hw.button2.Pressed()) ? 1.0f : 0.0f;
    int32_t inc;
    inc = hw.encoder.Increment();
    uint8_t prev_waveform;
    prev_waveform = waveform;
    if(inc > 0)
    {
        waveform = (waveform + 1) % daisysp::Oscillator::WAVE_LAST;
    }
    else if(inc < 0)
    {
        waveform = (waveform + daisysp::Oscillator::WAVE_LAST - 1)
                   % daisysp::Oscillator::WAVE_LAST;
    }
    if(waveform != prev_waveform)
    {
        osc.SetWaveform(waveform);
    }
    osc.SetFreq(freq);
    osc.SetAmp(amp);

    for(size_t i = 0; i < size; i += 2)
    {
        osc.SetAmp(amp);
        sig        = osc.Process();
        out[i]     = sig;
        out[i + 1] = sig;
    }
}

float r, g, b, fsbright;
int   main(void)
{
    float    samplerate;
    uint32_t tick, now;
    uint8_t  active_led;
    hw.Init();
    dsy_gpio p;
    p.pin = {DSY_GPIOC, 0};
    p.mode = DSY_GPIO_MODE_INPUT;
    p.pull = DSY_GPIO_PULLDOWN;
    dsy_gpio_init(&p);
    for(size_t i = 0; i < 32; i++) {
        p.pin = hw.seed.GetPin(i);
		dsy_gpio_deinit(&p);
		p.mode = DSY_GPIO_MODE_INPUT;
		p.pull = DSY_GPIO_PULLDOWN;
		dsy_gpio_init(&p);
    }
    samplerate = hw.AudioSampleRate();
    hw.SetAudioBlockSize(128);
    // Init Synthesis
    osc.Init(samplerate);
    for(size_t i = 0; i < NUM_SWARM; i++)
    {
        swarm[i].Init(samplerate);
        swarm[i].SetAmp(0.1f);
        swarm[i].SetFreq(200.0f + (i * 1.0f));
        swarm[i].SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
    }
    del.Init();
    del.SetDelay(samplerate * 0.25f);
    arm_rfft_fast_init_f32(&fft, 1024);
    verb.Init(samplerate);
    //    sdcard.Init();
    //    sampler.Init();
    //    sampler.SetLooping(true);
    // Start
    hw.StartAdc();
    hw.StartAudio(AudioTest);
    //    hw.StartAudio(passthru);
    //    hw.ClearLeds();
    for(;;)
    {
        //        now = dsy_system_getnow();
        //        sampler.Prepare();
        arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);


        //        r        = hw.GetKnobValue(DEV_NAMESPACE::KNOB_1);
        //        g        = hw.GetKnobValue(DEV_NAMESPACE::KNOB_2);
        //        b        = hw.GetKnobValue(DEV_NAMESPACE::KNOB_3);
        //        fsbright = hw.GetKnobValue(DEV_NAMESPACE::KNOB_4);
        //
        //        // Led Ring goes in a circle
        //        if(now - tick > 250)
        //        {
        //            tick       = now;
        //            active_led = (active_led + 1) % DEV_NAMESPACE::RING_LED_LAST;
        //        }
        //        for(size_t i = 0; i < DEV_NAMESPACE::RING_LED_LAST; i++)
        //        {
        //            if(i == active_led)
        //            {
        //                hw.SetRingLed(static_cast<DEV_NAMESPACE::RingLed>(i), r, g, b);
        //            }
        //            else
        //            {
        //                hw.SetRingLed(
        //                    static_cast<DEV_NAMESPACE::RingLed>(i), 0.0f, 0.0f, 0.0f);
        //            }
        //        }
        //        hw.SetFootswitchLed(DEV_NAMESPACE::FOOTSWITCH_LED_1, fsbright);
        //        hw.SetFootswitchLed(DEV_NAMESPACE::FOOTSWITCH_LED_2, fsbright);
        //        hw.SetFootswitchLed(DEV_NAMESPACE::FOOTSWITCH_LED_3, fsbright);
        //        hw.SetFootswitchLed(DEV_NAMESPACE::FOOTSWITCH_LED_4, fsbright);

        //        hw.UpdateLeds();
        //        hw.DelayMs(20);
    }
}
