#include "daisy_patch.h"
#include "daisysp.h"

#include <string>

using namespace std;
using namespace daisy;
using namespace daisysp;

DaisyPatch hw;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
	hw.ProcessAllControls();
	for (size_t i = 0; i < size; i++) {
		out[0][i] = in[0][i];
		out[1][i] = in[1][i];
		out[2][i] = in[2][i];
		out[3][i] = in[3][i];
	}
}

void UpdateControls() {
}

void DisplayLines(const vector<string> &strs) {
	int line_num = 0;
	for (string str : strs) {
		char* cstr = &str[0];
		hw.display.SetCursor(0, line_num*10);
		hw.display.WriteString(cstr, Font_7x10, true);
		line_num++;
	}
}

void UpdateDisplay() {
	hw.display.Fill(false);
	vector<string> strs;

	strs.push_back("foo");
	strs.push_back("bar");

	DisplayLines(strs);
	hw.display.Update();
}

int main(void) {
	hw.Init();
	hw.SetAudioBlockSize(4); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
	hw.StartAdc();
	hw.StartAudio(AudioCallback);

	while(true) {
		for (size_t i = 0; i < 100; i++) {
			UpdateControls();
		}
		UpdateDisplay();
	}
}
