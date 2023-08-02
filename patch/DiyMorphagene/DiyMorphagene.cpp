#include <string>

#include "daisy_patch.h"
#include "daisysp.h"
#include "Dynamics/crossfade.h"

using namespace std;
using namespace daisy;
using namespace daisysp;

DaisyPatch hw;

const size_t BUFFER_SIZE = 60 * 48000;  // max of 60sec
float DSY_SDRAM_BSS buffer[BUFFER_SIZE];

// where in the sample we are playing/recording
size_t sample_offset;

// starting position dictated by ctrl2.
float last_ctrl2_val;
size_t sample_start;

// effective sample start; first +ve zero crossing after sample_start
size_t sample_effective_start;

// effective sample end; last +ve zero crossing before sample_end
size_t sample_effective_end;

// sample end dictated by ctrl3 (for length) + sample_start ( from ctrl2 )
float last_ctrl3_val;
size_t sample_end;

// actual recorded loop
size_t sample_length;

// 0 < sample_start < sample_effective_start < sample_effective_end < sample_end < sample_length
// sample_effective_start < sample_offset < sample_effective_end

enum State {
  WAITING,
  RECORDING,
  PLAYING,
};
State state = State::WAITING;

void SetEffectiveSampleStart() {
  // decide effective sample start by seeking forward
  // from start of buffer to first positive crossing
  float last_val = buffer[sample_start];
  for (size_t b = sample_start+1; b < sample_length; b++) {
    float value = buffer[b];
    if (last_val < 0 && value > 0) {
      sample_effective_start = b;
      return;
    }
    last_val = value;
  }
  // failed to find a crossing :()
  sample_effective_start = sample_start;
}

void SetEffectiveSampleEnd() {
  // decide effective sample end by seeking backwards
  // from the end of the buffer to find the last
  // positive crossing
  float last_val = buffer[sample_end-1];
  for (size_t b = sample_end-2; b > sample_effective_start; b--) {
    float value = buffer[b];
    if (last_val > 0 && value < 0) {
      sample_effective_end = b;
      return;
    }
    last_val = value;
  }
  // failed to find a crossing :()
  sample_effective_end = sample_end-1;
}

void UpdateSampleStartEnd() {
  sample_start = static_cast<size_t>(last_ctrl2_val * sample_length);
  SetEffectiveSampleStart();
  sample_end = sample_start + static_cast<size_t>(last_ctrl3_val * sample_length);
  if (sample_end >= sample_length) {
    sample_end = sample_length-1;
  }
  SetEffectiveSampleEnd();
}

void StopRecording() {
  sample_length = sample_offset-1;
  UpdateSampleStartEnd();
  sample_offset = sample_effective_start;
  state = PLAYING;
}

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {

  for (size_t b = 0; b < size; b++) {
    switch(state) {

      case WAITING:
        out[0][b] = in[0][b];
        break;

      case RECORDING:
        out[0][b] = in[0][b];
        buffer[sample_offset] = in[0][b];
        sample_offset++;
        if (sample_offset == BUFFER_SIZE) {
          StopRecording();
        }
        break;

      case PLAYING:
        if (sample_offset >= sample_effective_end) {
          sample_offset = sample_effective_start;
        }
        out[0][b] = buffer[sample_offset];
        sample_offset++;

        // core wave based on proportion we are through effective sample
        float core = float(sample_offset - sample_effective_start) / (sample_effective_end - sample_effective_start);
        out[1][b] = core;
        break;
    }

  }

}

void UpdateControls() {
  hw.ProcessAllControls();

  // check if ctrl2 ( sample start ) or ctrl3 ( sample length ) have changed
  bool changed = false;
  float val = hw.controls[1].Value();
  if (val<0) { val=}
  if (abs(last_ctrl2_val - val) > 0.001) {
    changed = true;
    last_ctrl2_val = val;
  }
  val = hw.controls[2].Value();
  if (abs(last_ctrl3_val - val) > 0.001) {
    changed = true;
    last_ctrl3_val = val;
  }
  // if either have changed we need to update the sample
  // start/end BUT we only do this dynamically during playback
  if (changed && state == PLAYING) {
    UpdateSampleStartEnd();
  }

  // click on encoder or trig at gate1 toggles recording
  if (hw.encoder.RisingEdge() || hw.gate_input[0].Trig()) {
    if (state == RECORDING) {
      StopRecording();
    } else {
      state = RECORDING;
      sample_offset = 0;
    }
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

void UpdateDisplay() {
  hw.display.Fill(false);
  vector<string> strs;

  FixedCapStr<18> str("");

  str.Append("state ");
  switch (state) {
    case WAITING:
      str.Append("WAITING ");
      break;
    case RECORDING:
      str.Append("RECORDING ");
      break;
    case PLAYING:
      str.Append("PLAYING ");
      break;
  }
  strs.push_back(string(str));

  str.Clear();
  str.Append("offset  ");
  str.AppendInt(sample_offset);
  strs.push_back(string(str));

  str.Clear();
  str.Append("start   ");
  str.AppendInt(sample_start);
  strs.push_back(string(str));

  str.Clear();
  str.Append("e_start ");
  str.AppendInt(sample_effective_start);
  strs.push_back(string(str));

  str.Clear();
  str.Append("e_end   ");
  str.AppendInt(sample_effective_end);
  strs.push_back(string(str));

  str.Clear();
  str.Append("end     ");
  str.AppendInt(sample_end);
  strs.push_back(string(str));

  str.Clear();
  str.Append("length  ");
  str.AppendInt(sample_length);
  strs.push_back(string(str));

  DisplayLines(strs);
  hw.display.Update();
}

int main(void) {
  hw.Init();

  hw.SetAudioBlockSize(64); // number of samples handled per callback
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

