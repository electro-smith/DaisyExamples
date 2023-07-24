#include "daisy_patch.h"
#include "daisysp.h"

#include <string>

using namespace std;
using namespace daisy;
using namespace daisysp;

#define BIT_SET(a,b) ((a) |= (1ULL<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1ULL<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1ULL<<(b)))
#define BIT_CHECK(a,b) (!!((a) & (1ULL<<(b))))        // '!!' to make sure this returns 0 or 1

DaisyPatch hw;
Parameter ctrl1;

const int NUM_BITS = 8;
const int MAX_VALUE = pow(2, NUM_BITS) - 1;

int bit_to_flip = 0;
float bit_to_flip_f = 0;

// float sig_min_v = 0;
// float sig_max_v = 0;

inline float clip(const float v) {
	if (v < -1.0) {
		return -1.0;
	} else if (v > 1.0) {
		return 1.0;
	} else {
		return v;
	}
}

inline int to_int(float in_v) {
	// map from (-1, 1) to (0, bitrate) as int
	in_v = clip(in_v);
	return (int)((in_v + 1) / 2 * MAX_VALUE);
}

inline float to_float(const int in_v) {
	// cast to float
	float val_f = (float)in_v;
	// and rescale back to (-1, 1)
	val_f = (val_f / MAX_VALUE * 2) - 1;
	//assert val_f >= -1.0;
	//assert val_f <= 1.0;
	return val_f;
}

inline float bitflip(float in_v, int bit) {
	// cast to int
	int int_v = to_int(in_v);
	// flip bit
	BIT_FLIP(int_v, bit);
	// return as float
	return to_float(int_v);
}


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
	for (size_t i = 0; i < size; i++) {
		const float in_v = in[0][i];
		// if (in_v < sig_min_v) { sig_min_v = in_v; }
		// if (in_v > sig_max_v) { sig_max_v = in_v; }
		out[0][i] = bitflip(in_v, 0);
		out[1][i] = bitflip(in_v, 2);
		out[2][i] = bitflip(in_v, 4);
		out[3][i] = bitflip(in_v, bit_to_flip);
	}
}

void UpdateControls() {
	hw.ProcessAllControls();

	// process ctrl1, value 0 to 1
	bit_to_flip_f = ctrl1.Process();

	// map to (0, NUM_BITS) for bit to flip
	bit_to_flip = (int)(bit_to_flip_f*NUM_BITS);
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

inline string int_to_string(const int v) {
	FixedCapStr<5> str("");
	str.AppendInt(v);
	return string(str);
}

inline string float_to_string(const float v) {
	FixedCapStr<5> str("");
	str.AppendFloat(v, 2);
	return string(str);
}

void UpdateDisplay() {
	hw.display.Fill(false);
	vector<string> strs;

	strs.push_back("flip bit v7");
	strs.push_back(int_to_string(bit_to_flip));
	// strs.push_back(float_to_string(sig_min_v));
	// strs.push_back(float_to_string(sig_max_v));
	// sig_min_v = sig_max_v = 0;

	DisplayLines(strs);
	hw.display.Update();
}

int main(void) {
	hw.Init();
	hw.SetAudioBlockSize(4); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
	hw.StartAdc();
	hw.StartAudio(AudioCallback);

	ctrl1.Init(hw.controls[0], 0.0f, 1.0f, Parameter::LINEAR);

	while(true) {
		for (size_t i = 0; i < 100; i++) {
			UpdateControls();
		}
		UpdateDisplay();
	}
}
