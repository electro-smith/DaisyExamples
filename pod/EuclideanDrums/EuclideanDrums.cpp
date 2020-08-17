#include "daisy_pod.h"
#include "daisysp.h"

#define UINT32_MSB 0x80000000U
#define MAX_LENGTH 32U

using namespace daisy;
using namespace daisysp;

DaisyPod hardware;

Oscillator osc;
WhiteNoise noise;

AdEnv kickVolEnv, kickPitchEnv, snareEnv;
Metro tick;
Parameter lengthParam;

uint32_t kickSeq =  0b10001000100010001000100010001000U;
uint32_t snareSeq = 0b00001000000010010000100001101101U;
uint8_t kickStep = 0;
uint8_t snareStep = 0;

void ProcessTick();

void ProcessControls();

uint32_t Bjork(uint32_t* array, int arrayLen);

void SetArray(uint32_t* array, int zeroes, int ones);

void AudioCallback(float* in, float* out, size_t size)
{
    float osc_out, noise_out, snr_env_out, kck_env_out, sig;

    ProcessTick();

    ProcessControls();

    //audio
    for (size_t i = 0; i < size; i+= 2)
    {
        snr_env_out = snareEnv.Process();
	kck_env_out = kickVolEnv.Process();

	osc.SetFreq(kickPitchEnv.Process());
	osc.SetAmp(kck_env_out);
	osc_out = osc.Process();

	noise_out = noise.Process();
	noise_out *= snr_env_out;

	sig = .5 * noise_out + .5 + osc_out;

        out[i] = sig;
	out[i+1] = sig;
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

int main(void)
{
    hardware.Init();
    float samplerate = hardware.AudioSampleRate();
    float callbackrate = hardware.AudioCallbackRate();
    
    //setup the drum sounds
    SetupDrums(samplerate);
    
    tick.Init(5, callbackrate);

    lengthParam.Init(hardware.knob2, 1, 33, lengthParam.LINEAR);
    
    hardware.StartAdc();
    hardware.StartAudio(AudioCallback);
    
    // Loop forever
    for(;;) {}
}

float snareLength = 32.f;
float kickLength = 32.f;
void IncrementSteps(int& snareOffset, int& kickOffset)
{
    snareStep++;
    kickStep++;
    snareStep %= (int)snareLength;
    snareOffset = 32 - snareLength;
    
    kickStep %= (int)kickLength;
    kickOffset = 32 - kickLength; 
}

void ProcessTick()
{
    if (tick.Process())
    {
        int snareOffset, kickOffset;
	IncrementSteps(snareOffset, kickOffset);
	
	if ((kickSeq << (kickStep + kickOffset)) & UINT32_MSB)
	{
	    kickVolEnv.Trigger();
	    kickPitchEnv.Trigger();
	}
	
	if ((snareSeq << (snareStep + snareOffset)) & UINT32_MSB)
        {
	    snareEnv.Trigger();
	}
    }
    
}

uint32_t Msb(uint32_t number)
{
    for (int i = 15; i >=0; i--)
    {
        if ((number >> i) & 1)
	{
	    return (uint32_t)(i + 1);
	}
    }
    return 1;
}

uint32_t Bjork(uint32_t* array, int arrayLen)
{
    if (arrayLen == 1)
    {
        return array[0];
    }
    
    for (int i = 0; i < arrayLen / 2; i++)
    {
        uint32_t secondLen = Msb(array[arrayLen - i - 1]);
        array[i] = (array[i] << secondLen) + array[arrayLen - i - 1];
    }
    
    int odd = arrayLen % 2;
    return Bjork(array, arrayLen / 2 + odd);
}

void SetArray(uint32_t* array, int zeroes, int ones)
{
    for (int i = 0; i < ones; i++)
    {
	array[i] = 1;
    }
    for (int i = 0; i < zeroes; i++)
    {
	array[i + ones] = 0;
    }
}

uint8_t mode = 0;
void UpdateEncoder()
{
    mode += hardware.encoder.Increment();
    mode = (mode % 2 + 2) % 2;
    hardware.led2.Set(0, !mode, mode);
}

void ConditionalParameter(float o, float n, float &param, float update)
{
    if (abs(o - n) > 0.0005)
    {
        param = update;
    }
}

float snareAmount, kickAmount = 0.f;
float k1old, k2old = 0.f;
void UpdateKnobs()
{
    float k1 = hardware.knob1.Process();
    float k2 = hardware.knob2.Process();
    	
    if (mode)
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
void UpdateButtons()
{
    if (hardware.button1.Pressed())
    {
        tempo += .003;
	
    }
    
    else if (hardware.button2.Pressed())
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
    uint32_t snareArray[(int)snareLength];
    uint32_t Ones = (uint32_t)round(snareAmount * snareLength);
    uint32_t Zeroes = (int)snareLength - Ones;
    SetArray(snareArray, Zeroes, Ones);
    snareSeq = Bjork(snareArray, (int)snareLength);
    
    uint32_t kickArray[(int)kickLength];
    Ones = (uint32_t)round(kickAmount * kickLength);
    Zeroes = (int)kickLength - Ones;
    SetArray(kickArray, Zeroes, Ones);
    kickSeq = Bjork(kickArray, (int)kickLength);
}

void ProcessControls()
{
    hardware.DebounceControls();
    hardware.UpdateAnalogControls();

    UpdateEncoder();
    
    UpdateKnobs();

    UpdateButtons();

    UpdateVars();

    hardware.UpdateLeds();
}

