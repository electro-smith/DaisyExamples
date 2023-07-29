#include <string>

#include "daisy_patch.h"
#include "daisysp.h"

using namespace std;
using namespace daisy;
using namespace daisysp;

// need to shift things down or dft lowest on sequencer is still too high
const int TRANSPOSE_OCTAVES = -1;

DaisyPatch hw;
Oscillator osc;

// from https://github.com/stablum/AdditiveVoice/blob/master/additive_voice.cpp
float VOctFromCtrl(const int ctrl_id) {
  float in = hw.controls[ctrl_id].Value();
  float voltage = in * 5.0f + TRANSPOSE_OCTAVES;
  float freq = powf(2.f, voltage) * 55;
  return freq;
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
  for (size_t i = 0; i < size; i++) {
    out[0][i] = osc.Process();
    out[1][i] = in[1][i];
    out[2][i] = in[2][i];
    out[3][i] = in[3][i];
  }
}

void UpdateControls() {
  hw.ProcessAllControls();
  osc.SetFreq(VOctFromCtrl(0));
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
  DisplayLines(strs);
  hw.display.Update();
}

int main(void) {
  hw.Init();

  osc.Init(48000);
  osc.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);

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
