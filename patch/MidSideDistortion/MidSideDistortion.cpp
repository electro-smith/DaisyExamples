#include "daisy_patch.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;
using namespace std;

DaisyPatch patch;

const size_t MAX_DELAY_LINE_SIZE = 4096;

DelayLine<float, MAX_DELAY_LINE_SIZE> delay;

Parameter delay_amt_param;
float delay_amt;       // delay amount as samples (i.e. without interp)

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {

  for (size_t i = 0; i < size; i++) {
    // read raw left / right
    float in_left = in[0][i];
    float in_right = in[1][i];

    // split mid and side
    float out_mid = (in_left + in_right) / 2;
    float out_side = (in_left - in_right) / 2;

    // send both off for processing
    out[0][i] = out_mid;
    out[1][i] = out_side;

    // return processed values (from last step)
    float in_mid = in[2][i];
    float in_side = in[3][i];

    // recombine to left/right
    float out_left = in_mid + in_side;
    float out_right = in_mid - in_side;

    // send
    out[2][i] = out_left;
    out[3][i] = out_right;
  }

}

void ProcessControls() {
  patch.ProcessAllControls();
  delay_amt = delay_amt_param.Process();
  delay.SetDelay(delay_amt);
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

// inline string drive_amount_to_s(int i) {
//   float a = drive_amount[i];
//   if (a<0.2) {
//     return "OFF (<20)";
//   } else {
//     return to_string(static_cast<uint32_t>(a*100));
//   }
// }

void UpdateDisplay() {
  patch.display.Fill(false);
  vector<string> strs;
  FixedCapStr<20> delay_amt_str("delay ");
  delay_amt_str.AppendFloat(delay_amt, 3);
  strs.push_back(string(delay_amt_str));
  DisplayLines(strs);
  patch.display.Update();
}

int main(void) {
  patch.Init();
  patch.StartAdc();
  patch.StartAudio(AudioCallback);

  delay.Init();
  //const uint32_t init_delay = 1;
  //delay.SetDelay(init_delay);

  delay_amt_param.Init(patch.controls[0], 0.f, 4095.f, Parameter::LINEAR);

  while(true) {
    for (size_t i=0; i<100; i++) {
      ProcessControls();
    }
    UpdateDisplay();
  }

}
