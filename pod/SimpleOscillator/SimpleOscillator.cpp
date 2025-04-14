#include "daisy_pod.h"
#include "daisysp.h"

#define NUM_WAVEFORMS 4

using namespace daisy;
using namespace daisysp;

DaisyPod   hw;
Oscillator osc;
Parameter  p_freq;
Parameter  p_amp;

uint8_t waveforms[NUM_WAVEFORMS] = {
    Oscillator::WAVE_SIN,
    Oscillator::WAVE_TRI,
    Oscillator::WAVE_POLYBLEP_SAW,
    Oscillator::WAVE_POLYBLEP_SQUARE,
};

static float freq;
static float amp;
float        sig;
static int   waveform, octave, ampVal;
Color my_colors[5];

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    hw.ProcessDigitalControls();

    waveform += hw.encoder.Increment();
    waveform = DSY_CLAMP(waveform, 0, NUM_WAVEFORMS);
    osc.SetWaveform(waveforms[waveform]);

    if(hw.button2.RisingEdge())
        octave++;
    if(hw.button1.RisingEdge())
        octave--;
    octave = DSY_CLAMP(octave, 0, 4);

    // convert MIDI to frequency and multiply by octave size
    freq = mtof(p_freq.Process() + (octave * 12));
    amp = mtof(p_amp.Process() + (ampVal * 12));
    osc.SetFreq(freq);
    osc.SetAmp(amp);

    // Audio Loop
    for(size_t i = 0; i < size; i += 2)
    {
        // Process
        sig        = osc.Process();
        out[i]     = sig;
        out[i + 1] = sig;
        hw.led1.SetColor(my_colors[octave]);
        hw.led2.SetColor(my_colors[waveform]);
        hw.UpdateLeds();
    }
}

void InitSynth(float samplerate)
{
    // Init freq Parameter to knob1 using MIDI note numbers
    // min 10, max 127, curve linear
    p_freq.Init(hw.knob1, 0, 127, Parameter::LINEAR);
    // Init amp Parameter to knob2 using MIDI note numbers
    // min 10, max 127, curve linear
    p_amp.Init(hw.knob2, 0, 127, Parameter::LINEAR);

    osc.Init(samplerate);
    osc.SetAmp(1.f);

    waveform = 0;
    octave   = 0;
}

int main(void)
{
    float samplerate;

    // Init everything
    hw.Init();
    hw.SetAudioBlockSize(4);
    samplerate = hw.AudioSampleRate();
    InitSynth(samplerate);

    my_colors[0].Init(Color::PresetColor::RED);
    my_colors[1].Init(Color::PresetColor::GREEN);
    my_colors[2].Init(Color::PresetColor::BLUE);
    my_colors[3].Init(Color::PresetColor::WHITE);
    my_colors[4].Init(Color::PresetColor::PURPLE);

    // start callbacks
    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    while(1) {}
}
