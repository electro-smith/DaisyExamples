#include "daisy_patch.h"
#include "daisysp.h"

#include <string>

using namespace daisy;
using namespace daisysp;
using namespace std;

const size_t BLOCK_SIZE = 32;

using FloatBlock = float[BLOCK_SIZE];

const size_t MAX_LOOP_BLOCKS = 33000;
using LoopBuffer = FloatBlock[MAX_LOOP_BLOCKS];

const size_t N_LOOPS = 3;
static LoopBuffer DSY_SDRAM_BSS loop_buffer[N_LOOPS];

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

string FewDecimalPoints(float f) {
	FixedCapStr<10> str("");
	str.AppendFloat(f, 2);
	return string(str);
}

class Loop {

	public:
		Loop() : len_(0), idx_(0) {}

		void Process(const float* input, float* output) {

			// take a pointer to the current loop block
			FloatBlock* current = &((*buffer_)[idx_]);

			// process feedback
			if (feedback_amount != 1.0) {
				for (size_t b = 0; b < BLOCK_SIZE; b++) {
					(*current)[b] *= feedback_amount;
				}
			}

			// integrate input to loop buffer
			if (record_amount > 0) {
				for (size_t b = 0; b < BLOCK_SIZE; b++) {
					(*current)[b] += record_amount * input[b];
				}
			}

			// for first block use linear envelope to ramp up from 0.
			if (idx_ == 0) {
				for (size_t b = 0 ; b < BLOCK_SIZE; b++) {
					const float proportion = static_cast<float>(b) / (BLOCK_SIZE-1);
					(*current)[b] *= proportion;
				}
			}
			// for last block use linear envelope to ramp down to 0.
			else if (idx_ == len_-1) {
				for (size_t b = 0 ; b < BLOCK_SIZE; b++) {
					const float proportion = static_cast<float>(b) / (BLOCK_SIZE-1);
					(*current)[b] *= 1.0 - proportion;
				}
			}

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
			for (size_t l = 0; l < len_; l++) {
				for (size_t b = 0; b < BLOCK_SIZE; b++) {
					(*buffer_)[l][b] = 0.;
				}
			}
		}

		string State() {
			return to_string(static_cast<uint32_t>(idx_));
		}

		inline void SetLength(size_t len) { len_ = len; }
		inline void SetBuffer(LoopBuffer* buffer) { buffer_ = buffer; }

	private:
		size_t len_;
		size_t idx_;
		LoopBuffer* buffer_;
};

Loop loop[N_LOOPS];

void AudioCallback(AudioHandle::InputBuffer in,
				   AudioHandle::OutputBuffer out,
				   size_t size) {

	// monitor in 0 from out 0
	copy_n(in[0], size, out[0]);

	// process in0 to loop1, output to out1
	for (size_t l = 0; l < N_LOOPS; l++) {
		loop[l].Process(in[0], out[l+1]);
	}

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
		for (size_t l = 0; l < N_LOOPS; l++) {
			loop[l].Reset();
		}
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

	for (size_t l = 0; l < N_LOOPS; l++) {
		strs.push_back(loop[l].State());
	}

	DisplayLines(strs);
	patch.display.Update();
}

int main(void) {

	patch.Init();

	for (size_t l = 0; l < N_LOOPS; l++) {
		loop[l].SetBuffer(&loop_buffer[l]);
	}

	loop[0].SetLength(15000);  // 5 * 3000
	loop[1].SetLength(21000);  // 7 * 3000
	loop[2].SetLength(33000);  // 11 * 3000

	for (size_t l = 0; l < N_LOOPS; l++) {
		loop[l].Reset();
	}

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

