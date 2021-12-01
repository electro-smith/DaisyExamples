#include "daisy_pod.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPod   hw;
Oscillator osc[4];
Parameter  p_freq, p_inversion;
int        notes[4];
int        chord[10][3];
Color      colors[10];
int        chordNum = 0;

void UpdateControls();

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    UpdateControls();

    for(int i = 0; i < 4; i++)
    {
        osc[i].SetFreq(mtof(notes[i]));
    }

    // Audio Loop
    for(size_t i = 0; i < size; i += 2)
    {
        float sig = 0;
        for(int i = 0; i < 4; i++)
        {
            sig += osc[i].Process();
        }

        out[i]     = sig;
        out[i + 1] = sig;
    }
}

void InitSynth(float samplerate)
{
    // Init freq Parameter to knob1 using MIDI note numbers
    // min 10, max 127, curve linear
    p_freq.Init(hw.knob1, 0, 127, Parameter::LINEAR);
    p_inversion.Init(hw.knob2, 0, 5, Parameter::LINEAR);

    for(int i = 0; i < 4; i++)
    {
        osc[i].Init(samplerate);
        osc[i].SetAmp(0.1f);
        osc[i].SetWaveform(Oscillator::WAVE_SIN);
        notes[i] = 30;
    }
}

void InitChords()
{
    // Maj, min, Aug, Dim
    // Maj7, min7, dom7, min/Maj7
    // dim7, half dim7

    //set thirds
    for(int i = 0; i < 8; i++)
    {
        //every other chord, maj third, min third
        chord[i][0] = 3 + ((i + 1) % 2);
    }
    //min 3rds
    chord[8][0] = chord[9][0] = 3;

    //set fifths
    // perfect 5th
    chord[0][1] = chord[1][1] = chord[4][1] = chord[5][1] = chord[6][1]
        = chord[7][1]                                     = 7;
    // diminished 5th
    chord[3][1] = chord[8][1] = chord[9][1] = 6;
    // augmented 5th
    chord[2][1] = 8;

    //set sevenths
    // triads (octave since triad has no 7th)
    chord[0][2] = chord[1][2] = chord[2][2] = chord[3][2] = 12;
    // major 7th
    chord[4][2] = chord[7][2] = 11;
    // minor 7th
    chord[5][2] = chord[6][2] = chord[9][2] = 10;
    // diminished 7th
    chord[8][2] = 9;
}

void InitColors()
{
    for(int i = 0; i < 7; i++)
    {
        colors[i].Init((Color::PresetColor)i);
    }
    colors[7].Init(1, 1, 0);
    colors[8].Init(1, 0, 1);
    colors[9].Init(0, .7, .4);
}

int main(void)
{
    float samplerate;

    // Init everything
    hw.Init();
    hw.SetAudioBlockSize(4);
    samplerate = hw.AudioSampleRate();

    InitSynth(samplerate);
    InitChords();
    InitColors();

    // start callbacks
    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    while(1) {}
}

void UpdateEncoder()
{
    if(hw.encoder.RisingEdge())
    {
        chordNum = 0;
    }

    chordNum += hw.encoder.Increment();
    chordNum = (chordNum % 10 + 10) % 10;
}

void UpdateKnobs()
{
    int freq      = (int)p_freq.Process();
    int inversion = (int)p_inversion.Process();

    notes[0] = freq + (12 * (inversion >= 1));
    notes[1] = freq + chord[chordNum][0] + (12 * (inversion >= 2));
    notes[2] = freq + chord[chordNum][1] + (12 * (inversion >= 3));
    notes[3] = freq + chord[chordNum][2] + (12 * (inversion >= 4));
}

void UpdateLeds()
{
    hw.led1.SetColor(colors[chordNum]);
    hw.led2.SetColor(colors[chordNum]);
    hw.UpdateLeds();
}

void UpdateControls()
{
    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();

    UpdateEncoder();
    UpdateKnobs();
    UpdateLeds();
}
