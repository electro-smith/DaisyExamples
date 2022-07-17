#include "daisy_patch.h"
#include "daisysp.h"

#include <string>

using namespace daisy;
using namespace daisysp;
using namespace std;

const size_t BLOCK_SIZE = 16;

using FloatBlock = float[BLOCK_SIZE];

const size_t LOOP_BLOCKS = 800;
using LoopBuffer = FloatBlock[LOOP_BLOCKS];
static LoopBuffer DSY_SDRAM_BSS loop_1_buffer;

DaisyPatch patch;
Parameter ctrls[4];

float wet_dry_mix;

class Loop {

	public:
		Loop() : idx_(0) {}

		void Process(const float* input) {
			for (size_t b=0; b<BLOCK_SIZE; b++) {
				if (input[b] > max_v_) {
					max_v_ = input[b];
				} else if (input[b] < min_v_) {
					min_v_ = input[b];
				}
			}
		}

		void Reset() {
			min_v_ = 10;
			max_v_ = -10;
		}

		string State() {
			FixedCapStr<20> str("min_max ");
			str.AppendFloat(min_v_, 2);
			str.Append(" ");
			str.AppendFloat(max_v_, 2);
			return string(str);
		}

		inline void SetBuffer(LoopBuffer* buffer) { buffer_ = buffer; }

	private:
		float min_v_;
		float max_v_;
		size_t idx_;
		LoopBuffer* buffer_;
};

Loop loop1;

void AudioCallback(AudioHandle::InputBuffer in, 
				   AudioHandle::OutputBuffer out, 
				   size_t size) {	

	// monitor in 0 from out 0
	copy_n(in[0], size, out[0]);

	loop1.Process(in[0]);

	// for (size_t i = 0; i < size; i++) {
	// 	out[0][i] = in[09][i];
	// 	for (size_t c = 0; c < 4; c++) {
	// 		out[c][i] = in[c][i];
	// 	}
	// }
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
		loop1.Reset();
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

	strs.push_back(loop1.State());

	DisplayLines(strs);
	patch.display.Update();
}

int main(void) {

	loop1.SetBuffer(&loop_1_buffer);

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
