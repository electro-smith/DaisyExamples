#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "Looper.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

#define MAX_LOOP_LEN 48000 * 10

DaisyPatchSM                              hw;
static Looper<float, MAX_LOOP_LEN> DSY_SDRAM_BSS looper;
Switch                                    sw;

bool writeOn;
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    sw.Debounce();

    if(sw.RisingEdge()){
    	writeOn = !writeOn;
        looper.Reset();
    }

    if(looper.EndOfLoop()){
        writeOn = false;
    }

    for(size_t i = 0; i < size; i++)
    {
        float in_mixed = (IN_L[i] + IN_R[i]) * .5f;

        if(writeOn){
        	looper.Write(in_mixed);
        }

        OUT_L[i] = OUT_R[i] = (looper.Read() + in_mixed) * .5f;
    }
}

int main(void)
{
    hw.Init();
    int   cbrate     = hw.AudioCallbackRate();
    float samplerate = hw.AudioSampleRate();

    writeOn = false;

    /* initialize the switch
	 - We'll read the switch on pin B7
	 - The switch will be read at the callback rate
	 - We're using a momentary switch
	 - We'll need to invert the signal
	 - We'll use the internal pullup
	*/
    sw.Init(hw.B7,
    		cbrate,
    		Switch::Type::TYPE_MOMENTARY,
    		Switch::Polarity::POLARITY_INVERTED,
    		Switch::Pull::PULL_UP);

    looper.Init(samplerate);

    hw.StartAudio(AudioCallback);
    while(1)
    {
        hw.SetLed(writeOn);
    }
}
