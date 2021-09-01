#include "daisy_patch_sm.h"
#include "daisysp.h"

/** These are namespaces for the daisy libraries.
 *  These lines allow us to omit the "daisy::" and "daisysp::" before
 * referencing modules, and functions within the daisy libraries.
 */
using namespace daisy;
using namespace daisysp;

/** Our hardware board class handles the interface to the actual DaisyPatchSM
 * hardware. */
DaisyPatchSM hw;

/** A few Oscillators to build up a simple synthesizer
 *
 *  This is a generic oscillator class that can synthesize
 *  different waveforms, and has inputs for sync as well as FM
 */
Oscillator osc_a, osc_b, osc_c;

/** Callback for processing and synthesizing audio
 *
 *  The audio buffers are arranged as arrays of samples for each channel.
 *  For example, to access the left input you would use:
 *    in[0][n]
 *  where n is the specific sample.
 *  There are "size" samples in each array.
 *
 *  The default size is very small (just 4 samples per channel). This means the
 * callback is being called at 16kHz.
 *
 *  This size is acceptable for many applications, and provides an extremely low
 * latency from input to output. However, you can change this size by calling
 * hw.SetAudioBlockSize(desired_size). When running complex DSP it can be more
 * efficient to do the processing on larger chunks at a time.
 *
 */
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    /** First we'll tell the hardware to process the 8 analog inputs */
    hw.ProcessAnalogControls();

    /** We'll get a nice frequency control knob by using midi note numbers */
    float coarse_tune = 12.f + (hw.GetAdcValue(0) * 72.f);

    /** Using the second analog input we'll use a fine tune of the primary
   * frequency. */
    float fine_tune = hw.GetAdcValue(1) * 10.f;

    /** Convert those values from midi notes to frequency */
    float freq_a = mtof(coarse_tune + fine_tune);

    /** Our third control will detune the voices from each other */
    float detune_amt = hw.GetAdcValue(2);

    /**  Detuning each of the other oscillators by upto 5% of the primary
   * frequency */
    float freq_b = freq_a + (0.05 * freq_a * detune_amt);
    float freq_c = freq_a - (0.05 * freq_a * detune_amt);

    /** Set the oscillators to those frequencies */
    osc_a.SetFreq(freq_a);
    osc_b.SetFreq(freq_b);
    osc_c.SetFreq(freq_c);

    /** This loop will allow us to process the individual samples of audio */
    for(size_t i = 0; i < size; i++)
    {
        /** We'll combine both oscillator signals using simple addition
     *
     *  All of the DSP modules within DaisySP feature a "Process" function
     *  that will return the next sample of the synthesized sound, or effect.
     */
        float sig = osc_a.Process() + osc_b.Process() + osc_c.Process();

        /** In this example both outputs will be the same */
        out[0][i] = out[1][i] = sig;
    }
}

int main(void)
{
    /** Initialize the hardware */
    hw.Init();

    /** Initialize the oscillator modules */
    osc_a.Init(hw.AudioSampleRate());
    osc_b.Init(hw.AudioSampleRate());
    osc_c.Init(hw.AudioSampleRate());

    /** Let's set some specific parameters that won't change during the example
   *
   *  The POLYBLEP waveforms are bandlimited which means that they won't alias.
   */
    osc_a.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    osc_b.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    osc_c.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);

    /** Start Processing the audio */
    hw.StartAudio(AudioCallback);
    while(1) {}
}
