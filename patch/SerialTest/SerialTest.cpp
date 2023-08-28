#include <string>

#include "daisy_patch.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;
using namespace std;

DaisyPatch hw;
CpuLoadMeter cpu_load_meter;

size_t n = 0;

const int WAITING = 0;
const int RECORDING = 1;
const int FLUSHING = 2;
const int DONE = 3;
int state = WAITING;

const size_t BUFFER_SIZE = 10000;
const size_t BLOCK_SIZE = 32;
float DSY_SDRAM_BSS ctrl_vals[BUFFER_SIZE];              // ctrl value
float DSY_SDRAM_BSS buffer[BUFFER_SIZE][2][BLOCK_SIZE];  // in1, in2
size_t buffer_idx = 0;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                    size_t size) {

  hw.ProcessAllControls();

  if (hw.encoder.RisingEdge() && state==WAITING) {
    state = RECORDING;
  }

  // cpu_load_meter.OnBlockStart();

  for (size_t i = 0; i < size; i++) {
    out[0][i] = in[0][i];
    out[1][i] = in[1][i];
  }

  if (state == RECORDING) {
    ctrl_vals[buffer_idx] = hw.controls[0].Value();
    for (size_t i = 0; i < size; i++) {
      buffer[buffer_idx][0][i] = in[0][i];
      buffer[buffer_idx][1][i] = in[1][i];
    }
    buffer_idx++;
    if (buffer_idx == BUFFER_SIZE) {
      state = FLUSHING;
    }
  }

  n++;

  // cpu_load_meter.OnBlockEnd();
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

  FixedCapStr<20> str("");
  str.Append("n ");
  str.AppendInt(n);
  strs.push_back(string(str));

  str.Clear();
  str.Append("state ");
  str.AppendInt(state);
  strs.push_back(string(str));

  str.Clear();
  str.Append("buffer_idx ");
  str.AppendInt(buffer_idx);
  strs.push_back(string(str));

  DisplayLines(strs);
  hw.display.Update();

  if (state == FLUSHING) {
    FixedCapStr<50> str("");
    for (size_t i=0; i<BUFFER_SIZE; i++) {
      str.Clear();
      str.Append("b ");
      str.AppendInt(i);
      str.Append(" ");
      str.AppendFloat(ctrl_vals[i], 7);
      hw.seed.PrintLine(str);
      for (size_t b=0; b<BLOCK_SIZE; b++) {
        str.Clear();
        str.AppendFloat(buffer[i][0][b], 7);
        str.Append(" ");
        str.AppendFloat(buffer[i][1][b], 7);
        hw.seed.PrintLine(str);
      }
    }
    state = DONE;
  }

}


int main(void) {
  hw.Init();
  hw.SetAudioBlockSize(BLOCK_SIZE); // number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  hw.StartAdc();

  // Enable Logging, and set up the USB connection.
  hw.seed.StartLog();

  cpu_load_meter.Init(hw.AudioSampleRate(), hw.AudioBlockSize());
  hw.StartAudio(AudioCallback);

  while(1) {
    hw.ProcessAllControls();
    UpdateDisplay();
  }
}

