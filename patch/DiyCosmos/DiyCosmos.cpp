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

class Loop {

	public:
		Loop() : idx_(0) {}

		void Process(const float* input, float* output) {
			
			// integrate input to loop buffer
			if (record_amount > 0) {
				for (size_t b=0; b<BLOCK_SIZE; b++) {
					(*buffer_)[idx_][b] += record_amount * input[b];
				}
			}

			// and mirror to output
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
	str.AppendFloat(record_amount, 2);
	strs.push_back("rec4 " + string(str));
	
	strs.push_back(loop1.State());

	DisplayLines(strs);
	patch.display.Update();
}

int main(void) {

	patch.Init();

	loop1.SetBuffer(&loop_1_buffer);
	loop1.Reset();

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
