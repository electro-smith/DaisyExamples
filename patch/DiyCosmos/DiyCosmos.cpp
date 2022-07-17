#include "daisy_patch.h"
#include "daisysp.h"

#include <string>

using namespace daisy;
using namespace daisysp;
using namespace std;

const size_t BLOCK_SIZE = 16;

using FloatBlock = float[BLOCK_SIZE];

const size_t MAX_LOOP_BLOCKS = 30000;
using LoopBuffer = FloatBlock[MAX_LOOP_BLOCKS];
static LoopBuffer DSY_SDRAM_BSS loop_buffer[2];

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

float inp_min_v, inp_max_v;

string FewDecimalPoints(float f) {
	FixedCapStr<10> str("");
	str.AppendFloat(f, 2);
	return string(str);
}

class Loop {

	public:
		Loop() : len_(0), idx_(0), min_v_(100), max_v_(-100) {}

		void Process(const float* input, float* output) {

			// take a pointer to the current loop block
			FloatBlock* current = &((*buffer_)[idx_]);

			// process feedback
			if (feedback_amount != 1.0) {
				for (size_t b=0; b<BLOCK_SIZE; b++) {
					(*current)[b] *= feedback_amount;
				}
			}

			// integrate input to loop buffer
			if (record_amount > 0) {
				for (size_t b=0; b<BLOCK_SIZE; b++) {
					(*current)[b] += record_amount * input[b];
				}
			}

			// debug loop min max values
			// for (size_t b=0; b<BLOCK_SIZE; b++) {
			// 	const float f = (*current)[b];
			// 	if (f < min_v_) {
			// 		min_v_ = f;
			// 	} else if (f > max_v_) {
			// 		max_v_ = f;
			// 	}
			// }

			// write to output
			copy_n(*current, BLOCK_SIZE, output);

			// step forward in loop
			idx_++;
			if (idx_ == len_) {
				idx_ = 0;
			}

		}

		void Reset() {
			if (len_ > MAX_LOOP_BLOCKS) {
				len_ = MAX_LOOP_BLOCKS;
			}
			// zero entire loop buffer
		 	for (size_t l=0; l<len_; l++) {
				for (size_t b=0; b<BLOCK_SIZE; b++) {
					(*buffer_)[l][b] = 0.;
				}
			}

			// reset debugging min/max values
			min_v_ = 100;
			max_v_ = -100;
		}

		string State() {
			float block_min_v = 100;
			float block_max_v = -100;			
			for (size_t b=0; b<BLOCK_SIZE; b++) {
				const float v = (*buffer_)[idx_][b];
				if (v < block_min_v) {
					block_min_v = v;
				} else if (v > block_max_v) {
					block_max_v = v;
				}
			}

			return " " + FewDecimalPoints(block_min_v) +
				" " + FewDecimalPoints(block_max_v) + 
				" " + to_string(static_cast<uint32_t>(idx_));
		}

		inline void SetLength(size_t len) { len_ = len; }
		inline void SetBuffer(LoopBuffer* buffer) { buffer_ = buffer; }

	private:
		size_t len_;
		size_t idx_;
		LoopBuffer* buffer_;		
		float min_v_;
		float max_v_;
};

Loop loop1;

void AudioCallback(AudioHandle::InputBuffer in, 
				   AudioHandle::OutputBuffer out, 
				   size_t size) {	

	// debug capture min max values for input since last reset
	for (size_t b=0; b<size; b++) {
		const float f = in[0][b];
		if (f < inp_min_v) {
			inp_min_v = f;
		} else if (f > inp_max_v) {
			inp_max_v = f;
		}
	}

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



void UpdateDisplay() {
	patch.display.Fill(false);
	vector<string> strs;

	strs.push_back("rec " + FewDecimalPoints(record_amount));
	strs.push_back("fb  " + FewDecimalPoints(feedback_amount));
	//strs.push_back("mm " + FewDecimalPoints(inp_min_v) + " " + FewDecimalPoints(inp_max_v));
	
	strs.push_back(loop1.State());	

	DisplayLines(strs);
	patch.display.Update();
}

int main(void) {

	patch.Init();

	loop1.SetBuffer(&loop_buffer[0]);
	loop1.SetLength(15000);
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
