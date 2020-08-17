#include "daisysp.h"
#include "daisy_petal.h"

using namespace daisysp;
using namespace daisy;

static DaisyPetal petal;

float clip(float in)
{
	in = in > 1.f ? 1.f : in;
	in = in < -1.f ? -1.f : in;
	return in;
}

static void AudioCallback(float **in, float **out, size_t size)
{
	petal.UpdateAnalogControls();
	
	float gain = petal.knob[0].Process() * 100 + 10;
	float drywet = petal.knob[1].Process();
	
    for(size_t i = 0; i < size; i ++)
    {	
		float wet = in[0][i];
		
		wet *= gain;
		wet = clip(wet);
		
		if (wet > 0)
			out[0][i] = out[1][i] = 1 - expf(-wet);
		else
			out[0][i] = out[1][i] = -1 + expf(wet);
		
		out[0][i] = wet * drywet + in[0][i] * (1 - drywet);
    }

}

int main(void)
{
	petal.Init();
	
    // start callback
    petal.StartAdc();
    petal.StartAudio(AudioCallback);
    while(1) 
    {
    }
}