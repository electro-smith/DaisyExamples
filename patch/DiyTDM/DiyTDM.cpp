#include "daisy_patch.h"
#include "daisysp.h"

#include <string>

using namespace std;
using namespace daisy;
using namespace daisysp;

DaisyPatch hw;

const int BIT_RATE = pow(2, 8);

inline int to_int(float in_v) {
  // scale from (-1, 1) to (0, bitrate) and cast to int
  return (int)((in_v + 1) / 2 * BIT_RATE);
}

inline float to_float(int in_v) {
  // cast to float and rescale back to (-1, 1)
  return ((float)(in_v) / BIT_RATE * 2) - 1;
}

float bitflip(float in_v, int bit) {
	// cast to int
	int int_v = to_int(in_v);
	// create mask to flip bit
	int mask = 0;
	mask |= 1 << bit;
	// flip bits
	int_v ^= mask;
	// return as float
	return to_float(int_v);
}

inline float bit_crush(float in_v, int rate) {
	const int bit_rate = pow(2, rate);
    // scale from (-1, 1) to (0, bitrate)
    in_v = (in_v + 1) / 2 * bit_rate;
    // cast to int to float
    in_v = (float)((int)(in_v));
    // rescale back to (-1, 1)
    return (in_v / bit_rate * 2) - 1;
}


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
	hw.ProcessAllControls();
	for (size_t i = 0; i < size; i++) {
		out[0][i] = bitflip(in[0][i], 0);
		out[1][i] = bitflip(in[0][i], 2);
		out[2][i] = bitflip(in[0][i], 4);
		out[3][i] = bitflip(in[0][i], 6);
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

	strs.push_back("flip_some_bits");

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
