#include "daisy_pod.h"
#include "daisysp.h"

#define NUM_WAVEFORMS 4

using namespace daisy;
using namespace daisysp;

DaisyPod   hw;
Oscillator osc[4];
Parameter  p_freq;
int notes[4];
int chord[10][3];
int chordNum = 0;

void UpdateControls();

static void AudioCallback(float *in, float *out, size_t size)
{
    UpdateControls();

    for (int i = 0; i < 4; i++)
    {
	osc[i].SetFreq(mtof(notes[i]));
    }
    
    // Audio Loop
    for(size_t i = 0; i < size; i += 2)
    {
        float sig = 0;
	for (int i = 0; i < 4; i++)
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

    for (int i = 0; i < 4; i++)
    {
	osc[i].Init(samplerate);
	osc[i].SetAmp(0.1f);
	osc[i].SetWaveform(Oscillator::WAVE_SIN);
	notes[i] = 30;
    }
}

void InitChords()
{
  // 0    1    2     3
  // Maj, min, Aug, Dim
  //  4     5     6      7
  // Maj7, min7, dom7, min/Maj7
  //  8        9
  // dim7, half dim7

  //set thirds
  for (int i = 0; i < 8; i++)
  {
      //every other chord, maj third, min third
      chord[i][0] = 3 + ((i + 1) % 2);
  }
  chord[8][0] = chord[9][0] = 3;

  //set fifths
  // perfect
  chord[0][1] = chord[1][1] = chord[4][1] = chord[5][1] = chord[6][1] = chord[7][1] = 7;
  // diminished
  chord[3][1] = chord[8][1] = chord[9][1] = 6;
  // augmented
  chord[2][1] = 8;

  //set sevenths
  // triads
  chord[0][2] = chord[1][2] = chord[2][2] = chord[3][2] = 0;
  // major
  chord[4][2] = chord[7][2] = 11;
  // minor
  chord[5][2] = chord[6][2] = chord[9][2] = 10;
  // diminished
  chord[8][2] = 9;
}

int main(void)
{
    float samplerate;

    // Init everything
    hw.Init();
    samplerate = hw.AudioSampleRate();

    InitSynth(samplerate);
    InitChords();
    
    // start callbacks
    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    while(1) {}
}

void UpdateEncoder()
{
    if (hw.encoder.RisingEdge())
    {
        chordNum = 0;
    }
  
    chordNum += hw.encoder.Increment();   
    chordNum = (chordNum % 10 + 10) % 10;
}

void UpdateKnobs()
{
    notes[0] = p_freq.Process();
    notes[1] = notes[0] + chord[chordNum][0];
    notes[2] = notes[0] + chord[chordNum][1];
    notes[3] = notes[0] + chord[chordNum][2];
}

void UpdateControls()
{
    hw.UpdateAnalogControls();
    hw.DebounceControls();
  
    UpdateEncoder();
    UpdateKnobs();
}
