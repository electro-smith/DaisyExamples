#include "daisysp.h"
#include "daisy_patch.h"
using namespace daisy;
using namespace daisysp;
static DaisyPatch patch;
static ReverbSc verb;
static DcBlock blk[2];
static float drylevel, send;

static void VerbCallback(float **in, float **out, size_t size)
{
    float dryL, dryR, wetL, wetR, sendL, sendR;
    patch.UpdateAnalogControls();
    for (size_t i = 0; i < size; i++)
    {
        // read some controls
        drylevel = patch.GetCtrlValue(patch.CTRL_1);
        send     = patch.GetCtrlValue(patch.CTRL_2);

        // Read Inputs (only stereo in are used)
        dryL = in[0][i];
        dryR = in[1][i];

        // Send Signal to Reverb
        sendL = dryL * send;
        sendR = dryR * send;
        verb.Process(sendL, sendR, &wetL, &wetR);

        // Dc Block
        wetL = blk[0].Process(wetL);
        wetR = blk[1].Process(wetR);

        // Out 1 and 2 are Mixed 
        out[0][i] = (dryL * drylevel) + wetL;
        out[1][i] = (dryR * drylevel) + wetR;

        // Out 3 and 4 are just wet
        out[2][i] = wetL;
        out[3][i] = wetR;
    }
}

int main(void)
{
    float samplerate;
    patch.Init();
    samplerate = patch.AudioSampleRate();

    verb.Init(samplerate);
    verb.SetFeedback(0.85f);
    verb.SetLpFreq(18000.0f);

    blk[0].Init(samplerate);
    blk[1].Init(samplerate);

    patch.StartAdc();
    patch.StartAudio(VerbCallback);

    while(1) {}
}
