#include "daisy_patch.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;
using namespace std;

DaisyPatch patch;

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {
	for (size_t i = 0; i < size; i++) {
		out[0][i] = in[0][i];
		out[1][i] = in[1][i];
		out[2][i] = in[2][i];
		out[3][i] = in[3][i];
	}
}

void UpdateControls() {
  patch.ProcessAllControls();
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

    float x = 3.14;
    FixedCapStr<20> str("");
    str.AppendFloat(x, 2);
    str.Append(" ");
    str.AppendFloat(x, 2);
    strs.push_back(string(str));

    DisplayLines(strs);
    patch.display.Update();
}

int main(void) {
	patch.Init();
	patch.StartAdc();
	patch.StartAudio(AudioCallback);

	while(true) {
        UpdateControls();
        UpdateDisplay();
    }
}
