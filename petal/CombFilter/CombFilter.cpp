#include "daisy_petal.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPetal petal;
Comb       comb;
Oscillator lfo;
CrossFade  fader;

bool  bypassOn;
float buf[9600];

Parameter lfoFreqParam, lfoAmpParam;
Parameter combFreqParam, combRevParam;
Parameter faderPosParam;

float targetCombFreq, combFreq;

void UpdateControls();

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    UpdateControls();

    for(size_t i = 0; i < size; i++)
    {
        fonepole(combFreq, targetCombFreq, .0001f);
        float f = combFreq + lfo.Process() + 50.f;
        comb.SetFreq(f);

        float inf     = in[0][i];
        float process = comb.Process(in[0][i]);
        out[0][i] = out[1][i] = fader.Process(inf, process);
    }
}

int main(void)
{
    float samplerate;
    petal.Init();
    petal.SetAudioBlockSize(4);
    samplerate = petal.AudioSampleRate();

    lfoFreqParam.Init(petal.knob[0], 0, 2, Parameter::LINEAR);
    lfoAmpParam.Init(petal.knob[1], 0, 50, Parameter::LINEAR);
    combFreqParam.Init(petal.knob[2], 25, 300, Parameter::LOGARITHMIC);
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
        System::Delay(60);
    }
}


void UpdateControls()
{
    petal.ProcessDigitalControls();

    //knobs
    lfo.SetFreq(lfoFreqParam.Process());
    lfo.SetAmp(lfoAmpParam.Process());

    targetCombFreq = combFreqParam.Process();

    comb.SetRevTime(combRevParam.Process());

    fader.SetPos(faderPosParam.Process());
    if(bypassOn)
    {
        fader.SetPos(0);
    }

    //bypass switch
    if(petal.switches[0].RisingEdge())
    {
        bypassOn = !bypassOn;
    }
}
