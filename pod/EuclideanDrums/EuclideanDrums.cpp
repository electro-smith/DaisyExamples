#include "daisysp.h"
#include "daisy_pod.h"

#define UINT32_MSB 0x80000000U
#define MAX_LENGTH 32U

using namespace daisy;
using namespace daisysp;

DaisyPod hardware;

Oscillator osc;
WhiteNoise noise;

AdEnv     kickVolEnv, kickPitchEnv, snareEnv;
Metro     tick;
Parameter lengthParam;

bool    kickSeq[MAX_LENGTH];
bool    snareSeq[MAX_LENGTH];
uint8_t kickStep  = 0;
uint8_t snareStep = 0;

void ProcessTick();
void ProcessControls();

void AudioCallback(float* in, float* out, size_t size)
{
    float osc_out, noise_out, snr_env_out, kck_env_out, sig;

    ProcessTick();

    ProcessControls();

    //audio
    for(size_t i = 0; i < size; i += 2)
    {
        snr_env_out = snareEnv.Process();
        kck_env_out = kickVolEnv.Process();

        osc.SetFreq(kickPitchEnv.Process());
        osc.SetAmp(kck_env_out);
        osc_out = osc.Process();

        noise_out = noise.Process();
        noise_out *= snr_env_out;

        sig = .5 * noise_out + .5 + osc_out;

        out[i]     = sig;
        out[i + 1] = sig;
    }
}

void SetupDrums(float samplerate)
{
    osc.Init(samplerate);
    osc.SetWaveform(Oscillator::WAVE_TRI);
    osc.SetAmp(1);

    noise.Init();

    snareEnv.Init(samplerate);
    snareEnv.SetTime(ADENV_SEG_ATTACK, .01);
    snareEnv.SetTime(ADENV_SEG_DECAY, .2);
    snareEnv.SetMax(1);
    snareEnv.SetMin(0);

    kickPitchEnv.Init(samplerate);
    kickPitchEnv.SetTime(ADENV_SEG_ATTACK, .01);
    kickPitchEnv.SetTime(ADENV_SEG_DECAY, .05);
    kickPitchEnv.SetMax(400);
    kickPitchEnv.SetMin(50);

    kickVolEnv.Init(samplerate);
    kickVolEnv.SetTime(ADENV_SEG_ATTACK, .01);
    kickVolEnv.SetTime(ADENV_SEG_DECAY, 1);
    kickVolEnv.SetMax(1);
    kickVolEnv.SetMin(0);
}

void SetSeq(bool* seq, bool in)
{
    for(uint32_t i = 0; i < MAX_LENGTH; i++)
    {
        seq[i] = in;
    }
}

int main(void)
{
    hardware.Init();
    float samplerate   = hardware.AudioSampleRate();
    float callbackrate = hardware.AudioCallbackRate();

    //setup the drum sounds
    SetupDrums(samplerate);

    tick.Init(5, callbackrate);

    lengthParam.Init(hardware.knob2, 1, 32, lengthParam.LINEAR);

    SetSeq(snareSeq, 0);
    SetSeq(kickSeq, 0);

    hardware.StartAdc();
    hardware.StartAudio(AudioCallback);

    // Loop forever
    for(;;) {}
}

float snareLength = 32.f;
float kickLength  = 32.f;

void IncrementSteps()
{
    snareStep++;
    kickStep++;
    snareStep %= (int)snareLength;
    kickStep %= (int)kickLength;
}

void ProcessTick()
{
    if(tick.Process())
    {
        IncrementSteps();

        if(kickSeq[kickStep])
        {
            kickVolEnv.Trigger();
            kickPitchEnv.Trigger();
        }

        if(snareSeq[snareStep])
        {
            snareEnv.Trigger();
        }
    }
}

void SetArray(bool* seq, int arrayLen, float density)
{
    SetSeq(seq, 0);

    int ones  = (int)round(arrayLen * density);
    int zeros = arrayLen - ones;

    if(ones == 0)
        return;

    if(zeros == 0)
    {
        SetSeq(seq, 1);
        return;
    }

    int oneArr[ones];
    int zeroArr[ones];

    for(int i = 0; i < ones; i++)
    {
        oneArr[i] = zeroArr[i] = 0;
    }

    //how many zeroes per each one
    int idx = 0;
    for(int i = 0; i < zeros; i++)
    {
        zeroArr[idx] += 1;
        idx++;
        idx %= ones;
    }

    //how many ones remain
    int rem = 0;
    for(int i = 0; i < ones; i++)
    {
        if(zeroArr[i] == 0)
            rem++;
    }

    //how many ones on each prior one
    idx = 0;
    for(int i = 0; i < rem; i++)
    {
        oneArr[idx] += 1;
        idx++;
        idx %= ones - rem;
    }

    //fill the global seq
    idx = 0;
    for(int i = 0; i < (ones - rem); i++)
    {
        seq[idx] = 1;
        idx++;

        for(int j = 0; j < zeroArr[i]; j++)
        {
            seq[idx] = 0;
            idx++;
        }

        for(int j = 0; j < oneArr[i]; j++)
        {
            seq[idx] = 1;
            idx++;
        }
    }
}

uint8_t mode = 0;
void    UpdateEncoder()
{
    mode += hardware.encoder.Increment();
    mode = (mode % 2 + 2) % 2;
    hardware.led2.Set(0, !mode, mode);
}

void ConditionalParameter(float o, float n, float& param, float update)
{
    if(abs(o - n) > 0.0005)
    {
        param = update;
    }
}

float snareAmount, kickAmount = 0.f;
float k1old, k2old            = 0.f;
void  UpdateKnobs()
{
    float k1 = hardware.knob1.Process();
    float k2 = hardware.knob2.Process();

    if(mode)
    {
        ConditionalParameter(k1old, k1, snareAmount, k1);
        ConditionalParameter(k2old, k2, snareLength, lengthParam.Process());
    }
    else
    {
        ConditionalParameter(k1old, k1, kickAmount, k1);
        ConditionalParameter(k2old, k2, kickLength, lengthParam.Process());
    }

    k1old = k1;
    k2old = k2;
}

float tempo = 3;
void  UpdateButtons()
{
    if(hardware.button2.Pressed())
    {
        tempo += .003;
    }

    else if(hardware.button1.Pressed())
    {
        tempo -= .003;
    }

    tempo = std::fminf(tempo, 10.f);
    tempo = std::fmaxf(.5f, tempo);

    tick.SetFreq(tempo);
    hardware.led1.Set(tempo / 10.f, 0, 0);
}

void UpdateVars()
{
    SetArray(snareSeq, (int)round(snareLength), snareAmount);
    SetArray(kickSeq, (int)round(kickLength), kickAmount);
}

void ProcessControls()
{
    hardware.ProcessDigitalControls();
    hardware.ProcessAnalogControls();

    UpdateEncoder();

    UpdateKnobs();

    UpdateButtons();

    UpdateVars();

    hardware.UpdateLeds();
}
