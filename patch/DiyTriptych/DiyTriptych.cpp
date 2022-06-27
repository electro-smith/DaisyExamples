#include "daisy_patch.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;
using namespace std;

DaisyPatch patch;

// https://github.com/electro-smith/DaisySP/blob/master/Source/Filters/moogladder.h
//daisysp::MoogLadder filter;

// https://github.com/electro-smith/DaisySP/blob/master/Source/Effects/overdrive.h
daisysp::Overdrive overdrive;

// https://github.com/electro-smith/DaisySP/blob/master/Source/Effects/fold.h ??
daisysp::Fold fold;

// https://github.com/electro-smith/DaisySP/blob/master/Source/Filters/comb.h
daisysp::Comb comb;
const size_t COMB_BUFFER_SIZE = 9600;
float comb_buffer[COMB_BUFFER_SIZE];

Parameter ctrls[4];
float ctrl_params[4];

size_t process_order = 0;

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {
	for (size_t i = 0; i < size; i++) {

    float val = in[0][i];
    switch(process_order) {
      case 0: // OD_FOLD_COMB
        val = overdrive.Process(val);
        val = fold.Process(val);
        val = comb.Process(val);
        break;
      case 1: // OD_COMB_FOLD
        val = overdrive.Process(val);
        val = comb.Process(val);
        val = fold.Process(val);
        break;
      case 2: // FOLD_OD_COMB
        val = fold.Process(val);
        val = overdrive.Process(val);
        val = comb.Process(val);
        break;
      case 3: // COMB_OD_FOLD
        val = comb.Process(val);
        val = overdrive.Process(val);
        val = fold.Process(val);
        break;
      case 4: // FOLD_COMB_OD
        val = fold.Process(val);
        val = comb.Process(val);
        val = overdrive.Process(val);
        break;
      case 5: // COMB_FOLD_OD
        val = comb.Process(val);
        val = fold.Process(val);
        val = overdrive.Process(val);
        break;
      default:
        break;
    };
		out[0][i] = val;

		out[1][i] = out[2][i] = out[3][i] = 0;
	}
}

void FlushCombBuffer() {
  for (size_t i=0; i<COMB_BUFFER_SIZE; i++) {
    comb_buffer[i] = 0;
  }
}

void ProcessControls() {
	patch.ProcessAllControls();

  for(size_t c=0; c<4; c++) {
    ctrl_params[c] = ctrls[c].Process();
  }

  if (ctrl_params[0] < 0.1) {
    ctrl_params[0] = 0.1;
  }
  overdrive.SetDrive(ctrl_params[0]);  // (0, 1)

  if (ctrl_params[1] < 1) {
    ctrl_params[1] = 1;
  }
  fold.SetIncrement(ctrl_params[1]);

  comb.SetFreq(ctrl_params[2]);     // Hz
  comb.SetRevTime(ctrl_params[3]);  // decay time

  if (patch.encoder.Increment() > 0) {
    process_order++;
    FlushCombBuffer();
  }
  else if (patch.encoder.Increment() < 0) {
    process_order--;
    FlushCombBuffer();
  }
  process_order %= 6;
}

void DisplayLines(const vector<string> &strs) {
  int line_num=0;
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
  switch(process_order) {
    case 0:
      strs.push_back("OD_FOLD_COMB");
      break;
    case 1:
      strs.push_back("OD_COMB_FOLD");
      break;
    case 2:
      strs.push_back("FOLD_OD_COMB");
      break;
    case 3:
      strs.push_back("COMB_OD_FOLD");
      break;
    case 4:
      strs.push_back("FOLD_COMB_OD");
      break;
    case 5:
      strs.push_back("COMB_FOLD_OD");
      break;
  }
  strs.push_back("");
  for(size_t c=0; c<4; c++) {
    FixedCapStr<20> str("");
    switch(c) {
      case 0:
        str.Append("DRIVE ");
        break;
      case 1:
        str.Append("FOLD  ");
        break;
      case 2:
        str.Append("CMB_F ");
        break;
      case 3:
        str.Append("CMB_R ");
        break;
    }
    str.AppendFloat(ctrl_params[c], 2);
    strs.push_back(string(str));
  }
  DisplayLines(strs);
  patch.display.Update();
}

int main(void) {
	patch.Init();
	patch.StartAdc();
	patch.StartAudio(AudioCallback);

  ctrls[0].Init(patch.controls[0], 0.f, 1.f, Parameter::LINEAR);       // overdrive.SetDrive
  ctrls[1].Init(patch.controls[1], 0.f, 100.f, Parameter::LINEAR);     // fold.SetIncrement
  ctrls[2].Init(patch.controls[2], 15, 2000, Parameter::LOGARITHMIC);  // comb.SetFreq
  ctrls[3].Init(patch.controls[3], 0.f, 1.f, Parameter::LINEAR);       // comb.SetRevTime

//  filter.Init(patch.AudioSampleRate());

  FlushCombBuffer();
  comb.Init(patch.AudioSampleRate(), comb_buffer, COMB_BUFFER_SIZE);

  while(true) {
    for (size_t i=0; i<10000; i++) {
      ProcessControls();
    }
    UpdateDisplay();
  }

}
