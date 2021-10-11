#include "daisysp.h"
#include "daisy_petal.h"

#define MAX_SIZE (48000 * 60 * 5) // 5 minutes of floats at 48 khz

using namespace daisysp;
using namespace daisy;

static DaisyPetal petal;

bool first = true;  //first loop (sets length)
bool rec   = false; //currently recording
bool play  = false; //currently playing

int                 pos = 0;
float DSY_SDRAM_BSS buf[MAX_SIZE];
int                 mod    = MAX_SIZE;
int                 len    = 0;
float               drywet = 0;
bool                res    = false;

void ResetBuffer();
void Controls();

void NextSamples(float&                               output,
                 AudioHandle::InterleavingInputBuffer in,
                 size_t                               i);

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float output = 0;

    Controls();

    for(size_t i = 0; i < size; i += 2)
    {
        NextSamples(output, in, i);

        // left and right outs
        out[i] = out[i + 1] = output;
    }
}

int main(void)
{
    // initialize petal hardware and oscillator daisysp module

    petal.Init();
    petal.SetAudioBlockSize(4);
    ResetBuffer();

    // start callback
    petal.StartAdc();
    petal.StartAudio(AudioCallback);

    while(1)
    {
        //leds
        petal.SetFootswitchLed((DaisyPetal::FootswitchLed)1, play);
        petal.SetFootswitchLed((DaisyPetal::FootswitchLed)0, rec);
        petal.UpdateLeds();
        System::Delay(16); // 60Hz
    }
}

//Resets the buffer
void ResetBuffer()
{
    play  = false;
    rec   = false;
    first = true;
    pos   = 0;
    len   = 0;
    for(int i = 0; i < mod; i++)
    {
        buf[i] = 0;
    }
    mod = MAX_SIZE;
}

void UpdateButtons()
{
    //switch1 pressed
    if(petal.switches[0].RisingEdge())
    {
        if(first && rec)
        {
            first = false;
            mod   = len;
            len   = 0;
        }

        res  = true;
        play = true;
        rec  = !rec;
    }

    //switch1 held
    if(petal.switches[0].TimeHeldMs() >= 1000 && res)
    {
        ResetBuffer();
        res = false;
    }

    //switch2 pressed and not empty buffer
    if(petal.switches[1].RisingEdge() && !(!rec && first))
    {
        play = !play;
        rec  = false;
    }
}

//Deals with analog controls
void Controls()
{
    petal.ProcessAnalogControls();
    petal.ProcessDigitalControls();

    drywet = petal.knob[0].Process();

    UpdateButtons();
}

void WriteBuffer(AudioHandle::InterleavingInputBuffer in, size_t i)
{
    buf[pos] = buf[pos] * 0.5 + in[i] * 0.5;
    if(first)
    {
        len++;
    }
}

void NextSamples(float&                               output,
                 AudioHandle::InterleavingInputBuffer in,
                 size_t                               i)
{
    if(rec)
    {
        WriteBuffer(in, i);
    }

    output = buf[pos];

    //automatic looptime
    if(len >= MAX_SIZE)
    {
        first = false;
        mod   = MAX_SIZE;
        len   = 0;
    }

    if(play)
    {
        pos++;
        pos %= mod;
    }

    if(!rec)
    {
        output = output * drywet + in[i] * (1 - drywet);
    }
}
