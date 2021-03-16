//include "daisy_patch.h"
//#include "daisysp.h"

#include "cv_scaler.h"
#include "codec.h"
#include "debug_pin.h"
#include "debug_port.h"
#include "system.h"
#include "version.h"
#include "granular_processor.h"
#include "meter.h"
#include "resources.h"
#include "settings.h"
#include "ui.h"

//using namespace daisy;
//using namespace daisysp;

//DaisyPatch hw;
void AudioCallback(float **in, float **out, size_t size)
{
	//hw.ProcessAllControls();
	for (size_t i = 0; i < size; i++)
	{
		out[0][i] = in[0][i];
		out[1][i] = in[1][i];
		out[2][i] = in[2][i];
		out[3][i] = in[3][i];
	}
}

int main(void)
{
	//hw.Init();
	//hw.StartAdc();
	//hw.StartAudio(AudioCallback);
	while(1) {}
}
