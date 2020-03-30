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
#define NUM_SWARM 70
daisysp::Oscillator swarm[NUM_SWARM];
uint8_t             wave;
//SdmmcHandler        sdcard;
//WavPlayer           sampler;

arm_rfft_fast_instance_f32 fft;
float                      DSY_SDRAM_BSS fftinbuff[1024];
float                      DSY_SDRAM_BSS fftoutbuff[1024];

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
    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);
    arm_rfft_fast_f32(&fft, fftinbuff, fftoutbuff, 0);

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
            //            dryl = in[i];
            //            dryr = in[i + 1];
            dryl = tempo;
            dryr = temps;
            //                    verb.Process(sendl, sendr, &wetl, &wetr);
            wetl  = del.Read();
            sendl = dryl + (wetl * 0.5f);
            sendr = dryr + (wetr * 0.5f);
            del.Write(sendl);
            wetl = 0.0f;
            wetr = 0.0f;
            dryl = in[i];
            dryr = in[i + 1];

            out[i]     = dryr + wetl;
            out[i + 1] = dryl + wetl;
            //                    out[i + 1] = dryr + wetr;
        }
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

float r, g, b, fsbright;
int   main(void)
{
    float    samplerate;
    uint32_t tick, now;
    uint8_t  active_led;
    hw.Init();
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
    //    verb.Init(samplerate);
    //    sdcard.Init();
    //    sampler.Init();
    //    sampler.SetLooping(true);
    // Start
//    hw.StartAdc();
//    hw.StartAudio(AudioTest);
    hw.StartAudio(passthru);
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
