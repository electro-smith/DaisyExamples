#include "daisy_patch.h"
#include "daisysp.h"

#include <string>

using namespace std;
using namespace daisy;
using namespace daisysp;

#define BIT_SET(a,b) ((a) |= (1ULL<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1ULL<<(b)))
#define BIT_CHECK(a,b) (!!((a) & (1ULL<<(b))))        // '!!' to make sure this returns 0 or 1
#define BITMASK_FLIP(x, mask) ((x) ^= (mask))

DaisyPatch hw;
// Parameter range_lhs;
// Parameter range_rhs;

const int NUM_BITS = 8;
const int MAX_VALUE = pow(2, NUM_BITS) - 1;

int cursor = 0;     // which bit is the cursor controlled by the encoder above?
int flip_mask = 0;

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

inline float bitflip(float in_v) {
	// cast to int
	int int_v = to_int(in_v);
	// flip bit
	BITMASK_FLIP(int_v, flip_mask);
	// return as float
	return to_float(int_v);
}

inline int roll(int v) {
    const bool left_most_bit_set = BIT_CHECK(v, 0);
    v >>= 1;
    if (left_most_bit_set) {
        BIT_SET(v, 7);
    }
    return v;
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
	for (size_t i = 0; i < size; i++) {
		out[0][i] = bitflip(in[0][i]);
		out[1][i] = in[1][i];
		out[2][i] = in[2][i];
		out[3][i] = in[3][i];
	}
}

void UpdateControls() {
	hw.ProcessAllControls();

	// moving encoder moves cursor for mask bit flipping
	cursor += hw.encoder.Increment();
	if (cursor < 0) {
		cursor = 0;
	} else if (cursor > NUM_BITS-1) {
		cursor = NUM_BITS-1;
	}

	// clicking encoder toggles mask bit state
	if (hw.encoder.RisingEdge()) {
		BIT_FLIP(flip_mask, cursor);
	}

	// gate1 trigger rolls flip mask left
	if (hw.gate_input[0].Trig()) {
		flip_mask = roll(flip_mask);
	}
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

// inline string int_to_string(const int v) {
// 	FixedCapStr<5> str("");
// 	str.AppendInt(v);
// 	return string(str);
// }

// inline string float_to_string(const float v) {
// 	FixedCapStr<5> str("");
// 	str.AppendFloat(v, 2);
// 	return string(str);
// }

void UpdateDisplay() {
	hw.display.Fill(false);
	vector<string> strs;

	strs.push_back("flip bit v7");

	// bits of mask
	FixedCapStr<10> mask_str("");
	for (int i=0; i<NUM_BITS; i++) {
		if (BIT_CHECK(flip_mask, i)) {
			mask_str.Append("1");
		} else {
			mask_str.Append("0");
		}
	}
	strs.push_back(string(mask_str));

	// create a string with ^ for cursor position
	FixedCapStr<10> cursor_str("");
	for (int i=0; i<NUM_BITS; i++) {
		if (i==cursor) {
			cursor_str.Append("^");
		} else {
			cursor_str.Append(" ");
		}
	}
	strs.push_back(string(cursor_str));

	DisplayLines(strs);
	hw.display.Update();
}

int main(void) {
	hw.Init();
	hw.SetAudioBlockSize(4); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
	hw.StartAdc();
	hw.StartAudio(AudioCallback);

	// range_lhs.Init(hw.controls[0], 0.0f, 1.0f, Parameter::LINEAR);
	// range_rhs.Init(hw.controls[1], 0.0f, 1.0f, Parameter::LINEAR);

	while(true) {
		for (size_t i = 0; i < 100; i++) {
			UpdateControls();
		}
		UpdateDisplay();
	}
}
