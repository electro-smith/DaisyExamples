#include "daisy_petal.h"
#include "daisysp.h" 

using namespace daisy;
using namespace daisysp;

DaisyPetal petal;
PitchShifter psl ,psr;
Oscillator lfo;
CrossFade fader;

bool bypassOn;

Parameter lfoFreqParam, lfoAmpParam;
Parameter shiftTransParam, shiftDelParam;
Parameter faderPosParam;

float trans, transTarget;
float delsize, delsizeTarget;

void UpdateControls();

void AudioCallback(float **in, float **out, size_t size)
{
  UpdateControls();
  
  for (size_t i = 0; i < size; i++)
  {
    fonepole(trans, transTarget, .0005f);
    psl.SetTransposition(trans + lfo.Process());
    psr.SetTransposition(trans + lfo.Process());

    fonepole(delsize, delsizeTarget, .001f);
    psl.SetDelSize(delsize);
    psr.SetDelSize(delsize);

    
    float inl  = in[0][i];
    float processl = psl.Process(inl);

    float inr  = in[1][i];
    float processr = psr.Process(inr);
    
    out[0][i] = fader.Process(inl, processl); 
    out[1][i] = fader.Process(inr, processr); 
  }
}

int main(void)
{
    float samplerate;
    petal.Init();
    samplerate = petal.AudioSampleRate();

    lfoFreqParam.Init(petal.knob[0], .1, 20, Parameter::EXPONENTIAL);
    lfoAmpParam.Init(petal.knob[1], 0, 12, Parameter::EXPONENTIAL);	
    shiftTransParam.Init(petal.knob[2], -12, 12, Parameter::LINEAR);
    shiftDelParam.Init(petal.knob[3], 0, (SHIFT_BUFFER_SIZE - 256), Parameter::LINEAR);
    faderPosParam.Init(petal.knob[4], 0, 1, Parameter::LINEAR);
    
    lfo.Init(samplerate);
    lfo.SetWaveform(Oscillator::WAVE_SIN);
    
    psl.Init(samplerate);
    psr.Init(samplerate);
    
    fader.Init();
    
    petal.StartAdc();
    petal.StartAudio(AudioCallback);

    int i = 0;
    while(1) 
    {
      petal.ClearLeds();
      
      petal.SetFootswitchLed((DaisyPetal::FootswitchLed)0, !bypassOn);

      petal.SetRingLed((DaisyPetal::RingLed)i , 0, 1, 0);
      i++;
      i %= 8;
      
      petal.UpdateLeds();
      dsy_system_delay(60);
    }
}


void UpdateControls()
{
  petal.UpdateAnalogControls();
  petal.DebounceControls();

  //knobs
  lfo.SetFreq(lfoFreqParam.Process());
  lfo.SetAmp(lfoAmpParam.Process());

  delsizeTarget = 256 + shiftDelParam.Process();
  
  transTarget = shiftTransParam.Process();
    
  fader.SetPos(faderPosParam.Process());
  if (bypassOn)
  {
    fader.SetPos(0);
  }
	
  //bypass switch
  if (petal.switches[0].RisingEdge())
  {
    bypassOn = !bypassOn;
  }
}
