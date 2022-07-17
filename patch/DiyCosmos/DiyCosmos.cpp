#include "daisy_patch.h"
#include "daisysp.h"

#include <string>

using namespace daisy;
using namespace daisysp;
using namespace std;

const size_t BLOCK_SIZE = 16;

using FloatBlock = float[BLOCK_SIZE];

DaisyPatch patch;
Parameter ctrls[4];

float wet_dry_mix;

void AudioCallback(AudioHandle::InputBuffer in, 
				   AudioHandle::OutputBuffer out, 
				   size_t size) {	
	for (size_t i = 0; i < size; i++) {
		for (size_t c = 0; c < 4; c++) {
			out[c][i] = in[c][i];
		}
	}
}

void UpdateControls() {
	patch.ProcessAllControls();
	
	wet_dry_mix = ctrls[0].Process();
	if (wet_dry_mix < 0.02) {
		wet_dry_mix = 0;
	} else if (wet_dry_mix > 0.98) {
		wet_dry_mix = 1;
	}

	if (patch.encoder.RisingEdge()) {
		// reset loops!
  	}
}

void DisplayLines(const vector<string> &strs) {
	int line_num = 0;
	for (string str : strs) {
		char* cstr = &str[0];
		patch.display.SetCursor(0, line_num*10);
		patch.display.WriteString(cstr, Font_7x10, true);
		line_num++;
	}
}

void UpdateDisplay() {
	patch.display.Fill(false);
	vector<string> strs;

	FixedCapStr<20> str("");
	str.AppendFloat(wet_dry_mix, 2);
	strs.push_back("wet/dry " + string(str));

	DisplayLines(strs);
	patch.display.Update();
}

int main(void) {
	patch.Init();

	// ctrl 0 is wet / dry mix
	ctrls[0].Init(patch.controls[0], 0.0f, 1.0f, Parameter::LINEAR);



	patch.SetAudioBlockSize(BLOCK_SIZE);
	patch.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
	patch.StartAdc();
	patch.StartAudio(AudioCallback);
	while(true) {
		for (size_t i = 0; i < 100; i++) {
			UpdateControls();
		}
		UpdateDisplay();
	}
}
