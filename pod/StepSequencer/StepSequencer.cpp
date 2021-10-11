#include "daisy_pod.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPod pod;

Oscillator osc;
AdEnv      env;
Parameter  decayTimeParam, pitchParam, tickFreqParam, filterFreqParam;
Metro      tick;
MoogLadder flt;

bool    edit; //if we aren't editing, we're playing
bool    editCycle;
uint8_t step;
uint8_t wave;
float   dec[8];
float   pitch[8];
bool    active[8];
float   env_out;

Color colors[8];
float pent[] = {110.f, 128.33f, 146.66f, 174.166f, 192.5f};

float oldk1, oldk2;
float tickFrequency, filterFrequency;
float k1, k2;

void Controls();

void NextSamples(float &sig);

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float sig;

    Controls();

    // Audio Loop
    sig = 0;
    for(size_t i = 0; i < size; i += 2)
    {
        NextSamples(sig);

        out[i]     = sig;
        out[i + 1] = sig;
    }
}

int main(void)
{
    float sample_rate;
    pod.Init();
    pod.SetAudioBlockSize(4);
    sample_rate     = pod.AudioSampleRate();
    tickFrequency   = 3.f;
    filterFrequency = 20000.f;
    k1 = k2 = 0.f;

    osc.Init(sample_rate);
    env.Init(sample_rate);
    tick.Init(3, sample_rate);
    flt.Init(sample_rate);

    //Set up parameters
    decayTimeParam.Init(pod.knob1, .03, 1, decayTimeParam.LINEAR);
    pitchParam.Init(pod.knob2, 0, 10, pitchParam.LINEAR);
    tickFreqParam.Init(pod.knob1, 4, 13, tickFreqParam.LINEAR);
    filterFreqParam.Init(pod.knob2, 100, 10000, filterFreqParam.LOGARITHMIC);

    //Osc parameters
    osc.SetWaveform(osc.WAVE_TRI);

    //Envelope parameters
    env.SetTime(ADENV_SEG_ATTACK, 0.01);
    env.SetMin(0.0);
    env.SetMax(1);

    //Set filter parameters
    flt.SetFreq(20000);
    flt.SetRes(0.7);

    //Global variables
    oldk1 = oldk2 = 0;
    edit          = true;
    editCycle     = false;
    step          = 0;
    env_out       = 0;
    wave          = 2;

    for(int i = 0; i < 8; i++)
    {
        dec[i]    = .5;
        active[i] = false;
        pitch[i]  = 110;
        colors[i].Init((Color::PresetColor)i);
    }
    colors[7].Init(1, 1, 0);

    pod.StartAdc();
    pod.StartAudio(AudioCallback);

    while(1) {}
}

void ConditionalParameter(float o, float n, float &param, float update)
{
    if(abs(o - n) > 0.00005)
    {
        param = update;
    }
}

void UpdateEncoderPressed()
{
    //switch modes
    if(pod.encoder.RisingEdge())
    {
        edit = !edit;
        step = 0;
    }
    if(edit == false)
    {
        editCycle = false;
    }
}

void UpdateEncoderIncrement()
{
    if(edit)
    {
        step += pod.encoder.Increment();
        step = (step % 8 + 8) % 8;
    }
    else
    {
        wave += pod.encoder.Increment();
        wave = ((wave % 2 + 2) % 2);
        osc.SetWaveform(wave + 3);
    }
}

void UpdateButtons()
{
    if(edit)
    {
        if(pod.button2.RisingEdge())
        {
            active[step] = !active[step];
        }
        if(pod.button1.RisingEdge())
        {
            editCycle = !editCycle;
        }
    }
}

void UpdateKnobs()
{
    k1 = pod.knob1.Process();
    k2 = pod.knob2.Process();

    if(edit)
    {
        ConditionalParameter(oldk1, k1, dec[step], decayTimeParam.Process());

        int temp = (int)pitchParam.Process();
        ConditionalParameter(
            oldk2, k2, pitch[step], pent[temp % 5] * (temp / (int)5 + 1));
        osc.SetFreq(pitch[step]);
    }

    else
    {
        ConditionalParameter(oldk1, k1, tickFrequency, tickFreqParam.Process());
        ConditionalParameter(
            oldk2, k2, filterFrequency, filterFreqParam.Process());

        tick.SetFreq(tickFrequency);
        flt.SetFreq(filterFrequency);
    }

    oldk1 = k1;
    oldk2 = k2;
}

void UpdateLedEdit()
{
    Color cur_color = colors[step];
    pod.led1.SetColor(cur_color);

    if(active[step])
    {
        pod.led2.SetColor(cur_color);
    }

    else
    {
        pod.led2.Set(0, 0, 0);
    }

    pod.UpdateLeds();
}

void UpdateLeds()
{
    if(edit)
    {
        UpdateLedEdit();
    }
    else
    {
        pod.led2.Set(0, env_out, 0);
        pod.led1.Set(0, env_out, 0);
    }
    pod.UpdateLeds();
}

void Controls()
{
    pod.ProcessDigitalControls();
    pod.ProcessAnalogControls();

    UpdateEncoderPressed();

    UpdateEncoderIncrement();

    UpdateButtons();

    UpdateKnobs();

    UpdateLeds();
}

void NextSamples(float &sig)
{
    env_out = env.Process();
    osc.SetAmp(env_out);
    sig = osc.Process();
    sig = flt.Process(sig);

    if(tick.Process() && !edit)
    {
        step++;
        step %= 8;
        if(active[step])
        {
            env.Trigger();
        }
    }

    if(active[step])
    {
        env.SetTime(ADENV_SEG_DECAY, dec[step]);
        osc.SetFreq(pitch[step]);
    }
    if(!env.IsRunning() && editCycle)
    {
        env.Trigger();
    }
}
