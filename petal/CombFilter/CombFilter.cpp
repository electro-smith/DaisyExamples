#include "daisy_petal.h"
#include "daisysp.h" 

using namespace daisy;
using namespace daisysp;

DaisyPetal petal;
Comb comb;
Oscillator lfo;
CrossFade fader;

bool bypassOn;
float buf[9600];

Parameter lfoFreqParam, lfoAmpParam;
Parameter combFreqParam, combRevParam;
Parameter faderPosParam;

void UpdateControls();

void AudioCallback(float **in, float **out, size_t size)
{
  UpdateControls();
  
  for (size_t i = 0; i < size; i++)
  {
    comb.SetFreq(combFreqParam.Value() + lfo.Process());
    
    float inf  = in[0][i];
    float process = comb.Process(in[0][i]);
    out[0][i] = out[1][i] = fader.Process(inf, process); 
  }
}

int main(void)
{
    float samplerate;
    petal.Init();
    samplerate = petal.AudioSampleRate();

    lfoFreqParam.Init(petal.knob[0], 10, 20000, Parameter::LOGARITHMIC);
    lfoAmpParam.Init(petal.knob[1], 0, 10, Parameter::LINEAR);
    combFreqParam.Init(petal.knob[2], .01, 20, Parameter::LINEAR);
    combRevParam.Init(petal.knob[3], 0, 1, Parameter::LINEAR);
    faderPosParam.Init(petal.knob[4], 0, 1, Parameter::LINEAR);
    
    lfo.Init(samplerate);
    lfo.SetAmp(1);
    lfo.SetWaveform(Oscillator::WAVE_SIN);

    for(int i = 0; i < 9600; i++)
    {
        buf[i] = 0.0f;
    }

    // initialize Comb object
    comb.Init(samplerate, buf, 9600);

    fader.Init();

    petal.StartAdc();
    petal.StartAudio(AudioCallback);

    int i = 0;      
    while(1) 
    {
      petal.ClearLeds();

      petal.SetFootswitchLed((DaisyPetal::FootswitchLed)0, !bypassOn);

      petal.SetRingLed((DaisyPetal::RingLed)i, 0, 1, 1);
      i++;
      i %= 8;
      
      petal.UpdateLeds();
      dsy_system_delay(60);
    }
}

void UpdateControls()
{
  petal.DebounceControls();
	
  //knobs
  lfo.SetFreq(lfoFreqParam.Process());
  lfo.SetAmp(lfoAmpParam.Process());
  combFreqParam.Process();
  
  comb.SetRevTime(combRevParam.Process());
  
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
