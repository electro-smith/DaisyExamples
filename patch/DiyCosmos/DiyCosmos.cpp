#include "daisy_patch.h"
#include "daisysp.h"

#include <string>

using namespace daisy;
using namespace daisysp;
using namespace std;

const size_t BLOCK_SIZE = 16;

using FloatBlock = float[BLOCK_SIZE];

DaisyPatch patch;

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
	strs.push_back("foo");
	strs.push_back("bar");
	DisplayLines(strs);
	patch.display.Update();
}

int main(void) {
	patch.Init();
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
