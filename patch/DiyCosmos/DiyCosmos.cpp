#include "daisy_patch.h"
#include "daisysp.h"

#include <string>

using namespace daisy;
using namespace daisysp;
using namespace std;

const size_t BLOCK_SIZE = 16;

using FloatBlock = float[BLOCK_SIZE];

const size_t LOOP_BLOCKS = 10000;
using LoopBuffer = FloatBlock[LOOP_BLOCKS];
static LoopBuffer DSY_SDRAM_BSS loop_1_buffer;

DaisyPatch patch;
Parameter ctrls[4];

// CONTROL 0
// how much of input signal to record to loops
// 0 -> no recording
// 1 -> full mix
float record_amount = 0;

// CONTROL 1
// feedback amount; how much, if anything, loop decays over time
// 0 -> instant decay; no loop
// 1 -> loop stays
// 1.2 -> feedback
float feedback_amount = 0;

class Loop {

	public:
		Loop() : idx_(0) {}

		void Process(const float* input, float* output) {
			
			// process feedback
			if (feedback_amount != 1.0) {
				for (size_t b=0; b<BLOCK_SIZE; b++) {
					(*buffer_)[idx_][b] *= feedback_amount;
				}
			}

			// integrate input to loop buffer
			if (record_amount > 0) {
				for (size_t b=0; b<BLOCK_SIZE; b++) {
					(*buffer_)[idx_][b] += record_amount * input[b];
				}
			}

			// write to output
			copy_n((*buffer_)[idx_], BLOCK_SIZE, output);

			// step forward in loop
			idx_++;
			if (idx_ == LOOP_BLOCKS) {
				idx_ = 0;
			}

		}

		void Reset() {
		 	for (size_t l=0; l<LOOP_BLOCKS; l++) {
				for (size_t b=0; b<BLOCK_SIZE; b++) {
					(*buffer_)[l][b] = 0.;
				}
			}
		}

		string State() {
			return to_string(static_cast<uint32_t>(idx_));
		}

		inline void SetBuffer(LoopBuffer* buffer) { buffer_ = buffer; }

	private:
		size_t idx_;
		LoopBuffer* buffer_;
};

Loop loop1;

void AudioCallback(AudioHandle::InputBuffer in, 
				   AudioHandle::OutputBuffer out, 
				   size_t size) {	

	// monitor in 0 from out 0
	copy_n(in[0], size, out[0]);
	
	// process in0 to loop1, output to out1
	loop1.Process(in[0], out[1]);

}

 void UpdateControls() {
 	patch.ProcessAllControls();
	
 	record_amount = ctrls[0].Process();
 	if (record_amount < 0.02) {
 		record_amount = 0;
 	} else if (record_amount > 0.98) {
 		record_amount = 1;
 	}

	feedback_amount = ctrls[1].Process();
	if (feedback_amount < 0.02) {
		feedback_amount = 0;
	} else if (feedback_amount > 0.98 && feedback_amount < 1.02) {
		feedback_amount = 1;
	} else if (feedback_amount > 1.18) {
		feedback_amount = 1.2;
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

string TwoDecimalPoints(float f) {
	FixedCapStr<10> str("");
	str.AppendFloat(f, 2);
	return string(str);
}

void UpdateDisplay() {
	patch.display.Fill(false);
	vector<string> strs;

	strs.push_back("rec " + TwoDecimalPoints(record_amount));
	strs.push_back("fb  " + TwoDecimalPoints(feedback_amount));
	
	strs.push_back(loop1.State());

	DisplayLines(strs);
	patch.display.Update();
}

int main(void) {

	patch.Init();

	loop1.SetBuffer(&loop_1_buffer);
	loop1.Reset();

	ctrls[0].Init(patch.controls[0], 0.0f, 1.0f, Parameter::LINEAR);  // record_amount
	ctrls[1].Init(patch.controls[1], 0.0f, 1.2f, Parameter::LINEAR);  // feedback_amount

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
